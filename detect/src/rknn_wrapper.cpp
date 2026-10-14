#include "rknn_wrapper.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#ifdef HAVE_RKNN
#include "rknn_api.h"
#endif

RknnWrapper::RknnWrapper()
    : ready_(false),
      ctx_(nullptr),
      input_index_(0),
      input_w_(0),
      input_h_(0),
      input_c_(3),
      input_nchw_(false),
      input_float_(false) {}

RknnWrapper::~RknnWrapper() { release(); }

void RknnWrapper::release() {
#ifdef HAVE_RKNN
  if (ctx_) {
    rknn_destroy(reinterpret_cast<rknn_context>(ctx_));
    ctx_ = nullptr;
  }
#endif
  ready_ = false;
}

bool RknnWrapper::load(const std::string &modelPath) {
  release();
#ifndef HAVE_RKNN
  (void)modelPath;
  std::fprintf(stderr, "HAVE_RKNN 未定义，无法加载模型\n");
  return false;
#else
  std::ifstream ifs(modelPath, std::ios::binary);
  if (!ifs) {
    std::fprintf(stderr, "无法打开模型: %s\n", modelPath.c_str());
    return false;
  }
  std::vector<char> bytes((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  if (bytes.empty()) {
    std::fprintf(stderr, "模型为空: %s\n", modelPath.c_str());
    return false;
  }

  rknn_context ctx = 0;
  int ret = rknn_init(&ctx, bytes.data(), bytes.size(), 0, nullptr);
  if (ret != RKNN_SUCC) {
    std::fprintf(stderr, "rknn_init 失败 %d\n", ret);
    return false;
  }
  ctx_ = reinterpret_cast<void *>(ctx);

  rknn_input_output_num io_num;
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC || io_num.n_input < 1) {
    std::fprintf(stderr, "rknn_query IN_OUT_NUM 失败\n");
    release();
    return false;
  }

  rknn_tensor_attr in_attr;
  std::memset(&in_attr, 0, sizeof(in_attr));
  in_attr.index = 0;
  ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &in_attr, sizeof(in_attr));
  if (ret != RKNN_SUCC) {
    std::fprintf(stderr, "rknn_query INPUT_ATTR 失败\n");
    release();
    return false;
  }

  input_index_ = 0;
  input_nchw_ = (in_attr.fmt == RKNN_TENSOR_NCHW);
  input_float_ =
      (in_attr.type == RKNN_TENSOR_FLOAT16 || in_attr.type == RKNN_TENSOR_FLOAT32);
  if (input_nchw_) {
    input_c_ = static_cast<int>(in_attr.dims[1]);
    input_h_ = static_cast<int>(in_attr.dims[2]);
    input_w_ = static_cast<int>(in_attr.dims[3]);
  } else {
    input_h_ = static_cast<int>(in_attr.dims[1]);
    input_w_ = static_cast<int>(in_attr.dims[2]);
    input_c_ = static_cast<int>(in_attr.dims[3]);
  }

  ready_ = true;
  std::printf("RKNN ready: %dx%d input_nchw=%d\n", input_w_, input_h_,
              (int)input_nchw_);
  return true;
#endif
}

bool RknnWrapper::infer(const cv::Mat &nhwc_u8, std::vector<cv::Mat> *outputs) {
  if (!ready_ || !outputs) {
    return false;
  }
#ifndef HAVE_RKNN
  (void)nhwc_u8;
  return false;
#else
  rknn_context ctx = reinterpret_cast<rknn_context>(ctx_);

  rknn_input input;
  std::memset(&input, 0, sizeof(input));
  input.index = input_index_;
  input.type = input_float_ ? RKNN_TENSOR_FLOAT16 : RKNN_TENSOR_UINT8;
  input.fmt = input_nchw_ ? RKNN_TENSOR_NCHW : RKNN_TENSOR_NHWC;
  input.size = nhwc_u8.total() * nhwc_u8.elemSize();
  input.buf = nhwc_u8.data;
  input.pass_through = 0;

  int ret = rknn_inputs_set(ctx, 1, &input);
  if (ret != RKNN_SUCC) {
    std::fprintf(stderr, "rknn_inputs_set 失败 %d\n", ret);
    return false;
  }
  ret = rknn_run(ctx, nullptr);
  if (ret != RKNN_SUCC) {
    std::fprintf(stderr, "rknn_run 失败 %d\n", ret);
    return false;
  }

  rknn_input_output_num io_num;
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC) {
    return false;
  }

  std::vector<rknn_output> outs(io_num.n_output);
  std::memset(outs.data(), 0, outs.size() * sizeof(rknn_output));
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    outs[i].want_float = 1;
    outs[i].is_prealloc = 0;
  }
  ret = rknn_outputs_get(ctx, io_num.n_output, outs.data(), nullptr);
  if (ret != RKNN_SUCC) {
    std::fprintf(stderr, "rknn_outputs_get 失败 %d\n", ret);
    return false;
  }

  outputs->clear();
  for (uint32_t i = 0; i < io_num.n_output; ++i) {
    rknn_tensor_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.index = i;
    rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &attr, sizeof(attr));

    int n = 1;
    for (uint32_t d = 0; d < attr.n_dims; ++d) {
      n *= static_cast<int>(attr.dims[d]);
    }
    cv::Mat m(1, n, CV_32F);
    std::memcpy(m.data, outs[i].buf, n * sizeof(float));
    outputs->push_back(m);
  }

  rknn_outputs_release(ctx, io_num.n_output, outs.data());
  return true;
#endif
}
