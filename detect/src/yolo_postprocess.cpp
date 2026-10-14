#include "yolo_postprocess.h"

#include <algorithm>
#include <cmath>
#include <vector>

static float IoU(const DetBox &a, const DetBox &b) {
  float xx1 = std::max(a.x1, b.x1);
  float yy1 = std::max(a.y1, b.y1);
  float xx2 = std::min(a.x2, b.x2);
  float yy2 = std::min(a.y2, b.y2);
  float w = std::max(0.f, xx2 - xx1);
  float h = std::max(0.f, yy2 - yy1);
  float inter = w * h;
  float area_a = std::max(0.f, a.x2 - a.x1) * std::max(0.f, a.y2 - a.y1);
  float area_b = std::max(0.f, b.x2 - b.x1) * std::max(0.f, b.y2 - b.y1);
  float uni = area_a + area_b - inter + 1e-6f;
  return inter / uni;
}

static void Nms(std::vector<DetBox> *boxes, float iou_thres) {
  std::sort(boxes->begin(), boxes->end(),
            [](const DetBox &a, const DetBox &b) { return a.score > b.score; });
  std::vector<int> keep;
  std::vector<char> removed(boxes->size(), 0);
  for (size_t i = 0; i < boxes->size(); ++i) {
    if (removed[i]) {
      continue;
    }
    keep.push_back(static_cast<int>(i));
    for (size_t j = i + 1; j < boxes->size(); ++j) {
      if (removed[j]) {
        continue;
      }
      if ((*boxes)[i].class_id != (*boxes)[j].class_id) {
        continue;
      }
      if (IoU((*boxes)[i], (*boxes)[j]) > iou_thres) {
        removed[j] = 1;
      }
    }
  }
  std::vector<DetBox> out;
  out.reserve(keep.size());
  for (int idx : keep) {
    out.push_back((*boxes)[idx]);
  }
  *boxes = out;
}

std::vector<DetBox> YoloV5Postprocess(const float *data, int num_props,
                                      int num_boxes, int num_classes, int model_w,
                                      int model_h, int origin_w, int origin_h,
                                      float conf_thres, float iou_thres,
                                      int filter_class) {
  std::vector<DetBox> boxes;
  if (!data || num_props < 5 + num_classes || num_boxes <= 0) {
    return boxes;
  }

  /* letterbox：等比缩放到 model，居中 pad */
  float r = std::min(static_cast<float>(model_w) / origin_w,
                     static_cast<float>(model_h) / origin_h);
  float new_unpad_w = origin_w * r;
  float new_unpad_h = origin_h * r;
  float dw = (model_w - new_unpad_w) * 0.5f;
  float dh = (model_h - new_unpad_h) * 0.5f;

  for (int i = 0; i < num_boxes; ++i) {
    const float *row = data + i * num_props;
    float obj = row[4];
    if (obj < conf_thres) {
      continue;
    }
    int best_c = 0;
    float best_s = row[5];
    for (int c = 1; c < num_classes; ++c) {
      if (row[5 + c] > best_s) {
        best_s = row[5 + c];
        best_c = c;
      }
    }
    float score = obj * best_s;
    if (score < conf_thres) {
      continue;
    }
    if (filter_class >= 0 && best_c != filter_class) {
      continue;
    }

    float cx = row[0];
    float cy = row[1];
    float bw = row[2];
    float bh = row[3];
    float x1 = (cx - bw * 0.5f - dw) / r;
    float y1 = (cy - bh * 0.5f - dh) / r;
    float x2 = (cx + bw * 0.5f - dw) / r;
    float y2 = (cy + bh * 0.5f - dh) / r;

    x1 = std::max(0.f, std::min(x1, static_cast<float>(origin_w - 1)));
    y1 = std::max(0.f, std::min(y1, static_cast<float>(origin_h - 1)));
    x2 = std::max(0.f, std::min(x2, static_cast<float>(origin_w - 1)));
    y2 = std::max(0.f, std::min(y2, static_cast<float>(origin_h - 1)));

    DetBox b;
    b.x1 = x1;
    b.y1 = y1;
    b.x2 = x2;
    b.y2 = y2;
    b.score = score;
    b.class_id = best_c;
    boxes.push_back(b);
  }

  Nms(&boxes, iou_thres);
  return boxes;
}
