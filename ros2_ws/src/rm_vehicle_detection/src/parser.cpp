#include "parser.h"

#include <opencv2/dnn/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <memory>
#include <vector>

namespace rm_vehicle_detection {
namespace {

using hobot::dnn_node::DNNTensor;

constexpr int kDebugCandidatesPerCall = 3;
constexpr int kExpectedChannels = 5;
constexpr int kTensorLayoutNCHW = 2;
constexpr int kTensorLayoutNHWC = 0;

struct ParsedDetection {
  int id;
  float xmin;
  float ymin;
  float xmax;
  float ymax;
  float score;
};

float NormalizeScore(const float raw_score) {
  if (raw_score >= 0.0F && raw_score <= 1.0F) {
    return raw_score;
  }
  return 1.0F / (1.0F + std::exp(-raw_score));
}

struct OutputTensorAccessor {
  const float *raw = nullptr;
  hbDNNTensorProperties properties {};
  int channels = 0;
  int anchors = 0;
  bool is_nchw = true;
};

bool BuildOutputTensorAccessor(
  const std::shared_ptr<DNNTensor> &tensor,
  OutputTensorAccessor &accessor) {
  if (!tensor || !tensor->sysMem[0].virAddr) {
    return false;
  }

  accessor.raw = reinterpret_cast<const float *>(tensor->sysMem[0].virAddr);
  accessor.properties = tensor->properties;

  const auto &valid = accessor.properties.validShape.dimensionSize;
  const bool nchw =
    accessor.properties.tensorLayout == kTensorLayoutNCHW ||
    valid[1] == kExpectedChannels;
  const bool nhwc =
    accessor.properties.tensorLayout == kTensorLayoutNHWC ||
    valid[3] == kExpectedChannels;

  if (nchw) {
    accessor.is_nchw = true;
    accessor.channels = valid[1];
    accessor.anchors = valid[2] * std::max(valid[3], 1);
    return accessor.channels >= kExpectedChannels && accessor.anchors > 0;
  }

  if (nhwc) {
    accessor.is_nchw = false;
    accessor.channels = valid[3];
    accessor.anchors = valid[1] * std::max(valid[2], 1);
    return accessor.channels >= kExpectedChannels && accessor.anchors > 0;
  }

  return false;
}

bool ReadOutputValue(
  const OutputTensorAccessor &accessor,
  const int channel,
  const int anchor,
  float &value) {
  if (!accessor.raw || channel < 0 || channel >= accessor.channels || anchor < 0 ||
    anchor >= accessor.anchors)
  {
    return false;
  }

  const auto &aligned = accessor.properties.alignedShape.dimensionSize;
  const auto &valid = accessor.properties.validShape.dimensionSize;

  std::size_t offset = 0U;
  if (accessor.is_nchw) {
    const int aligned_h = std::max(aligned[2], 1);
    const int aligned_w = std::max(aligned[3], 1);
    const int valid_w = std::max(valid[3], 1);
    const int h = anchor / valid_w;
    const int w = anchor % valid_w;
    offset = static_cast<std::size_t>(channel) * static_cast<std::size_t>(aligned_h) *
      static_cast<std::size_t>(aligned_w) +
      static_cast<std::size_t>(h) * static_cast<std::size_t>(aligned_w) +
      static_cast<std::size_t>(w);
  } else {
    const int aligned_h = std::max(aligned[1], 1);
    const int aligned_w = std::max(aligned[2], 1);
    const int aligned_c = std::max(aligned[3], 1);
    const int valid_w = std::max(valid[2], 1);
    const int h = anchor / valid_w;
    const int w = anchor % valid_w;
    offset = static_cast<std::size_t>(h) * static_cast<std::size_t>(aligned_w) *
      static_cast<std::size_t>(aligned_c) +
      static_cast<std::size_t>(w) * static_cast<std::size_t>(aligned_c) +
      static_cast<std::size_t>(channel);
  }

  value = accessor.raw[offset];
  return true;
}

DebugAnchorInfo ReadBestAnchor(const OutputTensorAccessor &accessor) {
  DebugAnchorInfo info;
  if (!accessor.raw || accessor.channels < kExpectedChannels || accessor.anchors <= 0) {
    return info;
  }

  float best_score = -1.0F;
  int best_anchor = -1;
  for (int anchor = 0; anchor < accessor.anchors; ++anchor) {
    float raw_score = 0.0F;
    if (!ReadOutputValue(accessor, 4, anchor, raw_score)) {
      continue;
    }
    const float score = NormalizeScore(raw_score);
    if (score > best_score) {
      best_score = score;
      best_anchor = anchor;
    }
  }

  if (best_anchor < 0) {
    return info;
  }

  info.anchor = best_anchor;
  ReadOutputValue(accessor, 0, best_anchor, info.raw0);
  ReadOutputValue(accessor, 1, best_anchor, info.raw1);
  ReadOutputValue(accessor, 2, best_anchor, info.raw2);
  ReadOutputValue(accessor, 3, best_anchor, info.raw3);
  info.score = best_score;
  return info;
}

std::vector<DebugAnchorInfo> ReadTopAnchors(
  const OutputTensorAccessor &accessor,
  const std::size_t top_k) {
  std::vector<DebugAnchorInfo> infos;
  if (!accessor.raw || accessor.channels < kExpectedChannels || accessor.anchors <= 0 ||
    top_k == 0U)
  {
    return infos;
  }

  std::vector<int> indices(static_cast<std::size_t>(accessor.anchors));
  for (int anchor = 0; anchor < accessor.anchors; ++anchor) {
    indices[anchor] = anchor;
  }

  const auto limit = std::min<std::size_t>(top_k, indices.size());
  std::partial_sort(
    indices.begin(),
    indices.begin() + static_cast<std::ptrdiff_t>(limit),
    indices.end(),
    [&accessor](const int lhs, const int rhs) {
      float lhs_score = 0.0F;
      float rhs_score = 0.0F;
      ReadOutputValue(accessor, 4, lhs, lhs_score);
      ReadOutputValue(accessor, 4, rhs, rhs_score);
      return NormalizeScore(lhs_score) > NormalizeScore(rhs_score);
    });

  infos.reserve(limit);
  for (std::size_t i = 0; i < limit; ++i) {
    const int anchor = indices[i];
    DebugAnchorInfo info;
    info.anchor = anchor;
    ReadOutputValue(accessor, 0, anchor, info.raw0);
    ReadOutputValue(accessor, 1, anchor, info.raw1);
    ReadOutputValue(accessor, 2, anchor, info.raw2);
    ReadOutputValue(accessor, 3, anchor, info.raw3);
    float raw_score = 0.0F;
    ReadOutputValue(accessor, 4, anchor, raw_score);
    info.score = NormalizeScore(raw_score);
    infos.emplace_back(info);
  }
  return infos;
}

float MaybeDenormalize(const float value, const int size) {
  if (std::fabs(value) <= 2.0F) {
    return value * static_cast<float>(size);
  }
  return value;
}

bool DecodeAnchorBox(
  const float raw0,
  const float raw1,
  const float raw2,
  const float raw3,
  const YoloV8ParserConfig &config,
  float &x1,
  float &y1,
  float &x2,
  float &y2) {
  const float max_x = static_cast<float>(config.input_width - 1);
  const float max_y = static_cast<float>(config.input_height - 1);

  if (config.box_format == YoloBoxFormat::kCxcywh) {
    const float cx = MaybeDenormalize(raw0, config.input_width);
    const float cy = MaybeDenormalize(raw1, config.input_height);
    const float width = std::max(0.0F, MaybeDenormalize(raw2, config.input_width));
    const float height = std::max(0.0F, MaybeDenormalize(raw3, config.input_height));

    x1 = std::clamp(cx - width * 0.5F, 0.0F, max_x);
    y1 = std::clamp(cy - height * 0.5F, 0.0F, max_y);
    x2 = std::clamp(cx + width * 0.5F, 0.0F, max_x);
    y2 = std::clamp(cy + height * 0.5F, 0.0F, max_y);
    return width > 0.0F && height > 0.0F && x2 > x1 && y2 > y1;
  }

  x1 = std::clamp(MaybeDenormalize(raw0, config.input_width), 0.0F, max_x);
  y1 = std::clamp(MaybeDenormalize(raw1, config.input_height), 0.0F, max_y);
  x2 = std::clamp(MaybeDenormalize(raw2, config.input_width), 0.0F, max_x);
  y2 = std::clamp(MaybeDenormalize(raw3, config.input_height), 0.0F, max_y);
  return x2 > x1 && y2 > y1;
}

int32_t ParseSingleOutput(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<ParsedDetection> &parsed,
  std::vector<cv::Rect2d> &bboxes,
  std::vector<float> &scores) {
  if (!node_output || node_output->output_tensors.empty() || !node_output->output_tensors[0]) {
    return -1;
  }

  auto &tensor = node_output->output_tensors[0];
  hbSysFlushMem(&(tensor->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);

  OutputTensorAccessor accessor;
  if (!BuildOutputTensorAccessor(tensor, accessor)) {
    return -1;
  }

  int debug_candidates_left = kDebugCandidatesPerCall;

  for (int anchor = 0; anchor < accessor.anchors; ++anchor) {
    float raw0 = 0.0F;
    float raw1 = 0.0F;
    float raw2 = 0.0F;
    float raw3 = 0.0F;
    float raw_score = 0.0F;
    if (
      !ReadOutputValue(accessor, 0, anchor, raw0) ||
      !ReadOutputValue(accessor, 1, anchor, raw1) ||
      !ReadOutputValue(accessor, 2, anchor, raw2) ||
      !ReadOutputValue(accessor, 3, anchor, raw3) ||
      !ReadOutputValue(accessor, 4, anchor, raw_score))
    {
      continue;
    }
    const float score = NormalizeScore(raw_score);

    if (score < config.score_threshold) {
      continue;
    }

    float x1 = 0.0F;
    float y1 = 0.0F;
    float x2 = 0.0F;
    float y2 = 0.0F;

    if (!DecodeAnchorBox(raw0, raw1, raw2, raw3, config, x1, y1, x2, y2)) {
      continue;
    }

    if (debug_candidates_left > 0) {
      std::fprintf(
        stderr,
        "[rm_vehicle_detection_parser] raw candidate anchor=%d score=%.4f raw0=%.4f raw1=%.4f raw2=%.4f raw3=%.4f format=%s -> x1=%.1f y1=%.1f x2=%.1f y2=%.1f\n",
        anchor,
        score,
        raw0,
        raw1,
        raw2,
        raw3,
        config.box_format == YoloBoxFormat::kCxcywh ? "cxcywh" : "xyxy",
        x1,
        y1,
        x2,
        y2);
      std::fflush(stderr);
      --debug_candidates_left;
    }

    bboxes.emplace_back(x1, y1, x2 - x1, y2 - y1);
    scores.emplace_back(score);
    parsed.push_back(ParsedDetection {0, x1, y1, x2, y2, score});
  }

  return 0;
}

}  // namespace

int32_t ParseDetections(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  const YoloV8ParserConfig &config,
  std::vector<std::shared_ptr<YoloV8Detection>> &results) {
  std::vector<cv::Rect2d> bboxes;
  std::vector<float> scores;
  std::vector<ParsedDetection> parsed;

  if (ParseSingleOutput(node_output, config, parsed, bboxes, scores) != 0) {
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

DebugAnchorInfo FindBestAnchor(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output) {
  if (!node_output || node_output->output_tensors.empty() || !node_output->output_tensors[0]) {
    return {};
  }

  auto &tensor = node_output->output_tensors[0];
  hbSysFlushMem(&(tensor->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);
  OutputTensorAccessor accessor;
  if (!BuildOutputTensorAccessor(tensor, accessor)) {
    return {};
  }
  return ReadBestAnchor(accessor);
}

std::vector<DebugAnchorInfo> FindTopAnchors(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output,
  std::size_t top_k) {
  if (
    !node_output || node_output->output_tensors.empty() || !node_output->output_tensors[0] ||
    top_k == 0U)
  {
    return {};
  }

  auto &tensor = node_output->output_tensors[0];
  hbSysFlushMem(&(tensor->sysMem[0]), HB_SYS_MEM_CACHE_INVALIDATE);
  OutputTensorAccessor accessor;
  if (!BuildOutputTensorAccessor(tensor, accessor)) {
    return {};
  }
  return ReadTopAnchors(accessor, top_k);
}

OutputTensorDebugInfo GetOutputTensorDebugInfo(
  const std::shared_ptr<hobot::dnn_node::DnnNodeOutput> &node_output) {
  OutputTensorDebugInfo info;
  if (!node_output || node_output->output_tensors.empty() || !node_output->output_tensors[0]) {
    return info;
  }

  OutputTensorAccessor accessor;
  if (!BuildOutputTensorAccessor(node_output->output_tensors[0], accessor)) {
    return info;
  }

  info.layout = accessor.properties.tensorLayout;
  for (int i = 0; i < 4; ++i) {
    info.valid_shape[i] = accessor.properties.validShape.dimensionSize[i];
    info.aligned_shape[i] = accessor.properties.alignedShape.dimensionSize[i];
  }
  info.channels = accessor.channels;
  info.anchors = accessor.anchors;
  return info;
}

}  // namespace rm_vehicle_detection
