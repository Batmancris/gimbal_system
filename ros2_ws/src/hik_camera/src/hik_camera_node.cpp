#include "MvCameraControl.h"

#include <camera_info_manager/camera_info_manager.hpp>
#include <hbm_img_msgs/msg/hbm_msg1080_p.hpp>
#include <image_transport/camera_publisher.hpp>
#include <image_transport/image_transport.hpp>
#include <opencv2/imgproc.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace hik_camera {

class HikCameraNode : public rclcpp::Node {
 public:
  explicit HikCameraNode(const rclcpp::NodeOptions &options)
  : Node("hik_camera", options) {
    RCLCPP_INFO(get_logger(), "Starting HikCameraNode");

    use_sensor_data_qos_ = declare_parameter("use_sensor_data_qos", true);
    camera_name_ = declare_parameter("camera_name", "hik_camera");
    frame_id_ = declare_parameter("frame_id", "camera_optical_frame");
    exposure_auto_mode_ = declare_parameter("exposure_auto_mode", 0);
    exposure_time_ = declare_parameter("exposure_time", 6000.0);
    gain_auto_mode_ = declare_parameter("gain_auto_mode", 0);
    gain_ = declare_parameter("gain", 32.0);
    balance_white_auto_mode_ = declare_parameter("balance_white_auto_mode", 0);
    gamma_enable_ = declare_parameter("gamma_enable", false);
    gamma_selector_ = declare_parameter("gamma_selector", 1);
    gamma_ = declare_parameter("gamma", 1.0);
    camera_info_url_ = declare_parameter(
      "camera_info_url", "package://hik_camera/config/camera_info.yaml");

    auto qos = use_sensor_data_qos_ ? rmw_qos_profile_sensor_data : rmw_qos_profile_default;
    camera_pub_ = image_transport::create_camera_publisher(this, "image_raw", qos);
    hbmem_pub_ = create_publisher<hbm_img_msgs::msg::HbmMsg1080P>(
      "/hbmem_img", rclcpp::SensorDataQoS());

    camera_info_manager_ =
      std::make_unique<camera_info_manager::CameraInfoManager>(this, camera_name_);
    if (camera_info_manager_->validateURL(camera_info_url_)) {
      camera_info_manager_->loadCameraInfo(camera_info_url_);
      camera_info_msg_ = camera_info_manager_->getCameraInfo();
    }
    camera_info_msg_.header.frame_id = frame_id_;

    if (!OpenCamera()) {
      RCLCPP_FATAL(get_logger(), "Failed to open Hikrobot camera");
      rclcpp::shutdown();
      return;
    }

    params_callback_handle_ = add_on_set_parameters_callback(
      std::bind(&HikCameraNode::OnParameters, this, std::placeholders::_1));

    running_.store(true);
    capture_thread_ = std::thread(&HikCameraNode::CaptureLoop, this);
  }

  ~HikCameraNode() override {
    running_.store(false);
    if (capture_thread_.joinable()) {
      capture_thread_.join();
    }
    CloseCamera();
  }

 private:
  bool OpenCamera() {
    MV_CC_DEVICE_INFO_LIST device_list;
    std::memset(&device_list, 0, sizeof(device_list));

    int ret = MV_CC_EnumDevices(MV_USB_DEVICE | MV_GIGE_DEVICE, &device_list);
    if (ret != MV_OK) {
      RCLCPP_ERROR(get_logger(), "MV_CC_EnumDevices failed: 0x%x", ret);
      return false;
    }
    if (device_list.nDeviceNum == 0) {
      RCLCPP_ERROR(get_logger(), "No Hikrobot camera found");
      return false;
    }

    ret = MV_CC_CreateHandle(&camera_handle_, device_list.pDeviceInfo[0]);
    if (ret != MV_OK) {
      RCLCPP_ERROR(get_logger(), "MV_CC_CreateHandle failed: 0x%x", ret);
      return false;
    }

    ret = MV_CC_OpenDevice(camera_handle_);
    if (ret != MV_OK) {
      RCLCPP_ERROR(get_logger(), "MV_CC_OpenDevice failed: 0x%x", ret);
      return false;
    }

    const auto *device_info = device_list.pDeviceInfo[0];
    if (device_info != nullptr && device_info->nTLayerType == MV_GIGE_DEVICE) {
      const int packet_size = MV_CC_GetOptimalPacketSize(camera_handle_);
      if (packet_size > 0) {
        const int ret_int = MV_CC_SetIntValue(
          camera_handle_, "GevSCPSPacketSize", static_cast<uint32_t>(packet_size));
        if (ret_int != MV_OK) {
          RCLCPP_WARN(
            get_logger(), "Failed to set GevSCPSPacketSize=%d: 0x%x", packet_size, ret_int);
        } else {
          RCLCPP_INFO(get_logger(), "Set GevSCPSPacketSize to %d", packet_size);
        }
      } else {
        RCLCPP_WARN(
          get_logger(), "MV_CC_GetOptimalPacketSize returned invalid value: %d", packet_size);
      }
    }

    int ret_enum = MV_CC_SetEnumValue(camera_handle_, "TriggerMode", 0);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set TriggerMode=Off: 0x%x", ret_enum);
    }

