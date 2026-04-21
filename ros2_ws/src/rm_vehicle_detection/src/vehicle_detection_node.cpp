#include "ai_msgs/msg/perception_targets.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"
#include "dnn_node/dnn_node.h"
#include "dnn_node/util/image_proc.h"
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#include "hobot_cv/hobotcv_imgproc.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/header.hpp"
#include "std_msgs/msg/string.hpp"

#include "parser.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string GetDefaultModelPath() {
  const auto package_prefix = ament_index_cpp::get_package_prefix("rm_vehicle_detection");
  return package_prefix + "/lib/rm_vehicle_detection/config/quant.bin";
}

rm_vehicle_detection::YoloBoxFormat ParseBoxFormat(const std::string &value) {
  if (value == "cxcywh") {
    return rm_vehicle_detection::YoloBoxFormat::kCxcywh;
  }
  return rm_vehicle_detection::YoloBoxFormat::kXyxy;
}

int ResizeNV12Img(const char *in_img_data,
                  const int &in_img_height,
                  const int &in_img_width,
                  const int &scaled_img_height,
                  const int &scaled_img_width,
                  cv::Mat &out_img,
                  float &x_ratio,
                  float &y_ratio) {
  cv::Mat src(
    in_img_height * 3 / 2, in_img_width, CV_8UC1, const_cast<char *>(in_img_data));

  float ratio_w =
    static_cast<float>(in_img_width) / static_cast<float>(scaled_img_width);
  float ratio_h =
    static_cast<float>(in_img_height) / static_cast<float>(scaled_img_height);
  float dst_ratio = std::max(ratio_w, ratio_h);

  int resized_width = scaled_img_width;
  int resized_height = scaled_img_height;
  if (dst_ratio == ratio_w) {
    resized_height = static_cast<int>(static_cast<float>(in_img_height) / dst_ratio);
  } else {
    resized_width = static_cast<int>(static_cast<float>(in_img_width) / dst_ratio);
  }

  const int remain = resized_width % 16;
  if (remain != 0) {
    resized_width -= remain;
    dst_ratio = static_cast<float>(in_img_width) / static_cast<float>(resized_width);
    resized_height = static_cast<int>(static_cast<float>(in_img_height) / dst_ratio);
  }
  resized_height = resized_height % 2 == 0 ? resized_height : resized_height - 1;

  x_ratio = dst_ratio;
  y_ratio = dst_ratio;

  return hobot_cv::hobotcv_resize(
    src, in_img_height, in_img_width, out_img, resized_height, resized_width);
}

struct VehicleNodeOutput : public hobot::dnn_node::DnnNodeOutput {
  float x_ratio = 1.0F;
  float y_ratio = 1.0F;
};

}  // namespace

class VehicleDetectionNode : public hobot::dnn_node::DnnNode {
 public:
  explicit VehicleDetectionNode(
    const std::string &node_name = "rm_vehicle_detection",
    const rclcpp::NodeOptions &options = rclcpp::NodeOptions())
  : hobot::dnn_node::DnnNode(node_name, options) {
    image_topic_ = this->declare_parameter<std::string>("image_topic", "/hbmem_img");
    output_topic_ =
      this->declare_parameter<std::string>("output_topic", "/vehicle_detection/targets");
    target_type_ = this->declare_parameter<std::string>("target_type", "vehicle");
    model_path_ = this->declare_parameter<std::string>("model_path", GetDefaultModelPath());
    debug_topic_ =
      this->declare_parameter<std::string>("debug_topic", "/vehicle_detection/debug_text");
    publish_debug_text_ = this->declare_parameter<bool>("publish_debug_text", false);
    debug_anchor_logs_ = this->declare_parameter<bool>("debug_anchor_logs", false);
    sample_roi_logs_ = this->declare_parameter<bool>("sample_roi_logs", false);
    box_format_name_ = this->declare_parameter<std::string>("box_format", "xyxy");
    if (box_format_name_ != "xyxy" && box_format_name_ != "cxcywh") {
      RCLCPP_WARN(
        this->get_logger(),
        "Unsupported box_format '%s', falling back to 'xyxy'",
        box_format_name_.c_str());
      box_format_name_ = "xyxy";
    }
    box_format_ = ParseBoxFormat(box_format_name_);
    score_threshold_ = this->declare_parameter<double>("score_threshold", 0.35);
    nms_threshold_ = this->declare_parameter<double>("nms_threshold", 0.5);
    nms_top_k_ = this->declare_parameter<int>("nms_top_k", 300);
    task_num_ = this->declare_parameter<int>("task_num", 4);
    log_fps_ = this->declare_parameter<bool>("log_fps", false);

    if (Init() != 0 || GetModelInputSize(0, model_input_width_, model_input_height_) < 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to initialize rm_vehicle_detection");
      rclcpp::shutdown();
      return;
    }

    LogModelInputInfo();

    image_subscription_ = this->create_subscription<hbm_img_msgs::msg::HbmMsg1080P>(
      image_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&VehicleDetectionNode::OnImage, this, std::placeholders::_1));

