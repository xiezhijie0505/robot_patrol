#ifndef ROBOT_RKNN_WRAPPER_H
#define ROBOT_RKNN_WRAPPER_H

#include <opencv2/core.hpp>
#include <string>
#include <vector>

/**
 * 轻量 RKNN 封装（无 Qt，逻辑对齐 DMS RknnEngine）。
 * 编译加 -DHAVE_RKNN 并链接 librknnrt；否则 load 失败（方便 PC 侧先过编译）。
 */
class RknnWrapper {
public:
  RknnWrapper();
  ~RknnWrapper();

  RknnWrapper(const RknnWrapper &) = delete;
  RknnWrapper &operator=(const RknnWrapper &) = delete;

  bool load(const std::string &modelPath);
  void release();
  bool isReady() const { return ready_; }

  int inputWidth() const { return input_w_; }
  int inputHeight() const { return input_h_; }

  /** 输入 NHWC uint8 BGR；输出每个张量 CV_32F */
  bool infer(const cv::Mat &nhwc_u8, std::vector<cv::Mat> *outputs);

private:
  bool ready_;
  void *ctx_;
  int input_index_;
  int input_w_;
  int input_h_;
  int input_c_;
  bool input_nchw_;
  bool input_float_;
};

#endif
