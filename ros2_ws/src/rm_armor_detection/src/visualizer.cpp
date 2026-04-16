#include "ai_msgs/msg/perception_targets.hpp"
#include "cv_bridge/cv_bridge.h"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/image_encodings.hpp"
#include "sensor_msgs/msg/image.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

namespace {

constexpr char kWindowName[] = "rm_autoaim_visualizer";
constexpr int kLineThickness = 2;
constexpr int kFontFace = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kFontScale = 0.6;
constexpr int kTextBaselinePadding = 4;
constexpr int kTargetTimeoutMs = 250;

cv::Scalar ColorForIndex(std::size_t index) {
  static const std::vector<cv::Scalar> kPalette = {
    {0, 255, 0},
    {0, 200, 255},
    {255, 180, 0},
    {255, 0, 255},
    {255, 255, 0},
    {0, 128, 255},
  };
  return kPalette[index % kPalette.size()];
}

}  // namespace

class AutoAimVisualizerNode : public rclcpp::Node {
 public:
  AutoAimVisualizerNode() : Node("rm_autoaim_visualizer") {
    image_topic_ = declare_parameter<std::string>("image_topic", "/image_raw");
    targets_topic_ = declare_parameter<std::string>("targets_topic", "/dnn_node_sample");
    window_name_ = declare_parameter<std::string>("window_name", kWindowName);
    show_keypoints_ = declare_parameter<bool>("show_keypoints", true);
    target_timeout_ms_ = declare_parameter<int>("target_timeout_ms", kTargetTimeoutMs);

    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      image_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&AutoAimVisualizerNode::OnImage, this, std::placeholders::_1));

    targets_sub_ = create_subscription<ai_msgs::msg::PerceptionTargets>(
      targets_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&AutoAimVisualizerNode::OnTargets, this, std::placeholders::_1));

    cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
    cv::resizeWindow(window_name_, 1280, 720);

    RCLCPP_INFO(
      get_logger(),
      "Visualizer started. image_topic=%s targets_topic=%s window=%s",
      image_topic_.c_str(),
      targets_topic_.c_str(),
      window_name_.c_str());
  }

  ~AutoAimVisualizerNode() override {
    cv::destroyWindow(window_name_);
  }

 private:
  void OnTargets(const ai_msgs::msg::PerceptionTargets::SharedPtr msg) {
    if (!msg) {
      return;
    }

    std::lock_guard<std::mutex> lock(targets_mutex_);
    latest_targets_ = *msg;
    latest_targets_stamp_ = now();
  }

  void OnImage(const sensor_msgs::msg::Image::SharedPtr msg) {
    if (!msg) {
      return;
    }

    cv_bridge::CvImageConstPtr cv_ptr;
    try {
      cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8);
    } catch (const cv_bridge::Exception &e) {
      RCLCPP_ERROR_THROTTLE(
        get_logger(), *get_clock(), 2000, "cv_bridge conversion failed: %s", e.what());
      return;
    }

    cv::Mat frame = cv_ptr->image.clone();
    if (frame.empty()) {
      return;
    }

    ai_msgs::msg::PerceptionTargets targets_copy;
    bool targets_fresh = false;
    {
      std::lock_guard<std::mutex> lock(targets_mutex_);
      targets_copy = latest_targets_;
      targets_fresh =
        (latest_targets_stamp_.nanoseconds() > 0) &&
        ((now() - latest_targets_stamp_).nanoseconds() <=
         static_cast<int64_t>(target_timeout_ms_) * 1000 * 1000);
    }

    if (targets_fresh) {
      DrawTargets(frame, targets_copy);
    }

    UpdateDisplayFps();
    DrawStatus(frame, targets_copy, targets_fresh);
    cv::imshow(window_name_, frame);
    cv::waitKey(1);
  }

  void DrawTargets(cv::Mat &frame, const ai_msgs::msg::PerceptionTargets &targets_msg) {
    for (std::size_t i = 0; i < targets_msg.targets.size(); ++i) {
      const auto &target = targets_msg.targets[i];
      if (target.rois.empty()) {
        continue;
      }

      const auto &roi = target.rois.front();
      const auto color = ColorForIndex(i);

      const int x = std::max<int>(0, roi.rect.x_offset);
      const int y = std::max<int>(0, roi.rect.y_offset);
      const int w = std::max<int>(0, roi.rect.width);
      const int h = std::max<int>(0, roi.rect.height);
      if (w <= 0 || h <= 0) {
        continue;
      }

      cv::rectangle(frame, cv::Rect(x, y, w, h), color, kLineThickness);

      const std::string label =
        target.type + " " + cv::format("%.2f", roi.confidence);
      int baseline = 0;
      const auto text_size =
        cv::getTextSize(label, kFontFace, kFontScale, kLineThickness, &baseline);

      const int text_top = std::max(0, y - text_size.height - 2 * kTextBaselinePadding);
      const int text_left = std::max(0, x);
      const int text_width = std::min(frame.cols - text_left, text_size.width + 8);
      const int text_height = text_size.height + 2 * kTextBaselinePadding;
      cv::rectangle(
        frame,
        cv::Rect(text_left, text_top, text_width, text_height),
        color,
        cv::FILLED);
      cv::putText(
        frame,
        label,
        cv::Point(text_left + 4, text_top + text_height - kTextBaselinePadding),
        kFontFace,
        kFontScale,
        cv::Scalar(0, 0, 0),
        kLineThickness);

      if (!show_keypoints_) {
        continue;
      }
      for (const auto &point_group : target.points) {
        for (const auto &pt : point_group.point) {
          const int px = static_cast<int>(std::lround(pt.x));
          const int py = static_cast<int>(std::lround(pt.y));
          cv::circle(frame, cv::Point(px, py), 3, color, cv::FILLED);
        }
      }
    }
  }

  void DrawStatus(
    cv::Mat &frame,
    const ai_msgs::msg::PerceptionTargets &targets_msg,
    bool targets_fresh) {
    const std::string status = cv::format(
      "display_fps:%.1f dnn_fps:%u targets:%zu %s",
      display_fps_,
      targets_msg.fps,
      targets_msg.targets.size(),
      targets_fresh ? "live" : "stale");
    cv::putText(
      frame,
      status,
      cv::Point(20, 30),
      kFontFace,
      0.8,
      cv::Scalar(0, 255, 255),
      2);
  }

  void UpdateDisplayFps() {
    const auto stamp = now();
    if (last_frame_stamp_.nanoseconds() > 0) {
      const double dt =
        static_cast<double>((stamp - last_frame_stamp_).nanoseconds()) / 1.0e9;
      if (dt > 0.0) {
        const double instant_fps = 1.0 / dt;
        display_fps_ = display_fps_ <= 0.0
          ? instant_fps
          : (0.9 * display_fps_ + 0.1 * instant_fps);
      }
    }
    last_frame_stamp_ = stamp;
  }

  std::string image_topic_;
  std::string targets_topic_;
  std::string window_name_;
  bool show_keypoints_ = true;
  int target_timeout_ms_ = kTargetTimeoutMs;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr targets_sub_;

  std::mutex targets_mutex_;
  ai_msgs::msg::PerceptionTargets latest_targets_;
  rclcpp::Time latest_targets_stamp_{0, 0, RCL_ROS_TIME};
  rclcpp::Time last_frame_stamp_{0, 0, RCL_ROS_TIME};
  double display_fps_ = 0.0;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AutoAimVisualizerNode>());
  rclcpp::shutdown();
  return 0;
}
