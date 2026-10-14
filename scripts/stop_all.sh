#!/bin/sh
# 停掉一键脚本拉起的后台进程
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

sh "$ROOT/alarms/scripts/stop_alarms.sh" 2>/dev/null
if [ -f /tmp/uart_alarm_bridge.pid ]; then
  kill "$(cat /tmp/uart_alarm_bridge.pid)" 2>/dev/null
  rm -f /tmp/uart_alarm_bridge.pid
  echo "stopped uart_alarm_bridge"
fi
pkill -f "python3 .*uart_alarm_bridge" 2>/dev/null
pkill -f "person_detect" 2>/dev/null
killall mediamtx 2>/dev/null
echo "stop_all done"
