// Generic USB/V4L2 camera node for platforms where vendor SDK is unavailable.

#include <camera_info_manager/camera_info_manager.hpp>
#include <image_transport/camera_publisher.hpp>
#include <image_transport/image_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <opencv2/opencv.hpp>

#include <memory>
#include <string>

namespace fyt::camera_driver {

class UsbCameraNode : public rclcpp::Node {
 public:
  explicit UsbCameraNode(const rclcpp::NodeOptions &options)
  : Node("usb_camera_driver", options) {
    device_ = declare_parameter<std::string>("device", "/dev/video0");
    frame_id_ = declare_parameter<std::string>("frame_id", "camera_optical_frame");
    width_ = declare_parameter<int>("width", 1280);
    height_ = declare_parameter<int>("height", 1024);
    fps_ = declare_parameter<int>("fps", 60);
    output_topic_ = declare_parameter<std::string>("output_topic", "image");
    camera_info_url_ = declare_parameter<std::string>(
      "camera_info_url",
      "package://rm_camera_driver/config/camera_info.yaml");

    cap_.open(device_, cv::CAP_V4L2);
    if (!cap_.isOpened()) {
      RCLCPP_FATAL(get_logger(), "Failed to open camera device: %s", device_.c_str());
      rclcpp::shutdown();
      return;
    }

    cap_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);
    cap_.set(cv::CAP_PROP_FPS, fps_);
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);

    camera_info_manager_ =
      std::make_unique<camera_info_manager::CameraInfoManager>(this, "usb_camera_driver");
    if (camera_info_manager_->validateURL(camera_info_url_)) {
      camera_info_manager_->loadCameraInfo(camera_info_url_);
      camera_info_ = camera_info_manager_->getCameraInfo();
    } else {
      camera_info_.width = width_;
      camera_info_.height = height_;
    }
    camera_info_.header.frame_id = frame_id_;

    camera_pub_ = image_transport::create_camera_publisher(this, output_topic_);

    const auto period = std::chrono::milliseconds(std::max(1, 1000 / std::max(1, fps_)));
    timer_ = create_wall_timer(period, std::bind(&UsbCameraNode::CaptureOnce, this));
  }

 private:
  void CaptureOnce() {
    cv::Mat frame;
    if (!cap_.read(frame) || frame.empty()) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000, "Failed to read frame from %s", device_.c_str());
      return;
    }

    if (frame.cols != width_ || frame.rows != height_) {
      cv::resize(frame, frame, cv::Size(width_, height_));
    }

    sensor_msgs::msg::Image msg;
    msg.header.stamp = now();
    msg.header.frame_id = frame_id_;
    msg.height = static_cast<uint32_t>(frame.rows);
    msg.width = static_cast<uint32_t>(frame.cols);
    msg.encoding = sensor_msgs::image_encodings::BGR8;
    msg.is_bigendian = false;
    msg.step = static_cast<sensor_msgs::msg::Image::_step_type>(frame.step);
    msg.data.assign(frame.data, frame.data + frame.total() * frame.elemSize());

    camera_info_.header.stamp = msg.header.stamp;
    camera_pub_.publish(msg, camera_info_);
  }

  cv::VideoCapture cap_;
  std::unique_ptr<camera_info_manager::CameraInfoManager> camera_info_manager_;
  image_transport::CameraPublisher camera_pub_;
  sensor_msgs::msg::CameraInfo camera_info_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::string device_;
  std::string frame_id_;
  std::string output_topic_;
  std::string camera_info_url_;
  int width_ = 1280;
  int height_ = 1024;
  int fps_ = 60;
};

}  // namespace fyt::camera_driver

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(fyt::camera_driver::UsbCameraNode)
