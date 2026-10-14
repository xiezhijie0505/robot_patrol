#ifndef ROBOT_FRAME_QUEUE_H
#define ROBOT_FRAME_QUEUE_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <opencv2/core.hpp>
#include <utility>

/**
 * 有界阻塞队列：采集线程生产，检测线程消费。
 * 队列满时丢最旧帧（实时优先，避免越积越慢）。
 */
class FrameQueue {
public:
  explicit FrameQueue(size_t capacity = 3) : capacity_(capacity), closed_(false) {}

  void push(cv::Mat frame) {
    std::lock_guard<std::mutex> lock(mu_);
    if (closed_) {
      return;
    }
    if (q_.size() >= capacity_) {
      q_.pop_front();
    }
    q_.push_back(std::move(frame));
    cv_.notify_one();
  }

  /** 阻塞取一帧；关闭且空时返回 false */
  bool pop(cv::Mat *out) {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [&] { return closed_ || !q_.empty(); });
    if (q_.empty()) {
      return false;
    }
    *out = std::move(q_.front());
    q_.pop_front();
    return true;
  }

  void close() {
    std::lock_guard<std::mutex> lock(mu_);
    closed_ = true;
    cv_.notify_all();
  }

private:
  size_t capacity_;
  bool closed_;
  std::deque<cv::Mat> q_;
  std::mutex mu_;
  std::condition_variable cv_;
};

/**
 * 最新帧槽：推流线程只关心「当前最新画面」，不排队。
 */
class LatestFrame {
public:
  void set(const cv::Mat &frame) {
    std::lock_guard<std::mutex> lock(mu_);
    frame.copyTo(frame_);
    seq_++;
    cv_.notify_all();
  }

  /** 等到 seq 变化或超时；拿到拷贝返回 true */
  bool waitCopy(cv::Mat *out, uint64_t *inout_seq, int timeout_ms) {
    std::unique_lock<std::mutex> lock(mu_);
    if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] {
          return seq_ != *inout_seq && !frame_.empty();
        })) {
      return false;
    }
    frame_.copyTo(*out);
    *inout_seq = seq_;
    return true;
  }

private:
  cv::Mat frame_;
  uint64_t seq_ = 0;
  std::mutex mu_;
  std::condition_variable cv_;
};

#endif
