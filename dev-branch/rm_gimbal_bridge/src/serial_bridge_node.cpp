#include "ai_msgs/msg/perception_targets.hpp"
#include "rclcpp/rclcpp.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace {

struct TargetCandidate {
  std::string type;
  double center_x;
  double center_y;
  double confidence;
  double distance_to_center;
};

speed_t ToBaudRate(int baud_rate) {
  switch (baud_rate) {
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      return B921600;
  }
}

uint16_t ClampToUInt16(double value) {
  if (value < 0.0) {
    return 0;
  }
  if (value > static_cast<double>(std::numeric_limits<uint16_t>::max())) {
    return std::numeric_limits<uint16_t>::max();
  }
  return static_cast<uint16_t>(std::lround(value));
}

}  // namespace

class GimbalSerialBridgeNode : public rclcpp::Node {
 public:
  GimbalSerialBridgeNode() : Node("rm_gimbal_bridge") {
    input_topic_ = declare_parameter<std::string>("input_topic", "/dnn_node_sample");
    serial_port_ = declare_parameter<std::string>("serial_port", "/dev/ttyS1");
    baud_rate_ = declare_parameter<int>("baud_rate", 921600);
    image_width_ = declare_parameter<int>("image_width", 1280);
    image_height_ = declare_parameter<int>("image_height", 1024);
    image_center_x_ = declare_parameter<double>("image_center_x", image_width_ / 2.0);
    image_center_y_ = declare_parameter<double>("image_center_y", image_height_ / 2.0);
    min_confidence_ = declare_parameter<double>("min_confidence", 0.5);
    enemy_prefix_ = declare_parameter<std::string>("enemy_prefix", "");
    selection_mode_ = declare_parameter<std::string>("selection_mode", "closest");
    log_selected_target_ = declare_parameter<bool>("log_selected_target", true);

    if (!OpenSerialPort()) {
      RCLCPP_ERROR(get_logger(), "Serial port init failed: %s", serial_port_.c_str());
    }

    subscription_ = create_subscription<ai_msgs::msg::PerceptionTargets>(
      input_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&GimbalSerialBridgeNode::OnTargets, this, std::placeholders::_1));
  }

  ~GimbalSerialBridgeNode() override {
    if (serial_fd_ >= 0) {
      close(serial_fd_);
      serial_fd_ = -1;
    }
  }

 private:
  bool OpenSerialPort() {
    serial_fd_ = open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd_ < 0) {
      RCLCPP_ERROR(get_logger(), "Open serial port failed: %s", std::strerror(errno));
      return false;
    }

    termios tty {};
    if (tcgetattr(serial_fd_, &tty) != 0) {
      RCLCPP_ERROR(get_logger(), "Read serial attributes failed: %s", std::strerror(errno));
      close(serial_fd_);
      serial_fd_ = -1;
      return false;
    }

    cfsetospeed(&tty, ToBaudRate(baud_rate_));
    cfsetispeed(&tty, ToBaudRate(baud_rate_));

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
      RCLCPP_ERROR(get_logger(), "Apply serial attributes failed: %s", std::strerror(errno));
      close(serial_fd_);
      serial_fd_ = -1;
      return false;
    }

    return true;
  }

  void OnTargets(const ai_msgs::msg::PerceptionTargets::SharedPtr msg) {
    if (!msg || serial_fd_ < 0) {
      return;
    }

    const auto selected = SelectTarget(*msg);
    if (!selected.has_value()) {
      return;
    }

    const uint16_t x = ClampToUInt16(selected->center_x);
    const uint16_t y = ClampToUInt16(selected->center_y);
    SendFrame(x, y);

    if (log_selected_target_) {
      RCLCPP_INFO_THROTTLE(
        get_logger(),
        *get_clock(),
        500,
        "target=%s center=(%u,%u) conf=%.3f dist=%.2f",
        selected->type.c_str(),
        x,
        y,
        selected->confidence,
        selected->distance_to_center);
    }
  }

  std::optional<TargetCandidate> SelectTarget(const ai_msgs::msg::PerceptionTargets &msg) const {
    std::optional<TargetCandidate> best;

    for (const auto &target : msg.targets) {
      if (!enemy_prefix_.empty() && target.type.rfind(enemy_prefix_, 0) != 0) {
        continue;
      }
      if (target.rois.empty()) {
        continue;
      }

      const auto &roi = target.rois.front();
      if (roi.confidence < min_confidence_) {
        continue;
      }

      const double center_x = static_cast<double>(roi.rect.x_offset) +
                              static_cast<double>(roi.rect.width) / 2.0;
      const double center_y = static_cast<double>(roi.rect.y_offset) +
                              static_cast<double>(roi.rect.height) / 2.0;
      const double dx = center_x - image_center_x_;
      const double dy = center_y - image_center_y_;
      const double distance = std::hypot(dx, dy);

      TargetCandidate current {
        target.type,
        center_x,
        center_y,
        roi.confidence,
        distance,
      };

      if (!best.has_value()) {
        best = current;
        continue;
      }

      if (selection_mode_ == "highest_confidence") {
        if (current.confidence > best->confidence) {
          best = current;
        }
      } else {
        if (current.distance_to_center < best->distance_to_center) {
          best = current;
        }
      }
    }

    return best;
  }

  void SendFrame(uint16_t x, uint16_t y) {
    const std::array<uint8_t, 8> frame = {
      0xFA,
      0xFB,
      static_cast<uint8_t>(x & 0xFF),
      static_cast<uint8_t>((x >> 8) & 0xFF),
      static_cast<uint8_t>(y & 0xFF),
      static_cast<uint8_t>((y >> 8) & 0xFF),
      0xFC,
      0xFD,
    };

    const auto written = write(serial_fd_, frame.data(), frame.size());
    if (written != static_cast<ssize_t>(frame.size())) {
      RCLCPP_WARN_THROTTLE(
        get_logger(),
        *get_clock(),
        1000,
        "Serial write incomplete: %zd/%zu",
        written,
        frame.size());
    }
  }

  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr subscription_;
  std::string input_topic_;
  std::string serial_port_;
  std::string enemy_prefix_;
  std::string selection_mode_;
  int baud_rate_ = 921600;
  int image_width_ = 1280;
  int image_height_ = 1024;
  double image_center_x_ = 640.0;
  double image_center_y_ = 512.0;
  double min_confidence_ = 0.5;
  bool log_selected_target_ = true;
  int serial_fd_ = -1;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GimbalSerialBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
