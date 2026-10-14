#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SQLite 访问：初始化、插入、查询报警记录。"""

from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Any, Dict, List, Optional

ROOT = Path(__file__).resolve().parent
DATA_DIR = ROOT / "data"
DB_PATH = DATA_DIR / "alarms.db"
IMAGES_DIR = DATA_DIR / "images"
SCHEMA_PATH = ROOT / "schema.sql"

ALLOWED_TYPES = ("smoke", "stranger", "other")


def ensure_dirs() -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    IMAGES_DIR.mkdir(parents=True, exist_ok=True)


def connect() -> sqlite3.Connection:
    ensure_dirs()
    conn = sqlite3.connect(str(DB_PATH), check_same_thread=False)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    return conn


def init_db() -> None:
    ensure_dirs()
    schema = SCHEMA_PATH.read_text(encoding="utf-8")
    with connect() as conn:
        conn.executescript(schema)
        conn.commit()


def _row_to_dict(row: sqlite3.Row) -> Dict[str, Any]:
    d = dict(row)
    # 给前端/API 一个可直接打开的图片 URL（有图时）
    if d.get("image_rel"):
        d["image_url"] = "/media/" + d["image_rel"].lstrip("/")
    else:
        d["image_url"] = None
    return d


def list_alarms(
    alarm_type: Optional[str] = None,
    limit: int = 50,
    offset: int = 0,
) -> List[Dict[str, Any]]:
    limit = max(1, min(int(limit), 200))
    offset = max(0, int(offset))
    sql = "SELECT * FROM alarms"
    args: List[Any] = []
    if alarm_type:
        sql += " WHERE type = ?"
        args.append(alarm_type)
    sql += " ORDER BY id DESC LIMIT ? OFFSET ?"
    args.extend([limit, offset])
    with connect() as conn:
        rows = conn.execute(sql, args).fetchall()
    return [_row_to_dict(r) for r in rows]


def get_alarm(alarm_id: int) -> Optional[Dict[str, Any]]:
    with connect() as conn:
        row = conn.execute(
            "SELECT * FROM alarms WHERE id = ?", (int(alarm_id),)
        ).fetchone()
    return _row_to_dict(row) if row else None


def insert_alarm(
    alarm_type: str,
    score: Optional[float] = None,
    image_rel: Optional[str] = None,
    note: Optional[str] = None,
) -> int:
    alarm_type = (alarm_type or "").strip().lower()
    if alarm_type not in ALLOWED_TYPES:
        raise ValueError("type 必须是 smoke / stranger / other")
    with connect() as conn:
        cur = conn.execute(
            "INSERT INTO alarms (type, score, image_rel, note) VALUES (?, ?, ?, ?)",
            (alarm_type, score, image_rel, note),
        )
        conn.commit()
        return int(cur.lastrowid)
