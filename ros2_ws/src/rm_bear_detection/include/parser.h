#ifndef RM_BEAR_DETECTION__PARSER_H_
#define RM_BEAR_DETECTION__PARSER_H_

#include "dnn_node/dnn_node_data.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rm_bear_detection {

enum class YoloBoxFormat {
  kXyxy = 0,
  kCxcywh = 1,
};

struct YoloV8Detection {
  int id;
  float xmin;
  float ymin;
  float xmax;
  float ymax;
  float score;
  std::string class_name;
};

struct YoloV8ParserConfig {
  float score_threshold;
  float nms_threshold;
  int nms_top_k;
  std::string class_name;
  int input_width;
  int input_height;
  YoloBoxFormat box_format = YoloBoxFormat::kXyxy;
};

struct DebugAnchorInfo {
  int anchor = -1;
  float raw0 = 0.0F;
  float raw1 = 0.0F;
  float raw2 = 0.0F;
  float raw3 = 0.0F;
  float score = 0.0F;
};

struct OutputTensorDebugInfo {
  int layout = -1;
  int valid_shape[4] = {0, 0, 0, 0};
  int aligned_shape[4] = {0, 0, 0, 0};
  int channels = 0;
  int anchors = 0;
};

int32_t ParseDetections(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<std::shared_ptr<YoloV8Detection>> &results);

DebugAnchorInfo FindBestAnchor(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output);

std::vector<DebugAnchorInfo> FindTopAnchors(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  std::size_t top_k);

OutputTensorDebugInfo GetOutputTensorDebugInfo(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output);

}  // namespace rm_bear_detection

#endif  // RM_BEAR_DETECTION__PARSER_H_
