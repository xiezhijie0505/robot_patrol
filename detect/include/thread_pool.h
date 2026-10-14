#ifndef ROBOT_THREAD_POOL_H
#define ROBOT_THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * 固定大小线程池：告警抓拍写盘 + HTTP POST（I/O）丢进池里，
 * 不堵检测线程。
 */
class ThreadPool {
public:
  explicit ThreadPool(size_t n) : stop_(false) {
    if (n < 1) {
      n = 1;
    }
    workers_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      workers_.emplace_back([this] { workerLoop(); });
    }
  }

  ~ThreadPool() { shutdown(); }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  void submit(std::function<void()> job) {
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (stop_) {
        return;
      }
      jobs_.push(std::move(job));
    }
    cv_.notify_one();
  }

  void shutdown() {
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (stop_) {
        return;
      }
      stop_ = true;
    }
    cv_.notify_all();
    for (auto &t : workers_) {
      if (t.joinable()) {
        t.join();
      }
    }
    workers_.clear();
  }

private:
  void workerLoop() {
    for (;;) {
      std::function<void()> job;
      {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [&] { return stop_ || !jobs_.empty(); });
        if (stop_ && jobs_.empty()) {
          return;
        }
        job = std::move(jobs_.front());
        jobs_.pop();
      }
      job();
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> jobs_;
  std::mutex mu_;
  std::condition_variable cv_;
  bool stop_;
};

#endif
