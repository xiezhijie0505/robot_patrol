#ifndef ROBOT_YOLO_POSTPROCESS_H
#define ROBOT_YOLO_POSTPROCESS_H

#include <opencv2/core.hpp>
#include <vector>

struct DetBox {
  float x1, y1, x2, y2; /* 原图坐标 */
  float score;
  int class_id;
};

/**
 * 适配常见 YOLOv5 RKNN 单输出：[1, N, 5+nc] 或展平后的 float 数组。
 * letterbox 还原到原图尺寸。默认 COCO：person = 0。
 */
std::vector<DetBox> YoloV5Postprocess(
    const float *data, int num_props, int num_boxes, int num_classes,
    int model_w, int model_h, int origin_w, int origin_h, float conf_thres,
    float iou_thres, int filter_class /* -1=全部 */);

#endif
