#!/bin/sh
# 板端演示：报警 HTTP + YOLO 陌生人抓拍（不含推流，避免抢摄像头）
# 用法：
#   cd /root/robot_patrol
#   sh scripts/start_demo_detect.sh

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== 1/2 启动 alarms HTTP ==="
sh "$ROOT/alarms/scripts/start_alarms.sh" || exit 1

echo ""
echo "=== 2/2 启动 YOLO 检测（前台，Ctrl+C 结束）==="
echo "浏览器查报警: http://<板子IP>:8080/"
echo "陌生人：抓拍入库，不停车"
sh "$ROOT/detect/scripts/start_detect.sh"
