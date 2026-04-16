#include "ai_msgs/msg/perception_targets.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"
#include "dnn_node/dnn_node.h"
#include "dnn_node/util/image_proc.h"
#include "hbm_img_msgs/msg/hbm_msg1080_p.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/header.hpp"

#include "parser.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string GetDefaultModelPath() {
  const auto package_prefix = ament_index_cpp::get_package_prefix("rm_vehicle_detection");
  return package_prefix + "/lib/rm_vehicle_detection/config/quant.bin";
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
    max_inflight_ = this->declare_parameter<int>("max_inflight", 1);
    log_fps_ = this->declare_parameter<bool>("log_fps", false);
    if (max_inflight_ < 1) {
      RCLCPP_WARN(this->get_logger(), "max_inflight=%d is invalid, using 1", max_inflight_);
      max_inflight_ = 1;
    }

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
    const int previous_inflight = inflight_.fetch_sub(1, std::memory_order_relaxed);
    if (previous_inflight <= 0) {
      inflight_.store(0, std::memory_order_relaxed);
    }

    auto pub_msg = std::make_unique<ai_msgs::msg::PerceptionTargets>();
    pub_msg->header = *node_output->msg_header;

    LogOutputStats(node_output);

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

  void LogOutputStats(const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output) {
    if (!node_output || node_output->output_tensors.size() != 1) {
      return;
    }

    auto &tensor = node_output->output_tensors[0];
    hbSysFlushMem(&(tensor->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);
    const auto &shape = tensor->properties.validShape;
    if (shape.numDimensions < 3 || shape.dimensionSize[1] < 5) {
      return;
    }

    const int channels = shape.dimensionSize[1];
    int anchors = shape.dimensionSize[2];
    if (shape.numDimensions > 3 && shape.dimensionSize[3] > 0) {
      anchors *= shape.dimensionSize[3];
    }
    if (anchors <= 0) {
      return;
    }

    const auto *raw = reinterpret_cast<float *>(tensor->sysMem[0].virAddr);
    if (raw == nullptr) {
      return;
    }

    auto channel_min = std::vector<float>(channels, 0.0F);
    auto channel_max = std::vector<float>(channels, 0.0F);
    for (int c = 0; c < channels; ++c) {
      channel_min[c] = raw[c * anchors];
      channel_max[c] = raw[c * anchors];
    }

    double conf_sum = 0.0;
    for (int i = 0; i < anchors; ++i) {
      for (int c = 0; c < channels; ++c) {
        const float value = raw[c * anchors + i];
        channel_min[c] = std::min(channel_min[c], value);
        channel_max[c] = std::max(channel_max[c], value);
      }
      conf_sum += raw[4 * anchors + i];
    }

    const float conf_min = channel_min[4];
    const float conf_max = channel_max[4];
    const float conf_mean = static_cast<float>(conf_sum / static_cast<double>(anchors));
    const float conf_max_sigmoid = 1.0F / (1.0F + std::exp(-conf_max));

    RCLCPP_INFO_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "output[0] shape=[%d,%d,%d,%d] conf_raw min=%.4f max=%.4f mean=%.4f conf_sigmoid_max=%.4f",
      shape.dimensionSize[0],
      channels,
      shape.dimensionSize[2],
      shape.numDimensions > 3 ? shape.dimensionSize[3] : 1,
      conf_min,
      conf_max,
      conf_mean,
      conf_max_sigmoid);
    RCLCPP_INFO_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "output[0] channel range c0=[%.4f,%.4f] c1=[%.4f,%.4f] c2=[%.4f,%.4f] c3=[%.4f,%.4f] c4=[%.4f,%.4f]",
      channel_min[0],
      channel_max[0],
      channel_min[1],
      channel_max[1],
      channel_min[2],
      channel_max[2],
      channel_min[3],
      channel_max[3],
      channel_min[4],
      channel_max[4]);
  }

 private:
  void OnImage(const hbm_img_msgs::msg::HbmMsg1080P::ConstSharedPtr &msg) {
    if (!rclcpp::ok() || !msg) {
      return;
    }

    if (inflight_.load(std::memory_order_relaxed) >= max_inflight_) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000,
        "Dropping vehicle frame because inference is still busy: inflight=%d limit=%d",
        inflight_.load(std::memory_order_relaxed), max_inflight_);
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

    cv::Mat bgr_image;
    if (
      hobot::dnn_node::ImageProc::Nv12ToBGR(
        reinterpret_cast<const char *>(msg->data.data()),
        static_cast<int>(msg->height),
        static_cast<int>(msg->width),
        bgr_image) != 0)
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to convert NV12 image to BGR");
      return;
    }

    hbDNNTensorProperties input_properties;
    if (GetModel()->GetInputTensorProperties(input_properties, 0) != 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to get model input tensor properties");
      return;
    }

    float resize_ratio = 1.0F;
    auto tensor = hobot::dnn_node::ImageProc::GetBGRTensorFromBGRImg(
      bgr_image,
      model_input_height_,
      model_input_width_,
      input_properties,
      resize_ratio,
      hobot::dnn_node::ImageType::RGB,
      false);
    if (!tensor) {
      RCLCPP_ERROR(this->get_logger(), "Failed to create RGB tensor input");
      return;
    }
    output->x_ratio = static_cast<float>(msg->width) / static_cast<float>(model_input_width_);
    output->y_ratio = static_cast<float>(msg->height) / static_cast<float>(model_input_height_);

    std::vector<std::shared_ptr<hobot::dnn_node::DNNTensor>> inputs {tensor};
    inflight_.fetch_add(1, std::memory_order_relaxed);
    if (Run(inputs, output, true) < 0) {
      inflight_.fetch_sub(1, std::memory_order_relaxed);
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
  int max_inflight_{1};
  bool log_fps_{false};
  int model_input_width_{-1};
  int model_input_height_{-1};
  std::atomic<int> inflight_{0};
  rclcpp::Subscription<hbm_img_msgs::msg::HbmMsg1080P>::ConstSharedPtr image_subscription_;
  rclcpp::Publisher<ai_msgs::msg::PerceptionTargets>::SharedPtr publisher_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VehicleDetectionNode>());
  rclcpp::shutdown();
  return 0;
}
