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

int ShapeDim(const hbDNNTensorShape &shape, const int index, const int fallback) {
  if (index < 0 || index >= shape.numDimensions) {
    return fallback;
  }
  return shape.dimensionSize[index];
}

int64_t ShapeElementCount(const hbDNNTensorShape &shape) {
  int64_t count = 1;
  for (int i = 0; i < shape.numDimensions; ++i) {
    const int dim = shape.dimensionSize[i];
    if (dim <= 0) {
      return 0;
    }
    count *= static_cast<int64_t>(dim);
  }
  return count;
}

float NormalizeScore(const float raw_score) {
  if (raw_score >= 0.0F && raw_score <= 1.0F) {
    return raw_score;
  }
  return 1.0F / (1.0F + std::exp(-raw_score));
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
  const int anchors = feature_w * feature_h;

  const auto &cls_shape = node_output->output_tensors[order[1]]->properties.validShape;
  const int64_t cls_elements = ShapeElementCount(cls_shape);
  int class_num = 1;
  if (anchors > 0 && cls_elements > 0 && cls_elements % anchors == 0) {
    class_num = static_cast<int>(std::max<int64_t>(1, cls_elements / anchors));
  }

  for (int h = 0; h < feature_h; ++h) {
    for (int w = 0; w < feature_w; ++w) {
      float raw_score = cls_raw[0];
      for (int i = 1; i < class_num; ++i) {
        if (cls_raw[i] > raw_score) {
          raw_score = cls_raw[i];
        }
      }
      cls_raw += class_num;

      const int32_t *cur_bbox = bbox_raw;
      bbox_raw += kReg * 4;

      const float score = NormalizeScore(raw_score);
      if (score < config.score_threshold) {
        continue;
      }

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

void ParseSingleOutput(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<ParsedDetection> &parsed,
  std::vector<cv::Rect2d> &bboxes,
  std::vector<float> &scores) {
  auto &tensor = node_output->output_tensors[0];
  hbSysFlushMem(&(tensor->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);

  const auto &shape = tensor->properties.validShape;
  const int channels = ShapeDim(shape, 1, 5);
  const int anchors = ShapeDim(shape, 2, 8400);
  const int class_num = channels - 4;
  if (channels < 5 || anchors <= 0 || class_num <= 0) {
    return;
  }

  auto *raw = reinterpret_cast<float *>(tensor->sysMem[0].virAddr);
  for (int i = 0; i < anchors; ++i) {
    float score = NormalizeScore(raw[4 * anchors + i]);
    for (int c = 1; c < class_num; ++c) {
      score = std::max(score, NormalizeScore(raw[(4 + c) * anchors + i]));
    }
    if (score < config.score_threshold) {
      continue;
    }

    const float raw0 = raw[i];
    const float raw1 = raw[anchors + i];
    const float raw2 = raw[2 * anchors + i];
    const float raw3 = raw[3 * anchors + i];

    float x1 = 0.0F;
    float y1 = 0.0F;
    float x2 = 0.0F;
    float y2 = 0.0F;
    bool valid_box = false;

    // Primary path: interpret output as [cx, cy, w, h].
    float cx = raw0;
    float cy = raw1;
    float width = raw2;
    float height = raw3;
    if (width > 0.0F && height > 0.0F) {
      if (width <= 2.0F && height <= 2.0F && cx <= 2.0F && cy <= 2.0F) {
        cx *= static_cast<float>(config.input_width);
        cy *= static_cast<float>(config.input_height);
        width *= static_cast<float>(config.input_width);
        height *= static_cast<float>(config.input_height);
      }

      x1 = std::clamp(cx - width * 0.5F, 0.0F, static_cast<float>(config.input_width - 1));
      y1 = std::clamp(cy - height * 0.5F, 0.0F, static_cast<float>(config.input_height - 1));
      x2 = std::clamp(cx + width * 0.5F, 0.0F, static_cast<float>(config.input_width - 1));
      y2 = std::clamp(cy + height * 0.5F, 0.0F, static_cast<float>(config.input_height - 1));
      valid_box = (x2 > x1 && y2 > y1);
    }

    // Fallback path: interpret output as [x1, y1, x2, y2].
    if (!valid_box) {
      float ax = raw0;
      float ay = raw1;
      float bx = raw2;
      float by = raw3;
      if (std::max(std::fabs(ax), std::fabs(bx)) <= 2.0F) {
        ax *= static_cast<float>(config.input_width);
        bx *= static_cast<float>(config.input_width);
      }
      if (std::max(std::fabs(ay), std::fabs(by)) <= 2.0F) {
        ay *= static_cast<float>(config.input_height);
        by *= static_cast<float>(config.input_height);
      }

      x1 = std::clamp(std::min(ax, bx), 0.0F, static_cast<float>(config.input_width - 1));
      y1 = std::clamp(std::min(ay, by), 0.0F, static_cast<float>(config.input_height - 1));
      x2 = std::clamp(std::max(ax, bx), 0.0F, static_cast<float>(config.input_width - 1));
      y2 = std::clamp(std::max(ay, by), 0.0F, static_cast<float>(config.input_height - 1));
      valid_box = (x2 > x1 && y2 > y1);
    }

    if (!valid_box) {
      continue;
    }

    bboxes.emplace_back(x1, y1, x2 - x1, y2 - y1);
    scores.emplace_back(score);
    parsed.push_back(ParsedDetection {0, x1, y1, x2, y2, score});
  }
}

}  // namespace

int32_t ParseDetections(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<std::shared_ptr<YoloV8Detection>> &results) {
  if (!node_output || node_output->output_tensors.empty()) {
    return -1;
  }

  std::vector<cv::Rect2d> bboxes;
  std::vector<float> scores;
  std::vector<ParsedDetection> parsed;

  if (node_output->output_tensors.size() == 1) {
    ParseSingleOutput(node_output, config, parsed, bboxes, scores);
  } else if (node_output->output_tensors.size() >= 6) {
    ParseScale(node_output, config, {0, 1, kStrides[0]}, parsed, bboxes, scores);
    ParseScale(node_output, config, {2, 3, kStrides[1]}, parsed, bboxes, scores);
    ParseScale(node_output, config, {4, 5, kStrides[2]}, parsed, bboxes, scores);
  } else {
    return -1;
  }

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
