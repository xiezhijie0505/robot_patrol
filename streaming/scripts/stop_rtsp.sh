#!/bin/sh
# 停止板端推流与 MediaMTX

killall -9 gst-launch-1.0 2>/dev/null
if [ -f /tmp/mediamtx.pid ]; then
  kill "$(cat /tmp/mediamtx.pid)" 2>/dev/null
  rm -f /tmp/mediamtx.pid
fi
killall mediamtx 2>/dev/null
echo "已停止 gst-launch / mediamtx"
