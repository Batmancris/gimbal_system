#include "parser.h"

#include <opencv2/dnn/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace rm_vehicle_detection {
namespace {

constexpr int kClassNum = 1;
constexpr int kReg = 16;
constexpr std::array<int, 3> kStrides = {8, 16, 32};

struct ParsedDetection {
  int id;
  float xmin;
  float ymin;
  float xmax;
  float ymax;
  float score;
};

float SigmoidInverse(const float p) {
  const float clamped = std::clamp(p, 1.0e-6F, 1.0F - 1.0e-6F);
  return -std::log(1.0F / clamped - 1.0F);
}

void ParseScale(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  const std::array<int, 3> &order,
  std::vector<ParsedDetection> &parsed,
  std::vector<cv::Rect2d> &bboxes,
  std::vector<float> &scores) {
  hbSysFlushMem(
    &(node_output->output_tensors[order[0]]->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);
  hbSysFlushMem(
    &(node_output->output_tensors[order[1]]->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);

  auto *bbox_raw =
    reinterpret_cast<int32_t *>(node_output->output_tensors[order[0]]->sysMem[0].virAddr);
  auto *cls_raw =
    reinterpret_cast<float *>(node_output->output_tensors[order[1]]->sysMem[0].virAddr);
  auto *bbox_scale = reinterpret_cast<float *>(
    node_output->output_tensors[order[0]]->properties.scale.scaleData);

  const int stride = order[2];
  const int feature_w = config.input_width / stride;
  const int feature_h = config.input_height / stride;
  const float conf_threshold = SigmoidInverse(config.score_threshold);

  for (int h = 0; h < feature_h; ++h) {
    for (int w = 0; w < feature_w; ++w) {
      const float raw_score = cls_raw[0];
      cls_raw += kClassNum;

      const int32_t *cur_bbox = bbox_raw;
      bbox_raw += kReg * 4;

      if (raw_score < conf_threshold) {
        continue;
      }

      const float score = 1.0F / (1.0F + std::exp(-raw_score));

      float ltrb[4] = {0.0F, 0.0F, 0.0F, 0.0F};
      for (int i = 0; i < 4; ++i) {
        float sum = 0.0F;
        for (int j = 0; j < kReg; ++j) {
          const float dfl =
            std::exp(static_cast<float>(cur_bbox[kReg * i + j]) * bbox_scale[kReg * i + j]);
          ltrb[i] += dfl * static_cast<float>(j);
          sum += dfl;
        }
        ltrb[i] /= sum;
      }

      if (ltrb[0] + ltrb[2] <= 0.0F || ltrb[1] + ltrb[3] <= 0.0F) {
        continue;
      }

      const float x1 = (static_cast<float>(w) + 0.5F - ltrb[0]) * static_cast<float>(stride);
      const float y1 = (static_cast<float>(h) + 0.5F - ltrb[1]) * static_cast<float>(stride);
      const float x2 = (static_cast<float>(w) + 0.5F + ltrb[2]) * static_cast<float>(stride);
      const float y2 = (static_cast<float>(h) + 0.5F + ltrb[3]) * static_cast<float>(stride);

      bboxes.emplace_back(x1, y1, x2 - x1, y2 - y1);
      scores.emplace_back(score);
      parsed.push_back(ParsedDetection {0, x1, y1, x2, y2, score});
    }
  }
}

}  // namespace

int32_t ParseDetections(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<std::shared_ptr<YoloV8Detection>> &results) {
  if (!node_output || node_output->output_tensors.size() < 6) {
    return -1;
  }

  std::vector<cv::Rect2d> bboxes;
  std::vector<float> scores;
  std::vector<ParsedDetection> parsed;

  ParseScale(node_output, config, {0, 1, kStrides[0]}, parsed, bboxes, scores);
  ParseScale(node_output, config, {2, 3, kStrides[1]}, parsed, bboxes, scores);
  ParseScale(node_output, config, {4, 5, kStrides[2]}, parsed, bboxes, scores);

  std::vector<int> indices;
  cv::dnn::NMSBoxes(
    bboxes, scores, config.score_threshold, config.nms_threshold, indices, 1.0F, config.nms_top_k);

  results.clear();
  results.reserve(indices.size());
  for (const int index : indices) {
    const auto &det = parsed[index];
    results.emplace_back(std::make_shared<YoloV8Detection>(YoloV8Detection {
      det.id,
      det.xmin,
      det.ymin,
      det.xmax,
      det.ymax,
      det.score,
      config.class_name,
    }));
  }

  return 0;
}

}  // namespace rm_vehicle_detection
