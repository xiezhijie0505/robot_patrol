#!/bin/sh
# 在 RK3568 板端执行
# 用法：
#   cd /root/robot_patrol/alarms
#   sh scripts/start_alarms.sh
#
# 电脑浏览器：http://<板子IP>:8080/

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

PORT="${ALARMS_PORT:-8080}"
LOG="${ALARMS_LOG:-/tmp/alarms_http.log}"
PID_FILE="${ALARMS_PID:-/tmp/alarms_http.pid}"

if ! command -v python3 >/dev/null 2>&1; then
  echo "未找到 python3，请先安装 Python3"
  exit 1
fi

# 检查 Flask
if ! python3 -c "import flask" 2>/dev/null; then
  echo "未安装 Flask。可执行其一："
  echo "  pip3 install -r requirements.txt"
  echo "  或: apt-get install -y python3-flask"
  exit 1
fi

# 已在跑则先停
if [ -f "$PID_FILE" ]; then
  oldpid="$(cat "$PID_FILE" 2>/dev/null)"
  if [ -n "$oldpid" ] && kill -0 "$oldpid" 2>/dev/null; then
    echo "停止旧进程 pid=$oldpid"
    kill "$oldpid" 2>/dev/null
    sleep 1
  fi
  rm -f "$PID_FILE"
fi

export ALARMS_HOST="${ALARMS_HOST:-0.0.0.0}"
export ALARMS_PORT="$PORT"

echo "启动报警 HTTP 服务 ..."
nohup python3 server.py >"$LOG" 2>&1 &
echo $! >"$PID_FILE"
sleep 1

if kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "已启动 pid=$(cat "$PID_FILE")  log=$LOG"
  echo "浏览器打开: http://<板子IP>:${PORT}/"
  tail -n 8 "$LOG"
else
  echo "启动失败，查看日志: $LOG"
  tail -n 30 "$LOG"
  exit 1
fi
