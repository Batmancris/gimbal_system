#include "ai_msgs/msg/perception_targets.hpp"
#include "rclcpp/rclcpp.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
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
#include <vector>

namespace {

constexpr uint8_t kVisionDiagHead0 = 0xD1;
constexpr uint8_t kVisionDiagHead1 = 0x5B;
constexpr uint8_t kVisionDiagTail0 = 0x6B;
constexpr uint8_t kVisionDiagTail1 = 0x1D;
constexpr size_t kVisionDiagFrameSize = 48;
constexpr uint8_t kVisionDiagFlagVisionEnabled = 0x01;
constexpr uint8_t kVisionDiagFlagTargetValid = 0x02;
constexpr uint8_t kVisionDiagFlagLinkOnline = 0x04;
constexpr uint8_t kVisionDiagFlagRcError = 0x08;
constexpr uint8_t kVisionDiagFlagDbusToe = 0x10;

struct TargetCandidate {
  std::string type;
  double center_x;
  double center_y;
  double confidence;
  double distance_to_center;
  double area;
};

struct VisionDiagFrame {
  uint8_t flags;
  uint8_t seq;
  uint16_t raw_x;
  uint16_t raw_y;
  int16_t error_x;
  int16_t error_y;
  int16_t yaw_add_mrad;
  int16_t pitch_add_mrad;
  uint16_t parsed_frames;
  uint16_t rx_bytes;
  uint8_t rc_sw0;
  uint8_t rc_sw1;
  int16_t rc_ch0;
  int16_t rc_ch1;
  int16_t rc_ch2;
  int16_t rc_ch3;
  uint8_t behaviour;
  int16_t manual_yaw_add_mrad;
  int16_t manual_pitch_add_mrad;
  uint8_t yaw_mode;
  uint8_t pitch_mode;
  int16_t yaw_set_mrad;
  int16_t pitch_set_mrad;
  int16_t yaw_given_current;
  int16_t pitch_given_current;
};

enum class FollowControlMode {
  kLightPredict,
  kPidPredict,
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

double DistanceBetween(const TargetCandidate &lhs, const TargetCandidate &rhs) {
  return std::hypot(lhs.center_x - rhs.center_x, lhs.center_y - rhs.center_y);
}

double ClampMagnitude(const double delta, const double limit) {
  if (limit <= 0.0) {
    return delta;
  }
  if (delta > limit) {
    return limit;
  }
  if (delta < -limit) {
    return -limit;
  }
  return delta;
}

double ClampAbs(const double value, const double limit) {
  return ClampMagnitude(value, limit);
}

double ComputeInterpolationAlpha(const double dt, const double rate_hz) {
  if (rate_hz <= 0.0) {
    return 1.0;
  }
  return std::clamp(dt * rate_hz, 0.05, 1.0);
}

uint16_t ReadLe16(const uint8_t *data) {
  return static_cast<uint16_t>(static_cast<uint16_t>(data[0]) |
                               (static_cast<uint16_t>(data[1]) << 8));
}

int16_t ReadLeI16(const uint8_t *data) {
  return static_cast<int16_t>(ReadLe16(data));
}

uint8_t VisionDiagChecksum(const uint8_t *frame) {
  uint8_t checksum = 0;
  for (size_t i = 2; i <= 44; ++i) {
    checksum ^= frame[i];
  }
  return checksum;
}

}  // namespace

class GimbalSerialBridgeNode : public rclcpp::Node {
 public:
  GimbalSerialBridgeNode() : Node("rm_gimbal_bridge") {
    input_topic_ = declare_parameter<std::string>("input_topic", "/vehicle_detection/targets");
    serial_port_ = declare_parameter<std::string>("serial_port", "/dev/ttyS1");
    baud_rate_ = declare_parameter<int>("baud_rate", 921600);
    image_width_ = declare_parameter<int>("image_width", 1280);
    image_height_ = declare_parameter<int>("image_height", 1024);
    image_center_x_ = declare_parameter<double>("image_center_x", image_width_ / 2.0);
    image_center_y_ = declare_parameter<double>("image_center_y", image_height_ / 2.0);
    min_confidence_ = declare_parameter<double>("min_confidence", 0.5);
    enemy_prefix_ = declare_parameter<std::string>("enemy_prefix", "");
    allowed_target_types_ =
      declare_parameter<std::vector<std::string>>(
      "allowed_target_types", std::vector<std::string>{"vehicle"});
    selection_mode_ = declare_parameter<std::string>("selection_mode", "closest");
    log_selected_target_ = declare_parameter<bool>("log_selected_target", false);
    log_diag_feedback_ = declare_parameter<bool>("log_diag_feedback", false);
    require_lower_vision_enabled_ =
      declare_parameter<bool>("require_lower_vision_enabled", true);
    enable_fixed_rate_follow_ = declare_parameter<bool>("enable_fixed_rate_follow", true);
    follow_send_rate_hz_ = declare_parameter<double>("follow_send_rate_hz", 50.0);
    follow_smoothing_alpha_ = declare_parameter<double>("follow_smoothing_alpha", 0.35);
    follow_interp_rate_hz_ = declare_parameter<double>("follow_interp_rate_hz", 10.0);
    follow_control_mode_name_ =
      declare_parameter<std::string>("follow_control_mode", "light_predict");
    follow_max_step_px_ = declare_parameter<double>("follow_max_step_px", 36.0);
    follow_deadband_px_ = declare_parameter<double>("follow_deadband_px", 12.0);
    measurement_jitter_deadband_px_ =
      declare_parameter<double>("measurement_jitter_deadband_px", 28.0);
    fast_follow_error_px_ = declare_parameter<double>("fast_follow_error_px", 120.0);
    fast_follow_smoothing_alpha_ =
      declare_parameter<double>("fast_follow_smoothing_alpha", 0.55);
    fast_follow_max_step_px_ = declare_parameter<double>("fast_follow_max_step_px", 72.0);
    light_follow_gain_ = declare_parameter<double>("light_follow_gain", 0.45);
    center_gate_x_ratio_ = declare_parameter<double>("center_gate_x_ratio", 0.35);
    center_gate_y_ratio_ = declare_parameter<double>("center_gate_y_ratio", 0.30);
    pid_kp_ = declare_parameter<double>("pid_kp", 0.55);
    pid_ki_ = declare_parameter<double>("pid_ki", 0.0);
    pid_kd_ = declare_parameter<double>("pid_kd", 0.08);
    pid_integral_limit_px_ = declare_parameter<double>("pid_integral_limit_px", 160.0);
    predict_alpha_ = declare_parameter<double>("predict_alpha", 0.65);
    predict_beta_ = declare_parameter<double>("predict_beta", 0.0);
    predict_horizon_sec_ = declare_parameter<double>("predict_horizon_sec", 0.0);
    target_switch_radius_px_ = declare_parameter<double>("target_switch_radius_px", 120.0);
    target_switch_min_conf_gain_ =
      declare_parameter<double>("target_switch_min_conf_gain", 0.10);
    target_switch_center_gain_px_ =
      declare_parameter<double>("target_switch_center_gain_px", 60.0);
    min_send_delta_px_ = declare_parameter<double>("min_send_delta_px", 2.0);
    send_keepalive_ms_ = declare_parameter<int>("send_keepalive_ms", 40);
    lower_diag_timeout_ms_ = declare_parameter<int>("lower_diag_timeout_ms", 500);
    lower_vision_latch_ms_ = declare_parameter<int>("lower_vision_latch_ms", 5000);
    target_hold_ms_ = declare_parameter<int>("target_hold_ms", 300);
    follow_control_mode_ = ParseFollowControlMode(follow_control_mode_name_);

    if (!OpenSerialPort()) {
      RCLCPP_ERROR(get_logger(), "Serial port init failed: %s", serial_port_.c_str());
    }

    subscription_ = create_subscription<ai_msgs::msg::PerceptionTargets>(
      input_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&GimbalSerialBridgeNode::OnTargets, this, std::placeholders::_1));

