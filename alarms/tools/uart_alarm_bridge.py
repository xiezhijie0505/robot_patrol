#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
板端：监听 STM32 串口上行事件，转发到本机报警 HTTP 入库。

依赖：pip3 install pyserial
用法（RK 上，先启动 alarms 服务）：
  python3 tools/uart_alarm_bridge.py /dev/ttyS3
  # 或 USB 转串口： /dev/ttyUSB0

识别行：
  EVT SMOKE  → POST type=smoke
"""

from __future__ import annotations

import argparse
import json
import sys
import urllib.error
import urllib.request


def post_alarm(base: str, alarm_type: str, note: str) -> None:
    url = base.rstrip("/") + "/api/alarms"
    body = json.dumps(
        {"type": alarm_type, "score": None, "note": note},
        ensure_ascii=False,
    ).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=3) as resp:
        print("posted:", resp.read().decode("utf-8", errors="replace"))


def main() -> int:
    ap = argparse.ArgumentParser(description="STM32 EVT → alarms HTTP")
    ap.add_argument("port", help="串口设备，如 /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--http", default="http://127.0.0.1:8080")
    args = ap.parse_args()

    try:
        import serial
    except ImportError:
        print("需要: pip3 install pyserial", file=sys.stderr)
        return 1

    ser = serial.Serial(args.port, args.baud, timeout=1)
    print(f"listening {args.port} @ {args.baud}, post → {args.http}")
    buf = ""
    try:
        while True:
            chunk = ser.read(128)
            if not chunk:
                continue
            buf += chunk.decode("utf-8", errors="ignore")
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip().strip("\r")
                if not line:
                    continue
                print("uart:", line)
                if line == "EVT SMOKE":
                    try:
                        post_alarm(args.http, "smoke", "STM32 smoke sensor")
                    except (urllib.error.URLError, TimeoutError) as e:
                        print("POST failed:", e, file=sys.stderr)
    except KeyboardInterrupt:
        print("bye")
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
