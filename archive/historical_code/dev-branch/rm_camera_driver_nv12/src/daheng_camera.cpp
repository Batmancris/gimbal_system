// Created by Chengfu Zou on 2023.7.1
// Copyright (C) FYT Vision Group. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rm_camera_driver/daheng_camera.hpp"
// std
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <thread>
// ros2
#include <rclcpp/rate.hpp>
#include <rclcpp/utilities.hpp>
// OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include <arm_neon.h>
#include <sys/ioctl.h>
#include <cstring>
#include <sstream>
// Daheng Galaxy Driver
#include "daheng/GxIAPI.h"
// project
#include "rm_utils/logger/log.hpp"
#include "hobot_cv/hobotcv_imgproc.h"
// Callback Wrapper for adapting GxAPI's interface;
static std::function<void GX_STDC(GX_FRAME_CALLBACK_PARAM *)> g_callback;
extern "C" void GX_STDC CallbackWrapper(GX_FRAME_CALLBACK_PARAM *arg) {
  if (g_callback) {
    g_callback(arg);
  }
}

inline void RGB24_to_NV12(const unsigned char* pRGB, unsigned char* pNV12, int width, int height)
{
    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint8x8_t u8_16 = vdup_n_u8(16);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);

    const int16x8_t s16_zero = vdupq_n_s16(0);
    const int8x8_t s8_rounding = vdup_n_s8(-128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);

    unsigned char* UVPtr = pNV12 + width * height;
    int pitch = width >> 4;

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < pitch; ++i) {
            // Load RGB 16 pixel
            uint8x16x3_t pixel_rgb = vld3q_u8(pRGB);

            uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
            uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
            uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
            uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
            uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
            uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);

            // NOTE:
            // declaration may not appear after executable statement in block
            uint16x8_t high_y;
            uint16x8_t low_y;

            uint8x8_t scalar = vdup_n_u8(66);  // scalar = 66
            high_y = vmull_u8(high_r, scalar);  // Y = R * 66
            low_y = vmull_u8(low_r, scalar);

            scalar = vdup_n_u8(129);
            high_y = vmlal_u8(high_y, high_g, scalar);  // Y = Y + R*129
            low_y = vmlal_u8(low_y, low_g, scalar);

            scalar = vdup_n_u8(25);
            high_y = vmlal_u8(high_y, high_b, scalar);  // Y = Y + B*25
            low_y = vmlal_u8(low_y, low_b, scalar);

            high_y = vaddq_u16(high_y, u16_rounding);  // Y = Y + 128
            low_y = vaddq_u16(low_y, u16_rounding);

            uint8x8_t u8_low_y = vshrn_n_u16(low_y, 8);  // Y = Y >> 8
            uint8x8_t u8_high_y = vshrn_n_u16(high_y, 8);

            low_y = vaddl_u8(u8_low_y, u8_16);  // Y = Y + 16
            high_y = vaddl_u8(u8_high_y, u8_16);

            uint8x16_t pixel_y = vcombine_u8(vqmovn_u16(low_y), vqmovn_u16(high_y));

            // Store
            vst1q_u8(pNV12, pixel_y);
            if (j % 2 == 0)
            {
                uint8x8x2_t mix_r = vuzp_u8(low_r, high_r);
                uint8x8x2_t mix_g = vuzp_u8(low_g, high_g);
                uint8x8x2_t mix_b = vuzp_u8(low_b, high_b);

                int16x8_t signed_r = vreinterpretq_s16_u16(vaddl_u8(mix_r.val[0], u8_zero));
                int16x8_t signed_g = vreinterpretq_s16_u16(vaddl_u8(mix_g.val[0], u8_zero));
                int16x8_t signed_b = vreinterpretq_s16_u16(vaddl_u8(mix_b.val[0], u8_zero));

                int16x8_t signed_u;
                int16x8_t signed_v;

                int16x8_t signed_scalar = vdupq_n_s16(-38);
                signed_u = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_v = vmulq_s16(signed_r, signed_scalar);

                signed_scalar = vdupq_n_s16(-74);
                signed_u = vmlaq_s16(signed_u, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(-94);
                signed_v = vmlaq_s16(signed_v, signed_g, signed_scalar);

                signed_scalar = vdupq_n_s16(112);
                signed_u = vmlaq_s16(signed_u, signed_b, signed_scalar);

                signed_scalar = vdupq_n_s16(-18);
                signed_v = vmlaq_s16(signed_v, signed_b, signed_scalar);

                signed_u = vaddq_s16(signed_u, s16_rounding);
                signed_v = vaddq_s16(signed_v, s16_rounding);

                int8x8_t s8_u = vshrn_n_s16(signed_u, 8);
                int8x8_t s8_v = vshrn_n_s16(signed_v, 8);

                signed_u = vsubl_s8(s8_u, s8_rounding);
                signed_v = vsubl_s8(s8_v, s8_rounding);

                signed_u = vmaxq_s16(signed_u, s16_zero);
                signed_v = vmaxq_s16(signed_v, s16_zero);

                uint16x8_t unsigned_u = vreinterpretq_u16_s16(signed_u);
                uint16x8_t unsigned_v = vreinterpretq_u16_s16(signed_v);

                uint8x8x2_t result;
                result.val[0] = vqmovn_u16(unsigned_u);
                result.val[1] = vqmovn_u16(unsigned_v);

                vst2_u8(UVPtr, result);
                UVPtr += 16;
            }

            pRGB += 3 * 16;
            pNV12 += 16;
        }
    }
}

