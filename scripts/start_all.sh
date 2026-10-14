#!/bin/sh
# 一条命令启动「板端常用演示」（单终端）
#
# 默认启动：
#   1) alarms HTTP（后台）
#   2) YOLO 陌生人抓拍（前台，Ctrl+C 只停检测；alarms 仍在）
#
# 用法（推荐）：车放在起点后
#   cd /root/robot_patrol
#   sh scripts/start_all.sh
# 有 RK↔STM32 串口时会自动发 PATROL START，不必再手动发码。
#
# 可选：
#   CHASSIS_UART=/dev/ttyS3   指定串口（不设则自动试常见设备）
#   START_PATROL=0            只起检测/报警，不发巡逻
#   START_SMOKE_BRIDGE=1      同时起烟雾串口桥
#   START_RTSP_ONLY=1         仅旧版推流
#   ENABLE_STREAM=0           检测但不推流

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

echo "==== robot_patrol 一键启动 ===="
echo "请确认小车已放在巡逻起点。"

# 自动找串口（可被 CHASSIS_UART 覆盖）
if [ -z "$CHASSIS_UART" ]; then
  for d in /dev/ttyS3 /dev/ttyS1 /dev/ttyUSB0 /dev/ttyUSB1; do
    if [ -e "$d" ]; then
      CHASSIS_UART="$d"
      break
    fi
  done
fi

# --- 1. 报警 HTTP（后台）---
echo "[1/3] alarms HTTP ..."
sh "$ROOT/alarms/scripts/start_alarms.sh" || exit 1

# --- 2. 自动发 PATROL START（默认开）---
if [ -n "$CHASSIS_UART" ] && [ "${START_PATROL:-1}" = "1" ]; then
  echo "[2/3] 串口 $CHASSIS_UART → PATROL START"
  if command -v stty >/dev/null 2>&1; then
    stty -F "$CHASSIS_UART" 115200 raw -echo 2>/dev/null
  fi
  if printf 'PATROL START\n' > "$CHASSIS_UART" 2>/dev/null; then
    echo "  已发送，小车应开始矩形巡逻"
  else
    echo "  警告: 写串口失败，检查接线/设备名；可手动: printf 'PATROL START\\n' > $CHASSIS_UART"
  fi

  if [ "${START_SMOKE_BRIDGE:-0}" = "1" ] && command -v python3 >/dev/null 2>&1; then
    nohup python3 "$ROOT/alarms/tools/uart_alarm_bridge.py" "$CHASSIS_UART" \
      > /tmp/uart_alarm_bridge.log 2>&1 &
    echo $! > /tmp/uart_alarm_bridge.pid
    echo "  smoke bridge pid=$(cat /tmp/uart_alarm_bridge.pid)"
  fi
elif [ "${START_PATROL:-1}" = "0" ]; then
  echo "[2/3] START_PATROL=0，跳过巡逻指令"
else
  echo "[2/3] 未找到串口设备，跳过 PATROL START（可 export CHASSIS_UART=/dev/xxx）"
fi

# --- 3. 多线程检测（默认同时推流 ENABLE_STREAM=1）---
if [ "${START_RTSP_ONLY:-0}" = "1" ]; then
  echo "[3/3] 仅旧版 gst 推流（无 YOLO）"
  exec sh "$ROOT/streaming/scripts/start_rtsp.sh"
fi

echo "[3/3] 多线程 YOLO（采集+检测+可选推流）。浏览器: http://<板子IP>:8080/"
echo "VLC: rtsp://<板子IP>:8554/cam  （ENABLE_STREAM=1 时）"
echo "Ctrl+C 结束检测；alarms 继续跑。停止: sh scripts/stop_all.sh"
export ENABLE_STREAM="${ENABLE_STREAM:-1}"
exec sh "$ROOT/detect/scripts/start_detect.sh"
