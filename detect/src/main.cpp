/**
 * 巡逻视觉（多线程，区别于 DMS 多进程）：
 *
 *   采集线程（生产者）──► FrameQueue ──► 检测线程（消费者 / YOLO）
 *         │                                    │
 *         └──► LatestFrame ──► 推流线程         └──► ThreadPool（抓拍+HTTP）
 *
 * 陌生人：只抓拍入库，不停车。
 * ENABLE_STREAM=1 时同一进程内推 UDP→MediaMTX，可与检测同时。
 */

#include "alarm_client.h"
#include "frame_queue.h"
#include "rknn_wrapper.h"
#include "thread_pool.h"
#include "yolo_postprocess.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

static std::string EnvOr(const char *key, const char *defv) {
  const char *v = std::getenv(key);
  return (v && v[0]) ? std::string(v) : std::string(defv);
}

static float EnvFloat(const char *key, float defv) {
  const char *v = std::getenv(key);
  return (v && v[0]) ? static_cast<float>(std::atof(v)) : defv;
}

static int EnvInt(const char *key, int defv) {
  const char *v = std::getenv(key);
  return (v && v[0]) ? std::atoi(v) : defv;
}

static cv::Mat Letterbox(const cv::Mat &bgr, int model_w, int model_h) {
  float r = std::min(static_cast<float>(model_w) / bgr.cols,
                     static_cast<float>(model_h) / bgr.rows);
  int new_w = static_cast<int>(bgr.cols * r);
  int new_h = static_cast<int>(bgr.rows * r);
  cv::Mat resized;
  cv::resize(bgr, resized, cv::Size(new_w, new_h));
  cv::Mat out(model_h, model_w, CV_8UC3, cv::Scalar(114, 114, 114));
  int dx = (model_w - new_w) / 2;
  int dy = (model_h - new_h) / 2;
  resized.copyTo(out(cv::Rect(dx, dy, new_w, new_h)));
  return out;
}

struct AlarmJob {
  std::vector<cv::Mat> frames;
  float score;
  std::string note;
  std::string alarm_url;
};