inline bool ThreadBindCPU(const int &cpu_id, pthread_t native_handle){
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);          // 初始化 CPU 集合
  CPU_SET(cpu_id, &cpuset); 
  int ret = pthread_setaffinity_np(native_handle, sizeof(cpu_set_t), &cpuset);
  if (ret != 0) {
      std::cerr << "Error setting CPU affinity: " << strerror(ret) << std::endl;
      return false;
  }
  return true;
}

namespace fyt::camera_driver {
DahengCameraNode::DahengCameraNode(const rclcpp::NodeOptions &options)
: Node("camera_driver", options) {
  FYT_REGISTER_LOGGER("camera_driver", "~/fyt2024-log", INFO);
  FYT_INFO("camera_driver", "Starting DahengCameraNode!");

  camera_name_ = this->declare_parameter("camera_name", "daheng");
  camera_info_url_ =
    this->declare_parameter("camera_info_url", "package://rm_bringup/config/camera_info.yaml");
  frame_id_ = this->declare_parameter("camera_frame_id", "camera_optical_frame");
  pixel_format_ = this->declare_parameter("pixel_format", "rgb8");
  resolution_width_ = this->declare_parameter("resolution_width", 1280);
  resolution_height_ = this->declare_parameter("resolution_height", 1024);
  auto_white_balance_ = this->declare_parameter("auto_white_balance", 1);
  frame_rate_ = this->declare_parameter("frame_rate", 100);
  exposure_time_ = this->declare_parameter("exposure_time", 5000);
  gain_ = this->declare_parameter("gain", 5.0);
  offest_x_ = this->declare_parameter("offsetX", 0);
  offset_y_ = this->declare_parameter("offsetY", 0);
  //为存储原始图像数据申请空间


  if (pixel_format_ == "mono8") {
    gx_pixel_format_ = GX_PIXEL_FORMAT_MONO8;
  } else if (pixel_format_ == "mono16") {
    gx_pixel_format_ = GX_PIXEL_FORMAT_MONO16;
  } else if (pixel_format_ == "bgr8") {
    gx_pixel_format_ = GX_PIXEL_FORMAT_BGR8;
    gx_bayer_type_ = BAYERBG;
  } else if (pixel_format_ == "rgb8") {
    gx_pixel_format_ = GX_PIXEL_FORMAT_RGB8;
    gx_bayer_type_ = BAYERRG;
  } else if (pixel_format_ == "bgra8") {
    gx_pixel_format_ = GX_PIXEL_FORMAT_BGRA8;
  } else {
    FYT_ERROR("camera_driver", "Unsupported pixel format: {}", pixel_format_);
  }

  camera_info_manager_ =
    std::make_unique<camera_info_manager::CameraInfoManager>(this, camera_name_);

  if (camera_info_manager_->validateURL(camera_info_url_)) {
    camera_info_manager_->loadCameraInfo(camera_info_url_);
    camera_info_ = camera_info_manager_->getCameraInfo();
  } else {
    camera_info_manager_->setCameraName(camera_name_);
    sensor_msgs::msg::CameraInfo camera_info;
    camera_info.width = resolution_width_;
    camera_info.height = resolution_height_;
    camera_info_manager_->setCameraInfo(camera_info);
    FYT_WARN("camera_driver", "Invalid camera info URL: {}", camera_info_url_);
  }
  camera_info_.header.frame_id = frame_id_;
  camera_info_.header.stamp = this->now();

  // bool use_sensor_data_qos = this->declare_parameter("use_sensor_data_qos", true);
  // auto qos = use_sensor_data_qos ? rmw_qos_profile_sensor_data : rmw_qos_profile_default;
  // pub_ = image_transport::create_camera_publisher(this, "image_raw", qos);
  publisher_hbmem_ = this->create_publisher<hbm_img_msgs::msg::HbmMsg1080P>(
              "hbmem_img", rclcpp::SensorDataQoS());
  cam_info_publisher_ = this->create_publisher<sensor_msgs::msg::CameraInfo>("camera_info", 10);
  // Heartbeat
  heartbeat_ = HeartBeatPublisher::create(this);

  // Check if camera is alive every seconds
  timer_ = this->create_wall_timer(std::chrono::milliseconds(1000),
                                   std::bind(&DahengCameraNode::timerCallback, this));

  // Param set callback
  on_set_parameters_callback_handle_ = this->add_on_set_parameters_callback(
    std::bind(&DahengCameraNode::onSetParameters, this, std::placeholders::_1));

  for(int i = 0; i < thread_num_; i++){
    thread_pool_.push_back(std::make_shared<std::thread>(std::bind(&DahengCameraNode::FrameProcess, this)));
    ThreadBindCPU(cpu_id_[i], thread_pool_[i]->native_handle());
  }
  ThreadBindCPU(self_cpu_id_, pthread_self());

  // Recorder
  bool enable_recorder = this->declare_parameter("recording", false);
  if (enable_recorder) {
    std::string home = std::getenv("HOME");

    namespace fs = std::filesystem;
    std::filesystem::path video_path = fs::path(home) / "fyt2024-log/video/" /
                                       std::string(std::to_string(std::time(nullptr)) + ".avi");

    recorder_ = std::make_unique<Recorder>(
      video_path, frame_rate_, cv::Size(resolution_width_, resolution_height_));
    recorder_->start();
    FYT_INFO("camera_driver", "Recorder started! Video file: {}", video_path.string());
  }

  FYT_INFO("camera_driver", "DahengCameraNode has been initialized!");
}

DahengCameraNode::~DahengCameraNode() {
  close();
  if (recorder_ != nullptr) {
    recorder_->stop();
    FYT_INFO(
      "camera_driver", "Recorder stopped! Video file {} has been saved", recorder_->path.string());
  }
  FYT_INFO("camera_driver", "DahengCameraNode has been destroyed!");


  {
    std::unique_lock<std::mutex> lock(mutex_);
    free(frame_buff_);
    frame_update_ = true;
    process_stop_ = true;
    cv_.notify_all();
  }
  for(int i = 0; i < thread_num_; i++){
    if (thread_pool_[i] && thread_pool_[i]->joinable()) {
      thread_pool_[i]->join();
      thread_pool_[i] = nullptr;
    }
  }

}

void DahengCameraNode::timerCallback() {
  // Open camera
  while (!is_open_ && rclcpp::ok()) {
    bool is_open_success = open();
    if (!is_open_success) {
      FYT_ERROR("camera_driver", "open() failed");
      close();
      return;
    }
  }
  // Watch Dog
  double dt = (this->now() - rclcpp::Time(camera_info_.header.stamp)).seconds();
  if (dt > 5.0) {
    FYT_WARN("camera_driver", "Camera is not alive! lost frame for {:.2f} seconds", dt);
    close();
  }
}

void DahengCameraNode::close() {
  FYT_INFO("camera_driver", "Closing Daheng Galaxy Camera Device!");
  if (is_open_ && dev_handle_ != nullptr) {
    //发送停采命令
    GXStreamOff(dev_handle_);
    GXCloseDevice(dev_handle_);
    // GXUnregisterCaptureCallback(dev_handle_);
  }
  GXCloseLib();
  is_open_ = false;
}

bool DahengCameraNode::open() {
  FYT_INFO("camera_driver", "Opening Daheng Galaxy Camera Device!");
  if (is_open_) {
    FYT_WARN("camera_driver", "Daheng Galaxy Camera Device is already opened!");
    close();
  }
  gx_status_ = GX_STATUS_SUCCESS;
  GX_OPEN_PARAM openParam;
  uint32_t device_num = 0;
  openParam.accessMode = GX_ACCESS_EXCLUSIVE;
  openParam.openMode = GX_OPEN_INDEX;
  openParam.pszContent = (char *)"1";
  // 尝试初始化库
  gx_status_ = GXInitLib();
  if (gx_status_ != GX_STATUS_SUCCESS) {
    FYT_ERROR("camera_driver", "Can't init lib");
    return false;
  }

  // 枚举设备列表
  gx_status_ = GXUpdateDeviceList(&device_num, 1000);
  if ((gx_status_ != GX_STATUS_SUCCESS) || (device_num <= 0)) {
    FYT_WARN("camera_driver", "Can't find camera");
    return false;
  }
  FYT_INFO("camera_driver", "Found {} devices", device_num);
  //打开设备
  gx_status_ = GXOpenDevice(&openParam, &dev_handle_);
  if (gx_status_ != GX_STATUS_SUCCESS) {
    FYT_ERROR("camera_driver", "Can't open device");
    return false;
  }
  is_open_ = true;

  GXGetInt(dev_handle_, GX_INT_WIDTH_MAX, &max_resolution_width_);
  GXGetInt(dev_handle_, GX_INT_HEIGHT_MAX, &max_resolution_height_);

  // 设置像素格式
  GXSetEnum(dev_handle_, GX_ENUM_PIXEL_FORMAT, gx_pixel_format_);
  // 设置宽度
  GXSetInt(dev_handle_, GX_INT_WIDTH, resolution_width_);
  // 设置高度
  GXSetInt(dev_handle_, GX_INT_HEIGHT, resolution_height_);

  // // 从中心裁剪
  // int64_t offest_x_ = (MAX_RESOLUTION_WIDTH - resolution_width_) / 2;
  // int64_t offset_y_ = (MAX_RESOLUTION_HEIGHT - resolution_height_) / 2;
  //  GXSetEnum(dev_handle_, GX_ENUM_RREGION_SELECTOR,
  //  GX_REGION_SELECTOR_REGION0);
  GXSetInt(dev_handle_, GX_INT_OFFSET_X, offest_x_);
  GXSetInt(dev_handle_, GX_INT_OFFSET_Y, offset_y_);

  // 获取图像大小
  GXGetInt(dev_handle_, GX_INT_PAYLOAD_SIZE, &gx_pay_load_size_);
  //设置是否开启自动白平衡
  if (auto_white_balance_) {
    GXSetEnum(dev_handle_, GX_ENUM_BALANCE_WHITE_AUTO, 1);
  } else {
    //设置白平衡数值  如果开启自动白平衡则无效
    GXSetEnum(dev_handle_, GX_ENUM_LIGHT_SOURCE_PRESET, GX_LIGHT_SOURCE_PRESET_DAYLIGHT_5000K);
  }
  //设置帧率
  GXSetEnum(dev_handle_, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ENUM_COVER_FRAMESTORE_MODE_ON);
  GXSetFloat(dev_handle_, GX_FLOAT_ACQUISITION_FRAME_RATE, frame_rate_);

  //设置曝光时间
  GXSetFloat(dev_handle_, GX_FLOAT_EXPOSURE_TIME, exposure_time_);
  //设置增益
  GXSetFloat(dev_handle_, GX_FLOAT_GAIN, gain_);

  //设置采集模式连续采集
  GXSetEnum(dev_handle_, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
  GXSetInt(dev_handle_, GX_INT_ACQUISITION_SPEED_LEVEL, 1);

  //注册图像处理回调函数
  // void (*)(GX_FRAME_CALLBACK_PARAM *)
  g_callback = std::bind(&DahengCameraNode::onFrameCallbackFun, this, std::placeholders::_1);
  gx_status_ = GXRegisterCaptureCallback(dev_handle_, nullptr, CallbackWrapper);
  if (gx_status_ != GX_STATUS_SUCCESS) {
    FYT_ERROR("camera_driver", "Register capture callback function failed!");
    return false;
  }

  //发送开采命令
  gx_status_ = GXStreamOn(dev_handle_);
  if (gx_status_ != GX_STATUS_SUCCESS) {
    FYT_ERROR("camera_driver", "Send start capture command failed!");
    return false;
  }
  FYT_INFO("camera_driver", "Daheng Galaxy Camera Device Open Success!");
  return true;
}

void DahengCameraNode::WriteFrame(GX_FRAME_CALLBACK_PARAM *pFrame){
  if (pFrame->status == GX_FRAME_STATUS_SUCCESS) {
    if(frame_buff_ == nullptr){
      frame_buff_ = malloc(pFrame->nImgSize * 2);
    }
    if(frame_img_size_ == -1){
      frame_img_size_ = pFrame->nImgSize;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if(read_block_number_ == 0){
      write_block_number_ = 1;
    } else {
      write_block_number_ = 0;
    }
    lock.unlock();
    time_stamps_[write_block_number_] = this->now();
    char* write_add = static_cast<char*>(frame_buff_) + (write_block_number_ * pFrame->nImgSize);
    memcpy(static_cast<void *>(write_add), const_cast<void *>(pFrame->pImgBuf), pFrame->nImgSize);

    lock.lock();
    frame_update_ = true;
    if(read_block_number_ == -1){
      read_completed_ = true;
    }
    if(read_completed_ == true){
      if(write_block_number_ == 0){
        read_block_number_ = 0;
      } else {
        read_block_number_ = 1;
      }
      read_completed_ = false;
    }
    lock.unlock();
    cv_.notify_all();
  }
}

bool DahengCameraNode::ReadFrame(sensor_msgs::msg::Image &image_msg){
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]{ return frame_update_; });
    if(read_block_number_ != -1 && frame_update_ == true && process_stop_ == false){
      image_msg.header.stamp = camera_info_.header.stamp = time_stamps_[read_block_number_];
      char* read_add = static_cast<char*>(frame_buff_) + (read_block_number_ * frame_img_size_);
      DxRaw8toRGB24(static_cast<void*>(read_add),
                    image_msg.data.data(),
                    resolution_width_,
                    resolution_height_,
                    RAW2RGB_NEIGHBOUR,
                    static_cast<DX_PIXEL_COLOR_FILTER>(gx_bayer_type_),
                    false);
      read_completed_ = true;
      frame_update_ = false;
      return true;
    } else {
      return false;
    }
  }
}


