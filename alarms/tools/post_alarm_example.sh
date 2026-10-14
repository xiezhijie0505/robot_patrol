#!/bin/sh
# 在板子或电脑上调用：向报警服务写入一条记录（可带图）
# 用法：
#   sh tools/post_alarm_example.sh
#   HOST=192.168.1.20 sh tools/post_alarm_example.sh
#   IMAGE=/path/to.jpg TYPE=stranger sh tools/post_alarm_example.sh

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-8080}"
TYPE="${TYPE:-smoke}"
NOTE="${NOTE:-manual test}"
BASE="http://${HOST}:${PORT}"

if [ -n "$IMAGE" ] && [ -f "$IMAGE" ]; then
  curl -sS -X POST "$BASE/api/alarms" \
    -F "type=$TYPE" \
    -F "score=0.9" \
    -F "note=$NOTE" \
    -F "image=@${IMAGE}"
else
  curl -sS -X POST "$BASE/api/alarms" \
    -H "Content-Type: application/json" \
    -d "{\"type\":\"$TYPE\",\"score\":0.9,\"note\":\"$NOTE\"}"
fi

echo