    publisher_ = this->create_publisher<ai_msgs::msg::PerceptionTargets>(output_topic_, 10);
    if (publish_debug_text_) {
      debug_publisher_ = this->create_publisher<std_msgs::msg::String>(debug_topic_, 10);
    }
  }

 protected:
  int SetNodePara() override {
    if (!dnn_node_para_ptr_) {
      return -1;
    }

    if (model_path_.empty()) {
      RCLCPP_ERROR(
        this->get_logger(),
        "Parameter 'model_path' is empty. Please set it to a valid quant.bin path");
      return -1;
    }

    dnn_node_para_ptr_->model_file = model_path_;
    dnn_node_para_ptr_->model_task_type = hobot::dnn_node::ModelTaskType::ModelInferType;
    dnn_node_para_ptr_->task_num = task_num_;
    return 0;
  }

  int PostProcess(
    const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output) override {
    if (!rclcpp::ok() || !node_output) {
      return 0;
    }

    const auto best_anchor = rm_vehicle_detection::FindBestAnchor(node_output);
    const auto top_anchors = rm_vehicle_detection::FindTopAnchors(node_output, 5);
    const auto output_debug = rm_vehicle_detection::GetOutputTensorDebugInfo(node_output);
    if (debug_anchor_logs_ && best_anchor.anchor >= 0) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 1000,
        "best raw anchor=%d score=%.4f raw0=%.4f raw1=%.4f raw2=%.4f raw3=%.4f",
        best_anchor.anchor,
        best_anchor.score,
        best_anchor.raw0,
        best_anchor.raw1,
        best_anchor.raw2,
        best_anchor.raw3);
    }

    auto pub_msg = std::make_unique<ai_msgs::msg::PerceptionTargets>();
    pub_msg->header = *node_output->msg_header;

    std::vector<std::shared_ptr<rm_vehicle_detection::YoloV8Detection>> detections;
    const rm_vehicle_detection::YoloV8ParserConfig parser_config {
      static_cast<float>(score_threshold_),
      static_cast<float>(nms_threshold_),
      nms_top_k_,
      target_type_,
      model_input_width_,
      model_input_height_,
      box_format_,
    };

    if (rm_vehicle_detection::ParseDetections(node_output, parser_config, detections) != 0) {
      RCLCPP_ERROR_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000, "Failed to parse vehicle detections");
      return -1;
    }

    auto vehicle_output = std::dynamic_pointer_cast<VehicleNodeOutput>(node_output);
    if (!vehicle_output) {
      RCLCPP_ERROR(this->get_logger(), "Failed to cast vehicle node output");
      return -1;
    }

    for (auto &det : detections) {
      if (!det) {
        continue;
      }

      const float xmin = std::max(0.0F, det->xmin * vehicle_output->x_ratio);
      const float ymin = std::max(0.0F, det->ymin * vehicle_output->y_ratio);
      const float xmax =
        std::min(static_cast<float>(model_input_width_ - 1), det->xmax) * vehicle_output->x_ratio;
      const float ymax =
        std::min(static_cast<float>(model_input_height_ - 1), det->ymax) * vehicle_output->y_ratio;

      if (xmax <= xmin || ymax <= ymin) {
        continue;
      }

      ai_msgs::msg::Roi roi;
      roi.rect.x_offset = static_cast<uint32_t>(std::lround(xmin));
      roi.rect.y_offset = static_cast<uint32_t>(std::lround(ymin));
      roi.rect.width = static_cast<uint32_t>(std::lround(xmax - xmin));
      roi.rect.height = static_cast<uint32_t>(std::lround(ymax - ymin));
      roi.confidence = det->score;

      ai_msgs::msg::Target target;
      target.type = det->class_name;
      target.rois.emplace_back(std::move(roi));
      pub_msg->targets.emplace_back(std::move(target));
    }

    if (sample_roi_logs_ && !pub_msg->targets.empty() && !pub_msg->targets.front().rois.empty()) {
      const auto &rect = pub_msg->targets.front().rois.front().rect;
      RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 1000,
        "vehicle roi sample x=%u y=%u w=%u h=%u targets=%zu",
        rect.x_offset, rect.y_offset, rect.width, rect.height, pub_msg->targets.size());
    }

    if (node_output->rt_stat) {
      pub_msg->fps = std::lround(node_output->rt_stat->output_fps);
      if (log_fps_ && node_output->rt_stat->fps_updated) {
        RCLCPP_INFO_THROTTLE(
          this->get_logger(), *this->get_clock(), 2000,
          "vehicle detection fps in=%.2f out=%.2f infer=%dms targets=%zu",
          node_output->rt_stat->input_fps,
          node_output->rt_stat->output_fps,
          node_output->rt_stat->infer_time_ms,
          pub_msg->targets.size());
      }
    }

    if (publish_debug_text_) {
      PublishDebugText(output_debug, top_anchors, detections, *pub_msg);
    }
    publisher_->publish(std::move(pub_msg));
    return 0;
  }

 private:
  void LogModelInputInfo() {
    auto *model = GetModel();
    if (!model) {
      RCLCPP_WARN(this->get_logger(), "Model handle is null; skip input-source logging");
      return;
    }

    int32_t input_source = -1;
    if (model->GetInputSource(input_source, 0) != 0) {
      RCLCPP_WARN(this->get_logger(), "Failed to query model input source");
      return;
    }

    hbDNNTensorProperties tensor_properties;
    std::memset(&tensor_properties, 0, sizeof(tensor_properties));
    if (model->GetInputTensorProperties(tensor_properties, 0) != 0) {
      RCLCPP_WARN(this->get_logger(), "Failed to query model input tensor properties");
      return;
    }

    RCLCPP_INFO(
      this->get_logger(),
      "vehicle model input_source=%d tensor_type=%d layout=%d valid_shape=[%d,%d,%d,%d] aligned_shape=[%d,%d,%d,%d]",
      input_source,
      tensor_properties.tensorType,
      tensor_properties.tensorLayout,
      tensor_properties.validShape.dimensionSize[0],
      tensor_properties.validShape.dimensionSize[1],
      tensor_properties.validShape.dimensionSize[2],
      tensor_properties.validShape.dimensionSize[3],
      tensor_properties.alignedShape.dimensionSize[0],
      tensor_properties.alignedShape.dimensionSize[1],
      tensor_properties.alignedShape.dimensionSize[2],
      tensor_properties.alignedShape.dimensionSize[3]);
  }

  void OnImage(const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr &msg) {
    if (!rclcpp::ok() || !msg) {
      return;
    }

    const std::string encoding(reinterpret_cast<const char *>(msg->encoding.data()));
    if (encoding != "nv12") {
      RCLCPP_ERROR_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000,
        "Only nv12 shared-memory images are supported, got '%s'", encoding.c_str());
      return;
    }

    auto output = std::make_shared<VehicleNodeOutput>();
    output->msg_header = std::make_shared<std_msgs::msg::Header>();
    output->msg_header->frame_id = std::to_string(msg->index);
    output->msg_header->stamp = msg->time_stamp;

    std::shared_ptr<hobot::dnn_node::NV12PyramidInput> pyramid;
    if (
      msg->height != static_cast<uint32_t>(model_input_height_) ||
      msg->width != static_cast<uint32_t>(model_input_width_))
    {
      cv::Mat resized;
      if (
        ResizeNV12Img(
          reinterpret_cast<const char *>(msg->data.data()),
          static_cast<int>(msg->height),
          static_cast<int>(msg->width),
          model_input_height_,
          model_input_width_,
          resized,
          output->x_ratio,
          output->y_ratio) < 0)
      {
        RCLCPP_ERROR(this->get_logger(), "Failed to resize NV12 image");
        return;
      }

      const uint32_t out_width = static_cast<uint32_t>(resized.cols);
      const uint32_t out_height = static_cast<uint32_t>(resized.rows * 2 / 3);
      pyramid = hobot::dnn_node::ImageProc::GetNV12PyramidFromNV12Img(
        reinterpret_cast<const char *>(resized.data),
        out_height,
        out_width,
        model_input_height_,
        model_input_width_);
    } else {
      pyramid = hobot::dnn_node::ImageProc::GetNV12PyramidFromNV12Img(
        reinterpret_cast<const char *>(msg->data.data()),
        msg->height,
        msg->width,
        model_input_height_,
        model_input_width_);
    }

    if (!pyramid) {
      RCLCPP_ERROR(this->get_logger(), "Failed to create NV12 pyramid input");
      return;
    }

    std::vector<std::shared_ptr<hobot::dnn_node::DNNInput>> inputs {pyramid};
    if (Run(inputs, output, nullptr, false) < 0) {
      RCLCPP_ERROR_THROTTLE(
        this->get_logger(), *this->get_clock(), 1000, "Vehicle inference run failed");
    }
  }

  bool BuildParserRectFromAnchor(
    const rm_vehicle_detection::DebugAnchorInfo &anchor_info,
    float &x1,
    float &y1,
    float &x2,
    float &y2) const {
    const float max_x = static_cast<float>(model_input_width_ - 1);
    const float max_y = static_cast<float>(model_input_height_ - 1);
    if (box_format_ == rm_vehicle_detection::YoloBoxFormat::kCxcywh) {
      const float cx = std::fabs(anchor_info.raw0) <= 2.0F ?
        anchor_info.raw0 * static_cast<float>(model_input_width_) : anchor_info.raw0;
      const float cy = std::fabs(anchor_info.raw1) <= 2.0F ?
        anchor_info.raw1 * static_cast<float>(model_input_height_) : anchor_info.raw1;
      const float width = std::max(
        0.0F,
        std::fabs(anchor_info.raw2) <= 2.0F ?
        anchor_info.raw2 * static_cast<float>(model_input_width_) : anchor_info.raw2);
      const float height = std::max(
        0.0F,
        std::fabs(anchor_info.raw3) <= 2.0F ?
        anchor_info.raw3 * static_cast<float>(model_input_height_) : anchor_info.raw3);
      x1 = std::clamp(cx - width * 0.5F, 0.0F, max_x);
      y1 = std::clamp(cy - height * 0.5F, 0.0F, max_y);
      x2 = std::clamp(cx + width * 0.5F, 0.0F, max_x);
      y2 = std::clamp(cy + height * 0.5F, 0.0F, max_y);
      return width > 0.0F && height > 0.0F && x2 > x1 && y2 > y1;
    }

    x1 = std::clamp(
      std::fabs(anchor_info.raw0) <= 2.0F ?
      anchor_info.raw0 * static_cast<float>(model_input_width_) : anchor_info.raw0,
      0.0F, max_x);
    y1 = std::clamp(
      std::fabs(anchor_info.raw1) <= 2.0F ?
      anchor_info.raw1 * static_cast<float>(model_input_height_) : anchor_info.raw1,
      0.0F, max_y);
    x2 = std::clamp(
      std::fabs(anchor_info.raw2) <= 2.0F ?
      anchor_info.raw2 * static_cast<float>(model_input_width_) : anchor_info.raw2,
      0.0F, max_x);
    y2 = std::clamp(
      std::fabs(anchor_info.raw3) <= 2.0F ?
      anchor_info.raw3 * static_cast<float>(model_input_height_) : anchor_info.raw3,
      0.0F, max_y);
    return x2 > x1 && y2 > y1;
  }

  void PublishDebugText(
    const rm_vehicle_detection::OutputTensorDebugInfo &output_debug,
    const std::vector<rm_vehicle_detection::DebugAnchorInfo> &top_anchors,
    const std::vector<std::shared_ptr<rm_vehicle_detection::YoloV8Detection>> &detections,
    const ai_msgs::msg::PerceptionTargets &pub_msg) {
    if (!debug_publisher_) {
      return;
    }

    std_msgs::msg::String debug_msg;
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(2);

    oss << "out l=" << output_debug.layout
        << " fmt=" << box_format_name_
        << " v=[" << output_debug.valid_shape[0]
        << "," << output_debug.valid_shape[1]
        << "," << output_debug.valid_shape[2]
        << "," << output_debug.valid_shape[3]
        << "] a=[" << output_debug.aligned_shape[0]
        << "," << output_debug.aligned_shape[1]
        << "," << output_debug.aligned_shape[2]
        << "," << output_debug.aligned_shape[3]
        << "] c=" << output_debug.channels
        << " n=" << output_debug.anchors;

    if (!top_anchors.empty()) {
      for (std::size_t i = 0; i < top_anchors.size(); ++i) {
        const auto &anchor = top_anchors[i];
        float x1 = 0.0F;
        float y1 = 0.0F;
        float x2 = 0.0F;
        float y2 = 0.0F;
        const bool valid = BuildParserRectFromAnchor(anchor, x1, y1, x2, y2);
        oss << "\n";
        oss << "raw t" << (i + 1)
            << " a=" << anchor.anchor
            << " s=" << anchor.score
            << " r0=" << anchor.raw0
            << " r1=" << anchor.raw1
            << " r2=" << anchor.raw2
            << " r3=" << anchor.raw3
            << " ok=" << (valid ? "y" : "n");
        if (valid) {
          oss << " p=(" << x1 << "," << y1 << "," << x2 << "," << y2 << ")";
        }
      }
    } else {
      oss << "\nraw top5: none";
    }

    if (!detections.empty() && detections.front()) {
      const auto &det = detections.front();
      oss << "\nparser[0] x1=" << det->xmin
          << " y1=" << det->ymin
          << " x2=" << det->xmax
          << " y2=" << det->ymax
          << " s=" << det->score;
    } else {
      oss << "\nparser[0]: none";
    }

    if (!pub_msg.targets.empty() && !pub_msg.targets.front().rois.empty()) {
      const auto &rect = pub_msg.targets.front().rois.front().rect;
      const auto &roi = pub_msg.targets.front().rois.front();
      oss << "\nroi[0] x=" << rect.x_offset
          << " y=" << rect.y_offset
          << " w=" << rect.width
          << " h=" << rect.height
          << " s=" << roi.confidence;
    } else {
      oss << "\nroi: none";
    }

    debug_msg.data = oss.str();
    debug_publisher_->publish(debug_msg);
  }

  std::string image_topic_;
  std::string output_topic_;
  std::string target_type_;
  std::string model_path_;
  std::string debug_topic_;
  std::string box_format_name_;
  double score_threshold_{0.35};
  double nms_threshold_{0.5};
  int nms_top_k_{300};
  int task_num_{4};
  bool log_fps_{false};
  bool publish_debug_text_{false};
  bool debug_anchor_logs_{false};
  bool sample_roi_logs_{false};
  rm_vehicle_detection::YoloBoxFormat box_format_{rm_vehicle_detection::YoloBoxFormat::kXyxy};
  int model_input_width_{-1};
  int model_input_height_{-1};
  rclcpp::Subscription<hbm_img_msgs::msg::HbmMsg1080P>::ConstSharedPtr image_subscription_;
  rclcpp::Publisher<ai_msgs::msg::PerceptionTargets>::SharedPtr publisher_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr debug_publisher_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VehicleDetectionNode>());
  rclcpp::shutdown();
  return 0;
}
