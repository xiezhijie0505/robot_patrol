# detect · YOLO 陌生人抓拍（多线程）

与 DMS **多进程 + 共享内存** 区分：本模块用 **单进程多线程**。

## 线程模型（面试可讲）

```text
采集线程（生产者）
    ├─ push → FrameQueue（有界队列，满则丢旧帧）
    └─ set  → LatestFrame（最新帧槽）

检测线程（消费者）
    └─ pop FrameQueue → YOLO(RKNN)
           └─ 命中 person → ThreadPool.submit(抓拍+HTTP)

推流线程（消费者，可选）
    └─ wait LatestFrame → OpenCV/GStreamer → UDP:5004 → MediaMTX → RTSP
```

| 点 | 实现 |
|----|------|
| 生产者-消费者 | `FrameQueue`：`mutex` + `condition_variable` |
| 线程池 | `ThreadPool`：固定 worker 做写盘/POST，不堵推理 |
| 同时看视频+抓拍 | 采集只开一次摄像头，推流/检测各消费 |

V1「陌生人」= 检测到 COCO **person** → 抓拍入库，**不停车**。

## 依赖

- OpenCV（建议带 GStreamer，才能 `ENABLE_STREAM=1`）
- `librknnrt`
- 系统 `curl`（或编译加 `-DHAVE_CURL=ON`）
- MediaMTX（同时推流时）

## 编译

```sh
cd /root/robot_patrol/detect
mkdir -p build && cd build
cmake .. -DHAVE_RKNN=ON
make -j$(nproc)
```

## 运行

```sh
cd /root/robot_patrol/alarms && sh scripts/start_alarms.sh
cd /root/robot_patrol/detect
# 默认同时推流+检测
sh scripts/start_detect.sh
# 只要检测：
# ENABLE_STREAM=0 sh scripts/start_detect.sh
```

- 电脑 VLC：`rtsp://<板子IP>:8554/cam`
- 电脑浏览器：`http://<板子IP>:8080/`

## 环境变量

| 变量 | 默认 | 含义 |
|------|------|------|
| `YOLO_MODEL` | `models/yolov5s.rknn` | 模型 |
| `VIDEO_DEV` | `/dev/video9` | 摄像头 |
| `ALARM_URL` | `http://127.0.0.1:8080` | 报警服务 |
| `ENABLE_STREAM` | `1` | 同进程推流 |
| `ALARM_POOL` | `2` | 告警线程池大小 |
| `CONF_THRES` | `0.45` | 置信度 |
| `COOLDOWN_S` | `10` | 告警冷却 |
| `BURST_N` | `3` | 连拍张数（取最近 N 帧） |

模型说明见 `models/README.md`。
