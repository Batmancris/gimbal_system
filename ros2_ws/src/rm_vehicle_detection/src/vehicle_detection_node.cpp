#include "ai_msgs/msg/perception_targets.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"
#include "dnn_node/dnn_node.h"
#include "dnn_node/util/image_proc.h"
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#include "hobot_cv/hobotcv_imgproc.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/header.hpp"

#include "parser.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string GetDefaultModelPath() {
  const auto package_prefix = ament_index_cpp::get_package_prefix("rm_vehicle_detection");
  return package_prefix + "/lib/rm_vehicle_detection/config/quant.bin";
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

    image_subscription_ = this->create_subscription<hbm_img_msgs::msg::HbmMsg1080P>(
      image_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&VehicleDetectionNode::OnImage, this, std::placeholders::_1));

    publisher_ = this->create_publisher<ai_msgs::msg::PerceptionTargets>(output_topic_, 10);
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

    publisher_->publish(std::move(pub_msg));
    return 0;
  }

 private:
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

  std::string image_topic_;
  std::string output_topic_;
  std::string target_type_;
  std::string model_path_;
  double score_threshold_{0.35};
  double nms_threshold_{0.5};
  int nms_top_k_{300};
  int task_num_{4};
  bool log_fps_{false};
  int model_input_width_{-1};
  int model_input_height_{-1};
  rclcpp::Subscription<hbm_img_msgs::msg::HbmMsg1080P>::ConstSharedPtr image_subscription_;
  rclcpp::Publisher<ai_msgs::msg::PerceptionTargets>::SharedPtr publisher_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VehicleDetectionNode>());
  rclcpp::shutdown();
  return 0;
}
