#!/bin/sh
# 板端：多线程检测（可选同时推流）
# 用法：
#   cd /root/robot_patrol/detect
#   sh scripts/start_detect.sh
#
# 先起 alarms；若 ENABLE_STREAM=1（默认），会先起 mediamtx。

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO="$(cd "$ROOT/.." && pwd)"
BIN="${DETECT_BIN:-$ROOT/build/person_detect}"
MODEL="${YOLO_MODEL:-$ROOT/models/yolov5s.rknn}"
VIDEO="${VIDEO_DEV:-/dev/video9}"
ALARM_URL="${ALARM_URL:-http://127.0.0.1:8080}"
ENABLE_STREAM="${ENABLE_STREAM:-1}"

killall -9 dms_watchdog dms_capture dms_ai SmartCockpitDMS 2>/dev/null

if [ ! -x "$BIN" ]; then
  echo "缺少可执行文件: $BIN"
  echo "请在板端 cmake && make，见 README.md"
  exit 1
fi

if [ ! -f "$MODEL" ]; then
  echo "缺少模型: $MODEL"
  exit 1
fi

# 同时推流：先起 MediaMTX
if [ "$ENABLE_STREAM" = "1" ]; then
  MTX="$REPO/streaming/bin/mediamtx"
  CFG="$REPO/streaming/configs/mediamtx.yml"
  if [ -x "$MTX" ] && [ -f "$CFG" ]; then
    killall mediamtx 2>/dev/null
    sleep 1
    echo "启动 MediaMTX ..."
    "$MTX" "$CFG" > /tmp/mediamtx.log 2>&1 &
    echo $! > /tmp/mediamtx.pid
    sleep 1
    echo "VLC: rtsp://<板子IP>:8554/cam"
  else
    echo "警告: 未找到 mediamtx，仅检测不推流（$MTX）"
  fi
fi

export YOLO_MODEL="$MODEL"
export VIDEO_DEV="$VIDEO"
export ALARM_URL="$ALARM_URL"
export ENABLE_STREAM="$ENABLE_STREAM"
export CONF_THRES="${CONF_THRES:-0.45}"
export COOLDOWN_S="${COOLDOWN_S:-10}"
export BURST_N="${BURST_N:-3}"
export ALARM_POOL="${ALARM_POOL:-2}"

echo "YOLO_MODEL=$YOLO_MODEL"
echo "ENABLE_STREAM=$ENABLE_STREAM  ALARM_POOL=$ALARM_POOL"
exec "$BIN"
