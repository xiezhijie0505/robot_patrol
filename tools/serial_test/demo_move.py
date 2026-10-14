#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""可选：python demo_move.py COM5"""

from __future__ import annotations

import sys
import time


def main() -> int:
    if len(sys.argv) < 2:
        print("用法: python demo_move.py <COMx> [speed]")
        return 1
    port = sys.argv[1]
    speed = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    try:
        import serial
    except ImportError:
        print("需要: pip install pyserial")
        return 1

    def send(ser, cmd: str) -> None:
        ser.write((cmd + "\n").encode("ascii"))
        print("sent:", cmd)
        time.sleep(0.05)

    ser = serial.Serial(port, 115200, timeout=0.2)
    try:
        send(ser, "STOP")
        time.sleep(0.3)
        send(ser, f"FWD {speed}")
        for _ in range(8):
            time.sleep(0.4)
            send(ser, "HB")
        send(ser, "TURN 40")
        for _ in range(4):
            time.sleep(0.4)
            send(ser, "HB")
        send(ser, "STOP")
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
