#include "ai_msgs/msg/perception_targets.hpp"
#include "rclcpp/rclcpp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace {

struct TargetCandidate {
  std::string type;
  double center_x;
  double center_y;
  double confidence;
  double distance_to_center;
  int x_offset;
  int y_offset;
  int width;
  int height;
  double area;
};

uint16_t ClampToUInt16(double value) {
  if (value < 0.0) {
    return 0;
  }
  if (value > static_cast<double>(std::numeric_limits<uint16_t>::max())) {
    return std::numeric_limits<uint16_t>::max();
  }
  return static_cast<uint16_t>(std::lround(value));
}

std::string FrameToHexString(const std::array<uint8_t, 8> &frame) {
  std::ostringstream oss;
  oss << std::hex << std::uppercase;
  for (std::size_t i = 0; i < frame.size(); ++i) {
    if (i != 0U) {
      oss << ' ';
    }
    oss.width(2);
    oss.fill('0');
    oss << static_cast<int>(frame[i]);
  }
  return oss.str();
}

}  // namespace

class GimbalTargetFeedbackNode : public rclcpp::Node {
 public:
  GimbalTargetFeedbackNode() : Node("rm_gimbal_feedback") {
    input_topic_ = declare_parameter<std::string>("input_topic", "/dnn_node_sample");
    image_width_ = declare_parameter<int>("image_width", 1280);
    image_height_ = declare_parameter<int>("image_height", 1024);
    image_center_x_ = declare_parameter<double>("image_center_x", image_width_ / 2.0);
    image_center_y_ = declare_parameter<double>("image_center_y", image_height_ / 2.0);
    min_confidence_ = declare_parameter<double>("min_confidence", 0.5);
    enemy_prefix_ = declare_parameter<std::string>("enemy_prefix", "");
    allowed_target_types_ =
      declare_parameter<std::vector<std::string>>("allowed_target_types", std::vector<std::string>{});
    selection_mode_ = declare_parameter<std::string>("selection_mode", "closest");
    log_when_empty_ = declare_parameter<bool>("log_when_empty", true);

    subscription_ = create_subscription<ai_msgs::msg::PerceptionTargets>(
      input_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&GimbalTargetFeedbackNode::OnTargets, this, std::placeholders::_1));
  }

 private:
  void OnTargets(const ai_msgs::msg::PerceptionTargets::SharedPtr msg) {
    if (!msg) {
      return;
    }

    const auto selected = SelectTarget(*msg);
    if (!selected.has_value()) {
      if (log_when_empty_) {
        RCLCPP_INFO_THROTTLE(
          get_logger(),
          *get_clock(),
          1000,
          "no target selected: total_targets=%zu min_confidence=%.2f enemy_prefix=%s",
          msg->targets.size(),
          min_confidence_,
          enemy_prefix_.empty() ? "<none>" : enemy_prefix_.c_str());
      }
      return;
    }

    const uint16_t x = ClampToUInt16(selected->center_x);
    const uint16_t y = ClampToUInt16(selected->center_y);
    const double dx = selected->center_x - image_center_x_;
    const double dy = selected->center_y - image_center_y_;
    const double norm_dx = image_width_ > 0 ? dx / (static_cast<double>(image_width_) * 0.5) : 0.0;
    const double norm_dy = image_height_ > 0 ? dy / (static_cast<double>(image_height_) * 0.5) : 0.0;
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

    RCLCPP_INFO_THROTTLE(
      get_logger(),
      *get_clock(),
      200,
      "target=%s conf=%.3f roi=(x=%d y=%d w=%d h=%d) center=(%u,%u) offset=(%.1f,%.1f) norm=(%.3f,%.3f) dist=%.2f frame=[%s]",
      selected->type.c_str(),
      selected->confidence,
      selected->x_offset,
      selected->y_offset,
      selected->width,
      selected->height,
      x,
      y,
      dx,
      dy,
      norm_dx,
      norm_dy,
      selected->distance_to_center,
      FrameToHexString(frame).c_str());
  }

  std::optional<TargetCandidate> SelectTarget(const ai_msgs::msg::PerceptionTargets &msg) const {
    std::optional<TargetCandidate> best;

    for (const auto &target : msg.targets) {
      if (!enemy_prefix_.empty() && target.type.rfind(enemy_prefix_, 0) != 0) {
        continue;
      }
      if (!IsTargetTypeAllowed(target.type)) {
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
      const double area = static_cast<double>(roi.rect.width) * static_cast<double>(roi.rect.height);

      TargetCandidate current {
        target.type,
        center_x,
        center_y,
        roi.confidence,
        distance,
        static_cast<int>(roi.rect.x_offset),
        static_cast<int>(roi.rect.y_offset),
        static_cast<int>(roi.rect.width),
        static_cast<int>(roi.rect.height),
        area,
      };

      if (!best.has_value()) {
        best = current;
        continue;
      }

      if (selection_mode_ == "highest_confidence") {
        if (current.confidence > best->confidence) {
          best = current;
        }
      } else if (selection_mode_ == "largest_box") {
        if (current.area > best->area) {
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

  bool IsTargetTypeAllowed(const std::string &type) const {
    if (allowed_target_types_.empty()) {
      return true;
    }
    for (const auto &allowed_type : allowed_target_types_) {
      if (allowed_type == type) {
        return true;
      }
    }
    return false;
  }

  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr subscription_;
  std::string input_topic_;
  std::string enemy_prefix_;
  std::vector<std::string> allowed_target_types_;
  std::string selection_mode_;
  int image_width_ = 1280;
  int image_height_ = 1024;
  double image_center_x_ = 640.0;
  double image_center_y_ = 512.0;
  double min_confidence_ = 0.5;
  bool log_when_empty_ = true;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GimbalTargetFeedbackNode>());
  rclcpp::shutdown();
  return 0;
}
