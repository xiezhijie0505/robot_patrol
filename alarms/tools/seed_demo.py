#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""写入几条示例报警，方便电脑浏览器先看到效果（无需真实检测）。"""

from __future__ import annotations

import base64
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import db  # noqa: E402

# 1x1 红色 PNG
_PNG_1X1 = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="
)


def main() -> None:
    db.init_db()

    img1 = "demo_stranger_1.png"
    img2 = "demo_stranger_2.png"
    (db.IMAGES_DIR / img1).write_bytes(_PNG_1X1)
    (db.IMAGES_DIR / img2).write_bytes(_PNG_1X1)

    rows = [
        ("smoke", 0.95, None, "演示：烟雾报警，无图"),
        ("stranger", 0.88, img1, "演示：陌生人抓拍 1"),
        ("stranger", 0.91, img2, "演示：陌生人抓拍 2"),
        ("other", None, None, "演示：其它事件"),
    ]
    ids = []
    for t, score, image_rel, note in rows:
        ids.append(db.insert_alarm(t, score=score, image_rel=image_rel, note=note))

    print("已写入示例报警 id:", ids)
    print("打开: http://<板子IP>:8080/")


if __name__ == "__main__":
    main()
