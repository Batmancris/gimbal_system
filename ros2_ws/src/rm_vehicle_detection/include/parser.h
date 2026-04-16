#ifndef RM_VEHICLE_DETECTION__PARSER_H_
#define RM_VEHICLE_DETECTION__PARSER_H_

#include "dnn_node/dnn_node_data.h"

#include <memory>
#include <string>
#include <vector>

namespace rm_vehicle_detection {

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
};

int32_t ParseDetections(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<std::shared_ptr<YoloV8Detection>> &results);

}  // namespace rm_vehicle_detection

#endif  // RM_VEHICLE_DETECTION__PARSER_H_
