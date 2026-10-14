#!/bin/sh
# 在 RK3568 板端执行（BusyBox ash / bash 均可）
# 用法：
#   cd /root/robot_patrol/streaming
#   sh scripts/start_rtsp.sh
#
# 电脑 VLC：rtsp://<板子IP>:8554/cam

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/mediamtx"
CFG="$ROOT/configs/mediamtx.yml"
LOG="${MEDIAMTX_LOG:-/tmp/mediamtx.log}"
VIDEO="${VIDEO_DEV:-/dev/video9}"

if [ ! -x "$BIN" ]; then
  echo "缺少可执行文件: $BIN"
  echo "请将 linux_arm64 的 mediamtx 放到 bin/ 并 chmod +x"
  exit 1
fi

if [ ! -f "$CFG" ]; then
  echo "缺少配置: $CFG"
  exit 1
fi

# 释放摄像头（若曾开 DMS）
killall -9 dms_watchdog dms_capture dms_ai SmartCockpitDMS 2>/dev/null

# 已在跑则先停
killall mediamtx 2>/dev/null
sleep 1

echo "启动 MediaMTX ..."
"$BIN" "$CFG" >"$LOG" 2>&1 &
echo $! > /tmp/mediamtx.pid
sleep 1
tail -n 15 "$LOG"

echo ""
echo "启动 GStreamer 推流（前台，Ctrl+C 结束推流；MediaMTX 仍在后台）..."
echo "VIDEO=$VIDEO"

gst-launch-1.0 -v \
  v4l2src device="$VIDEO" ! \
  videoconvert ! video/x-raw,format=NV12,width=640,height=480,framerate=30/1 ! \
  mpph264enc ! h264parse ! mpegtsmux ! \
  udpsink host=127.0.0.1 port=5004
