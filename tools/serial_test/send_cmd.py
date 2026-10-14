#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""可选：python send_cmd.py COM5 \"FWD 40\""""

from __future__ import annotations

import sys
import time


def main() -> int:
    if len(sys.argv) < 3:
        print("用法: python send_cmd.py <COMx> <CMD...>")
        print('示例: python send_cmd.py COM5 "FWD 40"')
        return 1
    port = sys.argv[1]
    cmd = " ".join(sys.argv[2:]).strip()
    try:
        import serial
    except ImportError:
        print("需要: pip install pyserial")
        return 1
    ser = serial.Serial(port, 115200, timeout=0.3)
    try:
        ser.write((cmd + "\n").encode("ascii", errors="ignore"))
        time.sleep(0.05)
        resp = ser.read(256)
        print("sent:", cmd)
        if resp:
            print("recv:", resp.decode("utf-8", errors="replace"))
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