void DahengCameraNode::FrameProcess(){

  uint8_t* mPtrIn= new uint8_t[resolution_width_ * resolution_height_ * 3 / 2];

  auto loanedMsg = publisher_hbmem_->borrow_loaned_message();
  auto &msg = loanedMsg.get();
  memcpy(msg.encoding.data(), "nv12", strlen("nv12"));
  msg.height = resolution_height_;
  msg.width = resolution_width_;

  sensor_msgs::msg::Image image_msg;
  image_msg.header.frame_id = frame_id_;
  image_msg.encoding = pixel_format_;
  image_msg.height = resolution_height_;
  image_msg.width = resolution_width_;
  image_msg.step = resolution_width_ * 3;
  image_msg.data.resize(image_msg.height * image_msg.step);
  while(process_stop_ == false){
    if(!ReadFrame(image_msg)){
      continue;
    }
    msg.time_stamp.sec = image_msg.header.stamp.sec;
    msg.time_stamp.nanosec = image_msg.header.stamp.nanosec;

    RGB24_to_NV12(image_msg.data.data(), mPtrIn,
              image_msg.width, image_msg.height);
    
    memcpy(&msg.data[0], mPtrIn, msg.height * msg.width * 3 / 2);
    publisher_hbmem_->publish(msg);
    cam_info_publisher_->publish(camera_info_);
    if (recorder_ != nullptr) {
      recorder_->addFrame(image_msg.data);
    }
  }

  delete[] mPtrIn;
  mPtrIn = nullptr;
}



void GX_STDC DahengCameraNode::onFrameCallbackFun(GX_FRAME_CALLBACK_PARAM *pFrame) {
  WriteFrame(pFrame);
}

rcl_interfaces::msg::SetParametersResult DahengCameraNode::onSetParameters(
  std::vector<rclcpp::Parameter> parameters) {
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  for (const auto &param : parameters) {
    if (param.get_name() == "exposure_time") {
      exposure_time_ = param.as_int();
      GXSetFloat(dev_handle_, GX_FLOAT_EXPOSURE_TIME, static_cast<double>(exposure_time_));
      FYT_INFO("camera_driver", "Set exposure_time: {}", exposure_time_);
    } else if (param.get_name() == "gain") {
      gain_ = param.as_double();
      GXSetFloat(dev_handle_, GX_FLOAT_GAIN, gain_);
      FYT_INFO("camera_driver", "Set gain: {}", gain_);
    }
  }
  return result;
}

}  // namespace fyt::camera_driver

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(fyt::camera_driver::DahengCameraNode)