    diag_timer_ = create_wall_timer(
      std::chrono::milliseconds(20),
      std::bind(&GimbalSerialBridgeNode::PollDiagnostics, this));
    if (enable_fixed_rate_follow_) {
      const double send_rate = std::clamp(follow_send_rate_hz_, 1.0, 200.0);
      const auto send_period = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double>(1.0 / send_rate));
      follow_send_timer_ = create_wall_timer(
        std::max(std::chrono::milliseconds(1), send_period),
        std::bind(&GimbalSerialBridgeNode::SendFollowFrameOnTimer, this));
      RCLCPP_INFO(get_logger(), "fixed-rate follow enabled at %.1f Hz", send_rate);
    }
  }

  ~GimbalSerialBridgeNode() override {
    if (serial_fd_ >= 0) {
      close(serial_fd_);
      serial_fd_ = -1;
    }
  }

 private:
  bool OpenSerialPort() {
    if (serial_fd_ >= 0) {
      close(serial_fd_);
      serial_fd_ = -1;
    }

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

  bool ReopenSerialPort() {
    RCLCPP_WARN(get_logger(), "Reopening serial port: %s", serial_port_.c_str());
    return OpenSerialPort();
  }

  bool LowerVisionControlAllowed() const {
    if (!require_lower_vision_enabled_) {
      return true;
    }
    return lower_vision_latched_;
  }

  static FollowControlMode ParseFollowControlMode(const std::string &value) {
    if (value == "pid_predict") {
      return FollowControlMode::kPidPredict;
    }
    return FollowControlMode::kLightPredict;
  }

  void OnTargets(const ai_msgs::msg::PerceptionTargets::SharedPtr msg) {
    if (!msg) {
      return;
    }

    const auto stamp = now();
    // 先选出一个连续稳定的目标，再更新控制器状态。目标连续性断开时，
    // SelectTarget 会返回空，下面会给下位机发送中心点刹车帧，而不是追另一个目标。
    auto selected = SelectTarget(*msg, stamp);

    if (selected.has_value()) {
      last_target_ = selected;
      last_target_stamp_ = stamp;
    }

    if (!selected.has_value()) {
      if (has_active_target_ || filter_initialized_ || predictor_initialized_) {
        ResetTrackingState();
      }
      has_active_target_ = false;
      SendNeutralFrameIfAllowed(stamp);
      return;
    }

    has_active_target_ = true;
    UpdatePredictorFromMeasurement(*selected, stamp);
    if (enable_fixed_rate_follow_) {
      return;
    }

    SendCurrentFollowFrame(stamp);
  }

  void SendFollowFrameOnTimer() {
    SendCurrentFollowFrame(now());
  }

  bool SendCurrentFollowFrame(const rclcpp::Time &stamp) {
    if (!LowerVisionControlAllowed()) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 1000,
        "feedback-only: lower vision not enabled yet, skip follow send");
      return false;
    }

    if (!has_active_target_ || !last_target_.has_value() || last_target_stamp_.nanoseconds() <= 0) {
      SendNeutralFrameIfAllowed(stamp);
      return false;
    }

    const auto target_age = stamp - last_target_stamp_;
    if (target_age > rclcpp::Duration::from_seconds(static_cast<double>(target_hold_ms_) / 1000.0)) {
      has_active_target_ = false;
      ResetTrackingState(true);
      SendNeutralFrameIfAllowed(stamp);
      return false;
    }

    const auto filtered = ComputeFollowCenter(stamp);
    const uint16_t x = ClampToUInt16(filtered.first);
    const uint16_t y = ClampToUInt16(filtered.second);

    if (!ShouldSendFrame(x, y, stamp)) {
      return false;
    }

    SendFrame(x, y);
    last_sent_x_ = x;
    last_sent_y_ = y;
    last_send_stamp_ = stamp;
    has_last_sent_frame_ = true;

    if (log_selected_target_) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 500,
        "target=%s center=(%u,%u) raw=(%.1f,%.1f) vel=(%.1f,%.1f) conf=%.3f dist=%.2f",
        last_target_->type.c_str(), x, y, last_target_->center_x, last_target_->center_y,
        predictor_vx_, predictor_vy_, last_target_->confidence, last_target_->distance_to_center);
    }
    return true;
  }

  void UpdatePredictorFromMeasurement(
    const TargetCandidate &target,
    const rclcpp::Time &stamp) {
    // 当前只保留轻量预测：低速已经验证顺滑，所以这里主要做 alpha 平滑，
    // 速度预测保持保守，避免还没验证的高速预测破坏低速手感。
    const double measurement_x =
      std::clamp(target.center_x, 0.0, static_cast<double>(image_width_));
    const double measurement_y =
      std::clamp(target.center_y, 0.0, static_cast<double>(image_height_));

    if (!predictor_initialized_) {
      predictor_x_ = measurement_x;
      predictor_y_ = measurement_y;
      predictor_vx_ = 0.0;
      predictor_vy_ = 0.0;
      predictor_stamp_ = stamp;
      predictor_initialized_ = true;
      filtered_center_x_ = measurement_x;
      filtered_center_y_ = measurement_y;
      filter_initialized_ = true;
      last_control_stamp_ = stamp;
      return;
    }

    double dt = 0.0;
    if (predictor_stamp_.nanoseconds() > 0) {
      dt = (stamp - predictor_stamp_).seconds();
    }
    dt = std::clamp(dt, 0.005, 0.2);

    const double predicted_x = predictor_x_ + predictor_vx_ * dt;
    const double predicted_y = predictor_y_ + predictor_vy_ * dt;
    const double residual_x = measurement_x - predicted_x;
    const double residual_y = measurement_y - predicted_y;
    const double alpha = std::clamp(predict_alpha_, 0.0, 1.0);
    const double beta = std::clamp(predict_beta_, 0.0, 1.0);

    predictor_x_ = predicted_x + alpha * residual_x;
    predictor_y_ = predicted_y + alpha * residual_y;
    predictor_vx_ += (beta * residual_x) / dt;
    predictor_vy_ += (beta * residual_y) / dt;
    predictor_stamp_ = stamp;
  }

  std::pair<double, double> ComputeFollowCenter(const rclcpp::Time &stamp) {
    if (!predictor_initialized_) {
      return {image_center_x_, image_center_y_};
    }

    // 把测量目标转换成发给 STM32 的目标点。逐帧限幅让低速更丝滑，
    // 但高速横移时也可能带来滞后，后续高速调参主要看这里和下位机限幅。
    double prediction_dt = 0.0;
    if (predictor_stamp_.nanoseconds() > 0) {
      prediction_dt = (stamp - predictor_stamp_).seconds();
    }
    prediction_dt = std::clamp(prediction_dt, 0.0, 0.25);

    const double target_x = std::clamp(
      predictor_x_ + predictor_vx_ * (prediction_dt + std::max(0.0, predict_horizon_sec_)),
      0.0, static_cast<double>(image_width_));
    const double target_y = std::clamp(
      predictor_y_ + predictor_vy_ * (prediction_dt + std::max(0.0, predict_horizon_sec_)),
      0.0, static_cast<double>(image_height_));
    const double dx_center = target_x - image_center_x_;
    const double dy_center = target_y - image_center_y_;
    if (std::hypot(dx_center, dy_center) <= follow_deadband_px_) {
      ResetControllerState();
      filtered_center_x_ = image_center_x_;
      filtered_center_y_ = image_center_y_;
      filter_initialized_ = true;
      last_control_stamp_ = stamp;
      return {filtered_center_x_, filtered_center_y_};
    }

    if (!filter_initialized_) {
      filtered_center_x_ = target_x;
      filtered_center_y_ = target_y;
      filter_initialized_ = true;
      last_control_stamp_ = stamp;
      return {filtered_center_x_, filtered_center_y_};
    }

    double dt = 0.0;
    if (last_control_stamp_.nanoseconds() > 0) {
      dt = (stamp - last_control_stamp_).seconds();
    }
    dt = std::clamp(dt, 0.005, 0.1);
    last_control_stamp_ = stamp;

    const double interp_alpha = ComputeInterpolationAlpha(dt, follow_interp_rate_hz_);
    if (follow_control_mode_ == FollowControlMode::kPidPredict) {
      const double alpha = std::clamp(follow_smoothing_alpha_, 0.0, 1.0);
      const double interpolated_x =
        filtered_center_x_ + (target_x - filtered_center_x_) * interp_alpha;
      const double interpolated_y =
        filtered_center_y_ + (target_y - filtered_center_y_) * interp_alpha;
      const double desired_x =
        filtered_center_x_ + (interpolated_x - filtered_center_x_) * alpha;
      const double desired_y =
        filtered_center_y_ + (interpolated_y - filtered_center_y_) * alpha;

      const double error_x = desired_x - filtered_center_x_;
      const double error_y = desired_y - filtered_center_y_;
      pid_integral_x_ = ClampAbs(pid_integral_x_ + error_x * dt, pid_integral_limit_px_);
      pid_integral_y_ = ClampAbs(pid_integral_y_ + error_y * dt, pid_integral_limit_px_);
      const double derivative_x = (error_x - pid_prev_error_x_) / dt;
      const double derivative_y = (error_y - pid_prev_error_y_) / dt;
      pid_prev_error_x_ = error_x;
      pid_prev_error_y_ = error_y;

      const double pid_step_x =
        pid_kp_ * error_x + pid_ki_ * pid_integral_x_ + pid_kd_ * derivative_x;
      const double pid_step_y =
        pid_kp_ * error_y + pid_ki_ * pid_integral_y_ + pid_kd_ * derivative_y;

      filtered_center_x_ += ClampMagnitude(pid_step_x, follow_max_step_px_);
      filtered_center_y_ += ClampMagnitude(pid_step_y, follow_max_step_px_);
    } else {
      ResetControllerState();
      const double target_error = std::hypot(dx_center, dy_center);
      const bool fast_follow = target_error >= fast_follow_error_px_;
      const double alpha = std::clamp(
        fast_follow ? fast_follow_smoothing_alpha_ : follow_smoothing_alpha_, 0.0, 1.0);
      const double gain = std::clamp(light_follow_gain_, 0.0, 1.0);
      const double max_step =
        fast_follow ? std::max(fast_follow_max_step_px_, follow_max_step_px_) : follow_max_step_px_;
      const double interpolated_x =
        filtered_center_x_ + (target_x - filtered_center_x_) * interp_alpha;
      const double interpolated_y =
        filtered_center_y_ + (target_y - filtered_center_y_) * interp_alpha;
      const double desired_x =
        filtered_center_x_ + (interpolated_x - filtered_center_x_) * gain;
      const double desired_y =
        filtered_center_y_ + (interpolated_y - filtered_center_y_) * gain;
      const double limited_step_x = ClampMagnitude(
        desired_x - filtered_center_x_, max_step);
      const double limited_step_y = ClampMagnitude(
        desired_y - filtered_center_y_, max_step);
      filtered_center_x_ += limited_step_x * std::max(alpha, 0.05);
      filtered_center_y_ += limited_step_y * std::max(alpha, 0.05);
    }

    filtered_center_x_ = std::clamp(filtered_center_x_, 0.0, static_cast<double>(image_width_));
    filtered_center_y_ = std::clamp(filtered_center_y_, 0.0, static_cast<double>(image_height_));
    return {filtered_center_x_, filtered_center_y_};
  }

  bool ShouldSendFrame(uint16_t x, uint16_t y, const rclcpp::Time &stamp) const {
    if (!has_last_sent_frame_) {
      return true;
    }

    const double dx = std::abs(static_cast<double>(x) - static_cast<double>(last_sent_x_));
    const double dy = std::abs(static_cast<double>(y) - static_cast<double>(last_sent_y_));
    if (dx >= min_send_delta_px_ || dy >= min_send_delta_px_) {
      return true;
    }

    if (last_send_stamp_.nanoseconds() <= 0) {
      return true;
    }

    const auto age = stamp - last_send_stamp_;
    return age >= rclcpp::Duration::from_seconds(static_cast<double>(send_keepalive_ms_) / 1000.0);
  }

  void ResetControllerState() {
    pid_integral_x_ = 0.0;
    pid_integral_y_ = 0.0;
    pid_prev_error_x_ = 0.0;
    pid_prev_error_y_ = 0.0;
  }

  void ResetTrackingState(bool keep_last_target = false) {
    has_active_target_ = false;
    filter_initialized_ = false;
    predictor_initialized_ = false;
    filtered_center_x_ = image_center_x_;
    filtered_center_y_ = image_center_y_;
    predictor_x_ = image_center_x_;
    predictor_y_ = image_center_y_;
    predictor_vx_ = 0.0;
    predictor_vy_ = 0.0;
    last_control_stamp_ = rclcpp::Time(0, 0, get_clock()->get_clock_type());
    predictor_stamp_ = rclcpp::Time(0, 0, get_clock()->get_clock_type());
    ResetControllerState();
    if (!keep_last_target) {
      last_target_.reset();
    }
    has_last_sent_frame_ = false;
  }

  void SendNeutralFrame() {
    SendFrame(
      ClampToUInt16(image_center_x_),
      ClampToUInt16(image_center_y_));
  }

  void SendNeutralFrameIfAllowed(const rclcpp::Time &stamp) {
    if (!LowerVisionControlAllowed()) {
      return;
    }

    // 中心点不是随便发的空值，而是丢目标或目标跳变时的刹车命令，
    // 防止下位机继续按旧坐标转动。
    const uint16_t x = ClampToUInt16(image_center_x_);
    const uint16_t y = ClampToUInt16(image_center_y_);
    if (!ShouldSendFrame(x, y, stamp)) {
      return;
    }

    SendFrame(x, y);
    last_sent_x_ = x;
    last_sent_y_ = y;
    last_send_stamp_ = stamp;
    has_last_sent_frame_ = true;
  }

  void PollDiagnostics() {
    std::array<uint8_t, 256> buffer {};
    ssize_t count;
    if (serial_fd_ < 0) {
      return;
    }

    do {
      count = read(serial_fd_, buffer.data(), buffer.size());
      if (count > 0) {
        diag_rx_buffer_.insert(diag_rx_buffer_.end(), buffer.begin(), buffer.begin() + count);
      }
    } while (count > 0);

    ParseDiagnostics();
  }

  void ParseDiagnostics() {
    while (diag_rx_buffer_.size() >= kVisionDiagFrameSize) {
      if (diag_rx_buffer_[0] != kVisionDiagHead0 || diag_rx_buffer_[1] != kVisionDiagHead1) {
        diag_rx_buffer_.erase(diag_rx_buffer_.begin());
        continue;
      }

      const auto *frame = diag_rx_buffer_.data();
      if (frame[46] != kVisionDiagTail0 || frame[47] != kVisionDiagTail1 ||
          VisionDiagChecksum(frame) != frame[45]) {
        diag_rx_buffer_.erase(diag_rx_buffer_.begin());
        continue;
      }

      const VisionDiagFrame diag {
        frame[2],
        frame[3],
        ReadLe16(&frame[4]),
        ReadLe16(&frame[6]),
        ReadLeI16(&frame[8]),
        ReadLeI16(&frame[10]),
        ReadLeI16(&frame[12]),
        ReadLeI16(&frame[14]),
        ReadLe16(&frame[16]),
        ReadLe16(&frame[18]),
        frame[20],
        frame[21],
        ReadLeI16(&frame[22]),
        ReadLeI16(&frame[24]),
        ReadLeI16(&frame[26]),
        ReadLeI16(&frame[28]),
        frame[30],
        ReadLeI16(&frame[31]),
        ReadLeI16(&frame[33]),
        frame[35],
        frame[36],
        ReadLeI16(&frame[37]),
        ReadLeI16(&frame[39]),
        ReadLeI16(&frame[41]),
        ReadLeI16(&frame[43]),
      };

      last_diag_ = diag;
      last_diag_stamp_ = now();

      // 下位机拥有最终视觉使能权。它还没回报 vision enabled 前，
      // 桥接可以只收诊断不发控制，避免调试启动时云台误动。
      const bool vision_enabled = (diag.flags & kVisionDiagFlagVisionEnabled) != 0;
      if (vision_enabled != lower_vision_enabled_) {
        lower_vision_enabled_ = vision_enabled;
        ResetTrackingState();
        if (!vision_enabled) {
          SendNeutralFrame();
        }
        RCLCPP_INFO(
          get_logger(),
          "lower vision enable changed: %d",
          vision_enabled);
      }
      if (vision_enabled) {
        lower_vision_latched_ = true;
        lower_vision_latched_stamp_ = last_diag_stamp_;
      } else {
        lower_vision_latched_ = false;
      }

      if (log_diag_feedback_) {
        const bool target_valid = (diag.flags & kVisionDiagFlagTargetValid) != 0;
        const bool link_online = (diag.flags & kVisionDiagFlagLinkOnline) != 0;
        const bool rc_error = (diag.flags & kVisionDiagFlagRcError) != 0;
        const bool dbus_toe = (diag.flags & kVisionDiagFlagDbusToe) != 0;
        RCLCPP_INFO_THROTTLE(
          get_logger(), *get_clock(), 200,
          "diag vision=%d latched=%d target=%d link=%d rc_error=%d dbus_toe=%d behaviour=%u sw=(%u,%u) ch=(%d,%d,%d,%d) seq=%u raw=(%u,%u) err=(%d,%d) vision_add=(%d,%d) manual_add=(%d,%d) mode=(%u,%u) set=(%d,%d) current=(%d,%d) parsed=%u rx=%u",
          vision_enabled, lower_vision_latched_, target_valid, link_online, rc_error, dbus_toe, diag.behaviour,
          diag.rc_sw0, diag.rc_sw1, diag.rc_ch0, diag.rc_ch1, diag.rc_ch2, diag.rc_ch3,
          diag.seq, diag.raw_x, diag.raw_y, diag.error_x, diag.error_y, diag.yaw_add_mrad, diag.pitch_add_mrad,
          diag.manual_yaw_add_mrad, diag.manual_pitch_add_mrad,
          diag.yaw_mode, diag.pitch_mode, diag.yaw_set_mrad, diag.pitch_set_mrad,
          diag.yaw_given_current, diag.pitch_given_current,
          diag.parsed_frames, diag.rx_bytes);

        if (vision_enabled && target_valid) {
          RCLCPP_INFO_THROTTLE(
            get_logger(), *get_clock(), 200,
            "tune err=(%d,%d) add=(%d,%d)mrad pitch_set=%dmrad current=(%d,%d) seq=%u",
            diag.error_x, diag.error_y, diag.yaw_add_mrad, diag.pitch_add_mrad,
            diag.pitch_set_mrad, diag.yaw_given_current, diag.pitch_given_current, diag.seq);
        }
      }

      diag_rx_buffer_.erase(diag_rx_buffer_.begin(), diag_rx_buffer_.begin() + kVisionDiagFrameSize);
    }
  }

  std::optional<TargetCandidate> SelectBestCandidate(
    const std::vector<TargetCandidate> &candidates) const {
    std::optional<TargetCandidate> best;
    for (const auto &current : candidates) {
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
      } else if (current.distance_to_center < best->distance_to_center) {
        best = current;
      }
    }
    return best;
  }

  bool ShouldSwitchTarget(
    const TargetCandidate &sticky_target,
    const TargetCandidate &challenger) const {
    const bool confidence_clear =
      challenger.confidence >= sticky_target.confidence + target_switch_min_conf_gain_;
    const bool center_clear =
      challenger.distance_to_center + target_switch_center_gain_px_ <
      sticky_target.distance_to_center;
    return confidence_clear && center_clear;
  }

  std::optional<TargetCandidate> SelectTarget(
    const ai_msgs::msg::PerceptionTargets &msg,
    const rclcpp::Time &stamp) const {
    std::vector<TargetCandidate> candidates;
    candidates.reserve(msg.targets.size());
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
      const double gate_x = static_cast<double>(image_width_) * 0.5 * center_gate_x_ratio_;
      const double gate_y = static_cast<double>(image_height_) * 0.5 * center_gate_y_ratio_;
      if (std::abs(dx) > gate_x || std::abs(dy) > gate_y) {
        continue;
      }
      const double distance = std::hypot(dx, dy);
      const double area = static_cast<double>(roi.rect.width) * static_cast<double>(roi.rect.height);

      candidates.push_back(TargetCandidate {target.type, center_x, center_y, roi.confidence, distance, area});
    }
    if (candidates.empty()) {
      return std::nullopt;
    }

    const auto fallback = SelectBestCandidate(candidates);
    if (!last_target_.has_value() || last_target_stamp_.nanoseconds() <= 0) {
      return fallback;
    }

    const auto age = stamp - last_target_stamp_;
    if (age > rclcpp::Duration::from_seconds(static_cast<double>(target_hold_ms_) / 1000.0)) {
      return fallback;
    }

    std::optional<TargetCandidate> sticky_target;
    double best_tracking_error = std::numeric_limits<double>::max();
    for (const auto &candidate : candidates) {
      const double tracking_error = DistanceBetween(candidate, *last_target_);
      if (tracking_error > target_switch_radius_px_) {
        continue;
      }
      if (tracking_error < best_tracking_error) {
        sticky_target = candidate;
        best_tracking_error = tracking_error;
      }
    }

    if (!sticky_target.has_value()) {
      // 突然跳到远处的目标先拒绝，不直接切过去；随后中心点帧会让下位机安全刹车。
      return std::nullopt;
    }
    if (!fallback.has_value()) {
      return sticky_target;
    }
    if (DistanceBetween(*fallback, *sticky_target) <= min_send_delta_px_) {
      return sticky_target;
    }
    if (ShouldSwitchTarget(*sticky_target, *fallback)) {
      return fallback;
    }
    return sticky_target;
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

  void SendFrame(uint16_t x, uint16_t y) {
    const std::array<uint8_t, 8> frame = {
      0xFA, 0xFB,
      static_cast<uint8_t>(x & 0xFF),
      static_cast<uint8_t>((x >> 8) & 0xFF),
      static_cast<uint8_t>(y & 0xFF),
      static_cast<uint8_t>((y >> 8) & 0xFF),
      0xFC, 0xFD,
    };

    if (serial_fd_ < 0 && !OpenSerialPort()) {
      return;
    }

    auto written = write(serial_fd_, frame.data(), frame.size());
    if (written == static_cast<ssize_t>(frame.size())) {
      return;
    }

    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 1000,
      "Serial write incomplete: %zd/%zu", written, frame.size());

    if (!ReopenSerialPort()) {
      return;
    }

    written = write(serial_fd_, frame.data(), frame.size());
    if (written != static_cast<ssize_t>(frame.size())) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000,
        "Serial write incomplete after reopen: %zd/%zu", written, frame.size());
    }
  }

  rclcpp::Subscription<ai_msgs::msg::PerceptionTargets>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr diag_timer_;
  rclcpp::TimerBase::SharedPtr follow_send_timer_;
  std::vector<uint8_t> diag_rx_buffer_;
  std::optional<VisionDiagFrame> last_diag_;
  rclcpp::Time last_diag_stamp_{0, 0, RCL_ROS_TIME};
  rclcpp::Time lower_vision_latched_stamp_{0, 0, RCL_ROS_TIME};
  std::string input_topic_;
  std::string serial_port_;
  std::string enemy_prefix_;
  std::vector<std::string> allowed_target_types_;
  std::string selection_mode_;
  int baud_rate_ = 921600;
  int image_width_ = 1280;
  int image_height_ = 1024;
  int lower_diag_timeout_ms_ = 500;
  int lower_vision_latch_ms_ = 5000;
  int target_hold_ms_ = 300;
  std::string follow_control_mode_name_;
  double image_center_x_ = 640.0;
  double image_center_y_ = 512.0;
  double min_confidence_ = 0.5;
  double follow_smoothing_alpha_ = 0.35;
  double follow_interp_rate_hz_ = 10.0;
  double follow_max_step_px_ = 36.0;
  double follow_deadband_px_ = 12.0;
  double measurement_jitter_deadband_px_ = 28.0;
  double fast_follow_error_px_ = 120.0;
  double fast_follow_smoothing_alpha_ = 0.55;
  double fast_follow_max_step_px_ = 72.0;
  double light_follow_gain_ = 0.45;
  double center_gate_x_ratio_ = 0.35;
  double center_gate_y_ratio_ = 0.30;
  double pid_kp_ = 0.55;
  double pid_ki_ = 0.0;
  double pid_kd_ = 0.08;
  double pid_integral_limit_px_ = 160.0;
  double predict_alpha_ = 0.65;
  double predict_beta_ = 0.0;
  double predict_horizon_sec_ = 0.0;
  double target_switch_radius_px_ = 120.0;
  double target_switch_min_conf_gain_ = 0.10;
  double target_switch_center_gain_px_ = 60.0;
  double min_send_delta_px_ = 2.0;
  double follow_send_rate_hz_ = 50.0;
  int send_keepalive_ms_ = 40;
  bool log_selected_target_ = true;
  bool log_diag_feedback_ = false;
  bool require_lower_vision_enabled_ = true;
  bool enable_fixed_rate_follow_ = true;
  FollowControlMode follow_control_mode_{FollowControlMode::kLightPredict};
  bool lower_vision_enabled_ = false;
  bool lower_vision_latched_ = false;
  bool has_active_target_ = false;
  bool filter_initialized_ = false;
  bool predictor_initialized_ = false;
  double filtered_center_x_ = 640.0;
  double filtered_center_y_ = 512.0;
  double predictor_x_ = 640.0;
  double predictor_y_ = 512.0;
  double predictor_vx_ = 0.0;
  double predictor_vy_ = 0.0;
  double pid_integral_x_ = 0.0;
  double pid_integral_y_ = 0.0;
  double pid_prev_error_x_ = 0.0;
  double pid_prev_error_y_ = 0.0;
  std::optional<TargetCandidate> last_target_;
  rclcpp::Time last_target_stamp_{0, 0, RCL_ROS_TIME};
  rclcpp::Time predictor_stamp_{0, 0, RCL_ROS_TIME};
  rclcpp::Time last_control_stamp_{0, 0, RCL_ROS_TIME};
  bool has_last_sent_frame_ = false;
  uint16_t last_sent_x_ = 0;
  uint16_t last_sent_y_ = 0;
  rclcpp::Time last_send_stamp_{0, 0, RCL_ROS_TIME};
  int serial_fd_ = -1;
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GimbalSerialBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