    ret_enum = MV_CC_SetEnumValue(camera_handle_, "AcquisitionMode", 2);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set AcquisitionMode=Continuous: 0x%x", ret_enum);
    }

    ret_enum = MV_CC_SetEnumValue(camera_handle_, "ExposureAuto", exposure_auto_mode_);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set ExposureAuto=%d: 0x%x", exposure_auto_mode_, ret_enum);
    }

    ret_enum = MV_CC_SetEnumValue(camera_handle_, "GainAuto", gain_auto_mode_);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set GainAuto=%d: 0x%x", gain_auto_mode_, ret_enum);
    }

    ret_enum = MV_CC_SetEnumValue(camera_handle_, "BalanceWhiteAuto", balance_white_auto_mode_);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(
        get_logger(), "Failed to set BalanceWhiteAuto=%d: 0x%x", balance_white_auto_mode_, ret_enum);
    }

    int ret_float = MV_CC_SetFloatValue(camera_handle_, "ExposureTime", exposure_time_);
    if (ret_float != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set ExposureTime: 0x%x", ret_float);
    }

    ret_float = MV_CC_SetFloatValue(camera_handle_, "Gain", gain_);
    if (ret_float != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set Gain: 0x%x", ret_float);
    }

    int ret_bool = MV_CC_SetBoolValue(camera_handle_, "GammaEnable", gamma_enable_);
    if (ret_bool != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set GammaEnable=%d: 0x%x", gamma_enable_, ret_bool);
    }

    ret_enum = MV_CC_SetEnumValue(camera_handle_, "GammaSelector", gamma_selector_);
    if (ret_enum != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set GammaSelector=%d: 0x%x", gamma_selector_, ret_enum);
    }

    ret_float = MV_CC_SetFloatValue(camera_handle_, "Gamma", gamma_);
    if (ret_float != MV_OK) {
      RCLCPP_WARN(get_logger(), "Failed to set Gamma: 0x%x", ret_float);
    }

    ret = MV_CC_GetImageInfo(camera_handle_, &img_info_);
    if (ret != MV_OK) {
      RCLCPP_ERROR(get_logger(), "MV_CC_GetImageInfo failed: 0x%x", ret);
      return false;
    }

    image_msg_.header.frame_id = frame_id_;
    image_msg_.encoding = sensor_msgs::image_encodings::RGB8;
    image_msg_.height = img_info_.nHeightValue;
    image_msg_.width = img_info_.nWidthValue;
    image_msg_.step = img_info_.nWidthValue * 3;
    image_msg_.data.resize(static_cast<size_t>(image_msg_.step) * image_msg_.height);
    nv12_buffer_.resize(static_cast<size_t>(img_info_.nWidthValue) * img_info_.nHeightValue * 3 / 2);

    std::memset(&convert_param_, 0, sizeof(convert_param_));
    convert_param_.nWidth = img_info_.nWidthValue;
    convert_param_.nHeight = img_info_.nHeightValue;
    convert_param_.enDstPixelType = PixelType_Gvsp_RGB8_Packed;

    camera_info_msg_.width = img_info_.nWidthValue;
    camera_info_msg_.height = img_info_.nHeightValue;

    ret = MV_CC_StartGrabbing(camera_handle_);
    if (ret != MV_OK) {
      RCLCPP_ERROR(get_logger(), "MV_CC_StartGrabbing failed: 0x%x", ret);
      return false;
    }

    return true;
  }

  void CloseCamera() {
    if (camera_handle_ != nullptr) {
      MV_CC_StopGrabbing(camera_handle_);
      MV_CC_CloseDevice(camera_handle_);
      MV_CC_DestroyHandle(&camera_handle_);
      camera_handle_ = nullptr;
    }
  }

  void CaptureLoop() {
    bool first_frame_logged = false;
    int consecutive_failures = 0;

    while (running_.load() && rclcpp::ok()) {
      MV_FRAME_OUT out_frame;
      std::memset(&out_frame, 0, sizeof(out_frame));
      int ret = MV_CC_GetImageBuffer(camera_handle_, &out_frame, 1000);
      if (ret != MV_OK) {
        ++consecutive_failures;
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 1000, "Get image buffer failed: 0x%x", ret);

        if (consecutive_failures >= 30) {
          RCLCPP_WARN(
            get_logger(),
            "GetImageBuffer failed %d times, restarting grabbing",
            consecutive_failures);
          MV_CC_StopGrabbing(camera_handle_);
          std::this_thread::sleep_for(std::chrono::milliseconds(120));
          const int restart_ret = MV_CC_StartGrabbing(camera_handle_);
          if (restart_ret != MV_OK) {
            RCLCPP_ERROR(get_logger(), "MV_CC_StartGrabbing restart failed: 0x%x", restart_ret);
          }
          consecutive_failures = 0;
        }
        continue;
      }

      consecutive_failures = 0;

      if (out_frame.stFrameInfo.nWidth == 0 || out_frame.stFrameInfo.nHeight == 0) {
        MV_CC_FreeImageBuffer(camera_handle_, &out_frame);
        continue;
      }

      image_msg_.width = out_frame.stFrameInfo.nWidth;
      image_msg_.height = out_frame.stFrameInfo.nHeight;
      image_msg_.step = out_frame.stFrameInfo.nWidth * 3;
      image_msg_.data.resize(static_cast<size_t>(image_msg_.step) * image_msg_.height);

      convert_param_.nWidth = out_frame.stFrameInfo.nWidth;
      convert_param_.nHeight = out_frame.stFrameInfo.nHeight;
      convert_param_.pSrcData = out_frame.pBufAddr;
      convert_param_.nSrcDataLen = out_frame.stFrameInfo.nFrameLen;
      convert_param_.enSrcPixelType = out_frame.stFrameInfo.enPixelType;
      convert_param_.pDstBuffer = image_msg_.data.data();
      convert_param_.nDstBufferSize = image_msg_.data.size();

      ret = MV_CC_ConvertPixelType(camera_handle_, &convert_param_);
      MV_CC_FreeImageBuffer(camera_handle_, &out_frame);

      if (ret != MV_OK) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 1000, "Convert pixel type failed: 0x%x", ret);
        continue;
      }

      image_msg_.header.stamp = now();
      camera_info_msg_.header = image_msg_.header;
      PublishHbmem(image_msg_.header.stamp);
      if (!first_frame_logged) {
        RCLCPP_INFO(
          get_logger(), "Publishing first frame: %ux%u, frame_len=%u, pixel_type=0x%lx",
          out_frame.stFrameInfo.nWidth, out_frame.stFrameInfo.nHeight,
          out_frame.stFrameInfo.nFrameLen, static_cast<unsigned long>(out_frame.stFrameInfo.enPixelType));
        first_frame_logged = true;
      }
      camera_pub_.publish(image_msg_, camera_info_msg_);
    }
  }

  rcl_interfaces::msg::SetParametersResult OnParameters(
    const std::vector<rclcpp::Parameter> &parameters) {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto &param : parameters) {
      if (param.get_name() == "exposure_auto_mode") {
        const int ret = MV_CC_SetEnumValue(
          camera_handle_, "ExposureAuto", static_cast<unsigned int>(param.as_int()));
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set exposure_auto_mode";
          return result;
        }
        exposure_auto_mode_ = static_cast<int>(param.as_int());
      } else if (param.get_name() == "exposure_time") {
        const int ret = MV_CC_SetFloatValue(camera_handle_, "ExposureTime", param.as_double());
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set exposure_time";
          return result;
        }
        exposure_time_ = param.as_double();
      } else if (param.get_name() == "gain_auto_mode") {
        const int ret = MV_CC_SetEnumValue(
          camera_handle_, "GainAuto", static_cast<unsigned int>(param.as_int()));
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set gain_auto_mode";
          return result;
        }
        gain_auto_mode_ = static_cast<int>(param.as_int());
      } else if (param.get_name() == "gain") {
        const int ret = MV_CC_SetFloatValue(camera_handle_, "Gain", param.as_double());
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set gain";
          return result;
        }
        gain_ = param.as_double();
      } else if (param.get_name() == "balance_white_auto_mode") {
        const int ret = MV_CC_SetEnumValue(
          camera_handle_, "BalanceWhiteAuto", static_cast<unsigned int>(param.as_int()));
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set balance_white_auto_mode";
          return result;
        }
        balance_white_auto_mode_ = static_cast<int>(param.as_int());
      } else if (param.get_name() == "gamma_enable") {
        const int ret = MV_CC_SetBoolValue(camera_handle_, "GammaEnable", param.as_bool());
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set gamma_enable";
          return result;
        }
        gamma_enable_ = param.as_bool();
      } else if (param.get_name() == "gamma_selector") {
        const int ret = MV_CC_SetEnumValue(
          camera_handle_, "GammaSelector", static_cast<unsigned int>(param.as_int()));
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set gamma_selector";
          return result;
        }
        gamma_selector_ = static_cast<int>(param.as_int());
      } else if (param.get_name() == "gamma") {
        const int ret = MV_CC_SetFloatValue(camera_handle_, "Gamma", param.as_double());
        if (ret != MV_OK) {
          result.successful = false;
          result.reason = "Failed to set gamma";
          return result;
        }
        gamma_ = param.as_double();
      }
    }

    return result;
  }

  void PublishHbmem(const builtin_interfaces::msg::Time &stamp) {
    if (image_msg_.width == 0 || image_msg_.height == 0) {
      return;
    }

    const auto width = static_cast<int>(image_msg_.width);
    const auto height = static_cast<int>(image_msg_.height);
    const size_t nv12_size = static_cast<size_t>(width) * height * 3 / 2;
    if (nv12_buffer_.size() != nv12_size) {
      nv12_buffer_.resize(nv12_size);
    }

    cv::Mat rgb(height, width, CV_8UC3, image_msg_.data.data());
    cv::Mat yuv_i420;
    cv::cvtColor(rgb, yuv_i420, cv::COLOR_RGB2YUV_I420);

    const uint8_t *i420 = yuv_i420.ptr<uint8_t>();
    const size_t y_size = static_cast<size_t>(width) * height;
    const size_t uv_plane_size = y_size / 4;
    std::memcpy(nv12_buffer_.data(), i420, y_size);

    const uint8_t *u_plane = i420 + y_size;
    const uint8_t *v_plane = u_plane + uv_plane_size;
    uint8_t *uv_dst = nv12_buffer_.data() + y_size;
    for (size_t i = 0; i < uv_plane_size; ++i) {
      uv_dst[2 * i] = u_plane[i];
      uv_dst[2 * i + 1] = v_plane[i];
    }

    hbm_img_msgs::msg::HbmMsg1080P msg;
    msg.index = frame_index_++;
    msg.time_stamp = stamp;
    msg.height = image_msg_.height;
    msg.width = image_msg_.width;
    msg.data_size = static_cast<uint32_t>(nv12_size);
    msg.step = image_msg_.width;
    const char encoding[] = "nv12";
    std::fill(msg.encoding.begin(), msg.encoding.end(), 0);
    std::memcpy(msg.encoding.data(), encoding, sizeof(encoding) - 1);
    std::memcpy(msg.data.data(), nv12_buffer_.data(), nv12_size);
    hbmem_pub_->publish(msg);
  }

  image_transport::CameraPublisher camera_pub_;
  rclcpp::Publisher<hbm_img_msgs::msg::HbmMsg1080P>::SharedPtr hbmem_pub_;
  std::unique_ptr<camera_info_manager::CameraInfoManager> camera_info_manager_;
  sensor_msgs::msg::Image image_msg_;
  sensor_msgs::msg::CameraInfo camera_info_msg_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr params_callback_handle_;

  void *camera_handle_ = nullptr;
  MV_IMAGE_BASIC_INFO img_info_;
  MV_CC_PIXEL_CONVERT_PARAM convert_param_;
  std::thread capture_thread_;
  std::atomic<bool> running_ {false};
  std::vector<uint8_t> nv12_buffer_;
  int32_t frame_index_ = 0;

  bool use_sensor_data_qos_ = true;
  std::string camera_name_;
  std::string frame_id_;
  std::string camera_info_url_;
  int exposure_auto_mode_ = 0;
  double exposure_time_ = 6000.0;
  int gain_auto_mode_ = 0;
  double gain_ = 32.0;
  int balance_white_auto_mode_ = 0;
  bool gamma_enable_ = false;
  int gamma_selector_ = 1;
  double gamma_ = 1.0;
};

}  // namespace hik_camera

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(hik_camera::HikCameraNode)
