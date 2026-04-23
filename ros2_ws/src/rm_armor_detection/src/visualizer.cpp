#include "ai_msgs/msg/perception_targets.hpp"
#include "cv_bridge/cv_bridge.h"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/image_encodings.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/string.hpp"

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
constexpr int kTargetTimeoutMs = 250;
constexpr double kFallbackRenderFps = 60.0;

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
    debug_topic_ = declare_parameter<std::string>("debug_topic", "/vehicle_detection/debug_text");
    window_name_ = declare_parameter<std::string>("window_name", kWindowName);
    show_labels_ = declare_parameter<bool>("show_labels", false);
    show_confidence_ = declare_parameter<bool>("show_confidence", true);
    show_keypoints_ = declare_parameter<bool>("show_keypoints", false);
    show_debug_text_ = declare_parameter<bool>("show_debug_text", false);
    target_timeout_ms_ = declare_parameter<int>("target_timeout_ms", kTargetTimeoutMs);
    display_max_fps_ = declare_parameter<double>("display_max_fps", 0.0);
    display_scale_ = declare_parameter<double>("display_scale", 1.0);
    fullscreen_ = declare_parameter<bool>("fullscreen", true);
    keep_aspect_ratio_ = declare_parameter<bool>("keep_aspect_ratio", true);
    window_width_ = declare_parameter<int>("window_width", 1280);
    window_height_ = declare_parameter<int>("window_height", 720);

    image_sub_ = create_subscription<sensor_msgs::msg::Image>(
      image_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&AutoAimVisualizerNode::OnImage, this, std::placeholders::_1));

    targets_sub_ = create_subscription<ai_msgs::msg::PerceptionTargets>(
      targets_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&AutoAimVisualizerNode::OnTargets, this, std::placeholders::_1));

    if (show_debug_text_) {
      debug_sub_ = create_subscription<std_msgs::msg::String>(
        debug_topic_,
        rclcpp::SystemDefaultsQoS(),
        std::bind(&AutoAimVisualizerNode::OnDebugText, this, std::placeholders::_1));
    }

    const int window_flags =
      cv::WINDOW_NORMAL | (keep_aspect_ratio_ ? cv::WINDOW_KEEPRATIO : cv::WINDOW_FREERATIO);
    cv::namedWindow(window_name_, window_flags);
    if (fullscreen_) {
      cv::setWindowProperty(window_name_, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    } else {
      cv::resizeWindow(window_name_, window_width_, window_height_);
    }

    const double render_fps =
      display_max_fps_ > 0.0 ? std::clamp(display_max_fps_, 1.0, 120.0) : kFallbackRenderFps;
    const auto render_period = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::duration<double>(1.0 / render_fps));
    render_timer_ = create_wall_timer(
      std::max(std::chrono::milliseconds(1), render_period),
      std::bind(&AutoAimVisualizerNode::RenderLatestFrame, this));

    RCLCPP_INFO(
      get_logger(),
      "Visualizer started. image_topic=%s targets_topic=%s window=%s fullscreen=%s keep_aspect=%s render_fps=%.1f",
      image_topic_.c_str(),
      targets_topic_.c_str(),
      window_name_.c_str(),
      fullscreen_ ? "true" : "false",
      keep_aspect_ratio_ ? "true" : "false",
      render_fps);
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

    std::lock_guard<std::mutex> lock(image_mutex_);
    latest_image_ = msg;
  }

  void RenderLatestFrame() {
    sensor_msgs::msg::Image::SharedPtr image;
    {
      std::lock_guard<std::mutex> lock(image_mutex_);
      image = latest_image_;
    }
    if (!image) {
      return;
    }

    cv_bridge::CvImageConstPtr cv_ptr;
    try {
      cv_ptr = cv_bridge::toCvShare(image, image->encoding);
    } catch (const cv_bridge::Exception &e) {
      RCLCPP_ERROR_THROTTLE(
        get_logger(), *get_clock(), 2000, "cv_bridge conversion failed: %s", e.what());
      return;
    }

    cv::Mat frame = cv_ptr->image.clone();
    if (frame.empty()) {
      return;
    }

    if (image->encoding == sensor_msgs::image_encodings::RGB8) {
      cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
    }

    const double render_scale = std::clamp(display_scale_, 0.1, 1.0);
    if (render_scale < 0.999) {
      cv::resize(frame, frame, cv::Size(), render_scale, render_scale, cv::INTER_LINEAR);
    }

    ai_msgs::msg::PerceptionTargets targets_copy;
    bool targets_fresh = false;
    std::string debug_text_copy;
    {
      std::lock_guard<std::mutex> lock(targets_mutex_);
      targets_copy = latest_targets_;
      debug_text_copy = latest_debug_text_;
      targets_fresh =
        (latest_targets_stamp_.nanoseconds() > 0) &&
        ((now() - latest_targets_stamp_).nanoseconds() <=
         static_cast<int64_t>(target_timeout_ms_) * 1000 * 1000);
    }

    if (targets_fresh) {
      DrawTargets(frame, targets_copy, render_scale);
    }

    DrawStatus(frame, targets_copy, targets_fresh);
    if (show_debug_text_) {
      DrawDebugText(frame, debug_text_copy);
    }
    cv::imshow(window_name_, frame);
    ApplyWindowLayout();
    cv::waitKey(1);
  }

  void ApplyWindowLayout() {
    if (window_layout_attempts_remaining_ <= 0) {
      return;
    }

    if (fullscreen_) {
      cv::setWindowProperty(window_name_, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    } else {
      cv::resizeWindow(window_name_, std::max(1, window_width_), std::max(1, window_height_));
      cv::moveWindow(window_name_, 0, 0);
    }
    --window_layout_attempts_remaining_;
  }

  void OnDebugText(const std_msgs::msg::String::SharedPtr msg) {
    if (!msg) {
      return;
    }

    std::lock_guard<std::mutex> lock(targets_mutex_);
    latest_debug_text_ = msg->data;
  }

  void DrawTargets(
    cv::Mat &frame,
    const ai_msgs::msg::PerceptionTargets &targets_msg,
    double render_scale) {
    for (std::size_t i = 0; i < targets_msg.targets.size(); ++i) {
      const auto &target = targets_msg.targets[i];
      if (target.rois.empty()) {
        continue;
      }

      const auto &roi = target.rois.front();
      const auto color = ColorForIndex(i);

      const int x = std::max<int>(
        0, static_cast<int>(std::lround(static_cast<double>(roi.rect.x_offset) * render_scale)));
      const int y = std::max<int>(
        0, static_cast<int>(std::lround(static_cast<double>(roi.rect.y_offset) * render_scale)));
      const int w = std::max<int>(
        0, static_cast<int>(std::lround(static_cast<double>(roi.rect.width) * render_scale)));
      const int h = std::max<int>(
        0, static_cast<int>(std::lround(static_cast<double>(roi.rect.height) * render_scale)));
      if (w <= 0 || h <= 0) {
        continue;
      }

      cv::rectangle(frame, cv::Rect(x, y, w, h), color, kLineThickness);

      if (show_labels_ || show_confidence_) {
        const std::string label = show_labels_ ?
          target.type + " " + cv::format("%.2f", roi.confidence) :
          cv::format("%.2f", roi.confidence);
        const cv::Point label_pos(x, std::max(20, y - 6));
        cv::putText(
          frame,
          label,
          label_pos,
          kFontFace,
          kFontScale,
          cv::Scalar(0, 0, 0),
          kLineThickness + 1);
        cv::putText(
          frame,
          label,
          label_pos,
          kFontFace,
          kFontScale,
          color,
          kLineThickness);
      }

      if (!show_keypoints_) {
        continue;
      }
      for (const auto &point_group : target.points) {
        for (const auto &pt : point_group.point) {
          const int px = static_cast<int>(std::lround(static_cast<double>(pt.x) * render_scale));
          const int py = static_cast<int>(std::lround(static_cast<double>(pt.y) * render_scale));
          cv::circle(frame, cv::Point(px, py), 3, color, cv::FILLED);
        }
      }
    }
  }

  void DrawStatus(
    cv::Mat &frame,
    const ai_msgs::msg::PerceptionTargets &targets_msg,
    bool targets_fresh) {
    const bool has_target = targets_fresh && HasDrawableTarget(targets_msg);
    const std::string status = cv::format(
      "fps:%u target:%s",
      targets_msg.fps,
      has_target ? "YES" : "NO");
    cv::putText(
      frame,
      status,
      cv::Point(20, 30),
      kFontFace,
      0.8,
      cv::Scalar(0, 0, 0),
      3);
    cv::putText(
      frame,
      status,
      cv::Point(20, 30),
      kFontFace,
      0.8,
      cv::Scalar(0, 255, 255),
      2);
  }

  bool HasDrawableTarget(const ai_msgs::msg::PerceptionTargets &targets_msg) const {
    return std::any_of(
      targets_msg.targets.begin(),
      targets_msg.targets.end(),
      [](const auto &target) {
        if (target.rois.empty()) {
          return false;
        }
        const auto &rect = target.rois.front().rect;
        return rect.width > 0 && rect.height > 0;
      });
  }

  void DrawDebugText(cv::Mat &frame, const std::string &debug_text) {
    if (debug_text.empty()) {
      return;
    }

    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= debug_text.size()) {
      const auto end = debug_text.find('\n', start);
      if (end == std::string::npos) {
        lines.emplace_back(debug_text.substr(start));
        break;
      }
      lines.emplace_back(debug_text.substr(start, end - start));
      start = end + 1;
    }

    int max_width = 0;
    int total_height = 0;
    for (const auto &line : lines) {
      int baseline = 0;
      const auto size = cv::getTextSize(line, kFontFace, 0.90, 2, &baseline);
      max_width = std::max(max_width, size.width);
      total_height += size.height + 18;
    }

    const cv::Rect box(16, 44, std::min(frame.cols - 32, max_width + 24), total_height + 16);
    cv::rectangle(frame, box, cv::Scalar(20, 20, 20), cv::FILLED);
    cv::rectangle(frame, box, cv::Scalar(0, 180, 255), 1);

    int y = box.y + 34;
    for (const auto &line : lines) {
      cv::putText(
        frame,
        line,
        cv::Point(box.x + 12, y),
        kFontFace,
        0.90,
        cv::Scalar(255, 255, 255),
        2);
      y += 38;
    }
  }

  std::string image_topic_;
  std::string targets_topic_;
  std::string debug_topic_;
  std::string window_name_;
  bool show_labels_ = false;
  bool show_confidence_ = true;
  bool show_keypoints_ = false;
  bool show_debug_text_ = false;
  bool fullscreen_ = true;
  bool keep_aspect_ratio_ = true;
  int window_width_ = 1280;
  int window_height_ = 720;
  int window_layout_attempts_remaining_ = 30;
  int target_timeout_ms_ = kTargetTimeoutMs;
  double display_max_fps_ = 0.0;
  double display_scale_ = 1.0;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr targets_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr debug_sub_;
  rclcpp::TimerBase::SharedPtr render_timer_;

  std::mutex image_mutex_;
  sensor_msgs::msg::Image::SharedPtr latest_image_;
  std::mutex targets_mutex_;
  ai_msgs::msg::PerceptionTargets latest_targets_;
  std::string latest_debug_text_;
  rclcpp::Time latest_targets_stamp_{0, 0, RCL_ROS_TIME};
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AutoAimVisualizerNode>());
  rclcpp::shutdown();
  return 0;
}