static void RunAlarmJob(const AlarmJob &job) {
  std::vector<std::string> paths;
  paths.reserve(job.frames.size());
  const long ts = static_cast<long>(std::time(nullptr));
  for (size_t i = 0; i < job.frames.size(); ++i) {
    char path[128];
    std::snprintf(path, sizeof(path), "/tmp/stranger_%ld_%zu.jpg", ts, i);
    if (cv::imwrite(path, job.frames[i])) {
      paths.push_back(path);
    }
  }
  if (paths.empty()) {
    return;
  }
  if (PostStrangerAlarm(job.alarm_url, job.score, job.note, paths)) {
    std::printf("[pool] stranger alarm score=%.2f files=%zu\n", job.score,
                paths.size());
  } else {
    std::fprintf(stderr, "[pool] POST failed\n");
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  const std::string model = EnvOr("YOLO_MODEL", "");
  const std::string video = EnvOr("VIDEO_DEV", "/dev/video9");
  const std::string alarm_url = EnvOr("ALARM_URL", "http://127.0.0.1:8080");
  const float conf = EnvFloat("CONF_THRES", 0.45f);
  const int cooldown_s = EnvInt("COOLDOWN_S", 10);
  const int burst_n = EnvInt("BURST_N", 3);
  const int pool_n = EnvInt("ALARM_POOL", 2);
  const int enable_stream = EnvInt("ENABLE_STREAM", 1);
  const int person_class = 0;

  if (model.empty()) {
    std::fprintf(stderr, "请设置 YOLO_MODEL=/path/to/yolov5s.rknn\n");
    return 1;
  }

  RknnWrapper rknn;
  if (!rknn.load(model)) {
    return 1;
  }
  const int model_w = rknn.inputWidth() > 0 ? rknn.inputWidth() : 640;
  const int model_h = rknn.inputHeight() > 0 ? rknn.inputHeight() : 640;

  FrameQueue detect_q(3);
  LatestFrame latest;
  ThreadPool pool(static_cast<size_t>(pool_n));
  std::atomic<bool> running{true};

  std::mutex cooldown_mu;
  auto last_alarm = std::chrono::steady_clock::now() -
                    std::chrono::seconds(cooldown_s);

  /* -------- 1) 采集线程：生产者 -------- */
  std::thread capture_th([&] {
    cv::VideoCapture cap(video, cv::CAP_V4L2);
    if (!cap.isOpened()) {
      cap.open(video);
    }
    if (!cap.isOpened()) {
      std::fprintf(stderr, "[capture] 无法打开 %s\n", video.c_str());
      running = false;
      detect_q.close();
      return;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    std::printf("[capture] producer on %s\n", video.c_str());

    cv::Mat frame;
    while (running) {
      if (!cap.read(frame) || frame.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }
      latest.set(frame);
      detect_q.push(frame.clone());
    }
    detect_q.close();
  });

  /* -------- 2) 推流线程：消费 LatestFrame → MediaMTX -------- */
  std::thread stream_th;
  if (enable_stream) {
    stream_th = std::thread([&] {
      /* OpenCV+GStreamer：BGR → 硬编 → UDP:5004（与 mediamtx.yml 一致） */
      const std::string pipe =
          "appsrc ! videoconvert ! video/x-raw,format=NV12 ! "
          "mpph264enc ! h264parse ! mpegtsmux ! "
          "udpsink host=127.0.0.1 port=5004";
      cv::VideoWriter writer;
      bool opened = false;
      uint64_t seq = 0;
      cv::Mat frame;
      std::printf("[stream] consumer trying GStreamer appsrc→UDP:5004\n");

      while (running) {
        if (!latest.waitCopy(&frame, &seq, 500)) {
          continue;
        }
        if (!opened) {
          opened = writer.open(pipe, cv::CAP_GSTREAMER, 0, 25.0,
                               cv::Size(frame.cols, frame.rows), true);
          if (!opened) {
            std::fprintf(stderr,
                         "[stream] VideoWriter 打开失败（OpenCV 需带 "
                         "GStreamer；可 ENABLE_STREAM=0）\n");
            return;
          }
          std::printf("[stream] pushing %dx%d\n", frame.cols, frame.rows);
        }
        writer.write(frame);
      }
      if (opened) {
        writer.release();
      }
    });
  }

  /* -------- 3) 检测线程：消费者 + 提交线程池 -------- */
  std::thread detect_th([&] {
    std::printf("[detect] consumer + alarm thread pool(%d)\n", pool_n);
    cv::Mat frame;
    /* 连拍用：最近几帧小缓冲 */
    std::deque<cv::Mat> recent;
    const size_t recent_max = static_cast<size_t>(std::max(burst_n, 1));

    while (running && detect_q.pop(&frame)) {
      if (recent.size() >= recent_max) {
        recent.pop_front();
      }
      recent.push_back(frame.clone());

      cv::Mat input = Letterbox(frame, model_w, model_h);
      std::vector<cv::Mat> outs;
      if (!rknn.infer(input, &outs) || outs.empty()) {
        continue;
      }

      const float *data = outs[0].ptr<float>();
      const int total = outs[0].cols * outs[0].rows;
      const int num_classes = 80;
      const int num_props = 5 + num_classes;
      if (total % num_props != 0) {
        static int warn = 0;
        if (warn++ < 3) {
          std::fprintf(stderr, "[detect] 输出长度与 YOLOv5 85 列不匹配: %d\n",
                       total);
        }
        continue;
      }
      const int num_boxes = total / num_props;
      std::vector<DetBox> dets = YoloV5Postprocess(
          data, num_props, num_boxes, num_classes, model_w, model_h, frame.cols,
          frame.rows, conf, 0.45f, person_class);
      if (dets.empty()) {
        continue;
      }

      {
        std::lock_guard<std::mutex> lock(cooldown_mu);
        auto now = std::chrono::steady_clock::now();
        if (now - last_alarm < std::chrono::seconds(cooldown_s)) {
          continue;
        }
        last_alarm = now;
      }

      float best = 0.f;
      for (const auto &d : dets) {
        if (d.score > best) {
          best = d.score;
        }
      }

      AlarmJob job;
      job.score = best;
      job.alarm_url = alarm_url;
      char note[64];
      std::snprintf(note, sizeof(note), "YOLO person n=%zu", dets.size());
      job.note = note;
      for (const auto &f : recent) {
        job.frames.push_back(f.clone());
      }

      pool.submit([job] { RunAlarmJob(job); });
      std::printf("[detect] person → submit alarm job score=%.2f\n", best);
    }
  });

  std::printf("multithread detect running model=%s alarm=%s stream=%d\n",
              model.c_str(), alarm_url.c_str(), enable_stream);
  std::printf("VLC: rtsp://<板子IP>:8554/cam  (需先起 mediamtx)\n");

  /* 主线程等 Ctrl+C：简单阻塞在 detect 结束 */
  detect_th.join();
  running = false;
  detect_q.close();
  if (capture_th.joinable()) {
    capture_th.join();
  }
  if (stream_th.joinable()) {
    stream_th.join();
  }
  pool.shutdown();
  return 0;
}
