#!/bin/sh
# 停止报警 HTTP 服务
PID_FILE="${ALARMS_PID:-/tmp/alarms_http.pid}"

if [ -f "$PID_FILE" ]; then
  pid="$(cat "$PID_FILE" 2>/dev/null)"
  if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
    kill "$pid" 2>/dev/null
    echo "已停止 pid=$pid"
  else
    echo "进程不存在或已退出"
  fi
  rm -f "$PID_FILE"
else
  # 兜底：按命令行匹配
  pkill -f "python3 server.py" 2>/dev/null && echo "已 pkill python3 server.py" || echo "未找到运行中的服务"
fi
