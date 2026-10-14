#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
报警记录 HTTP 服务（板端运行）。

电脑浏览器：http://<板子IP>:8080
检测程序入库：POST /api/alarms
"""

from __future__ import annotations

import os
import uuid
from datetime import datetime
from pathlib import Path

from flask import (
    Flask,
    abort,
    jsonify,
    render_template,
    request,
    send_from_directory,
)

import db

ROOT = Path(__file__).resolve().parent
HOST = os.environ.get("ALARMS_HOST", "0.0.0.0")
PORT = int(os.environ.get("ALARMS_PORT", "8080"))

app = Flask(
    __name__,
    template_folder=str(ROOT / "templates"),
    static_folder=str(ROOT / "static"),
)


def _save_upload(file_storage) -> str:
    """保存上传图片到 data/images/，返回相对路径（仅文件名）。"""
    if not file_storage or not file_storage.filename:
        raise ValueError("未选择图片文件")
    original = Path(file_storage.filename).name
    ext = Path(original).suffix.lower()
    if ext not in (".jpg", ".jpeg", ".png", ".webp", ".bmp"):
        raise ValueError("仅支持 jpg/png/webp/bmp")
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    name = f"{stamp}_{uuid.uuid4().hex[:8]}{ext}"
    dest = db.IMAGES_DIR / name
    file_storage.save(str(dest))
    return name


@app.route("/")
def page_index():
    alarm_type = request.args.get("type") or None
    items = db.list_alarms(alarm_type=alarm_type, limit=100)
    return render_template(
        "index.html",
        alarms=items,
        current_type=alarm_type or "",
        types=db.ALLOWED_TYPES,
    )


@app.route("/alarms/<int:alarm_id>")
def page_detail(alarm_id: int):
    item = db.get_alarm(alarm_id)
    if not item:
        abort(404)
    return render_template("detail.html", alarm=item)


@app.route("/media/<path:filename>")
def media(filename: str):
    # 禁止路径穿越
    safe = Path(filename).name
    path = db.IMAGES_DIR / safe
    if not path.is_file():
        abort(404)
    return send_from_directory(str(db.IMAGES_DIR), safe)


@app.route("/api/alarms", methods=["GET"])
def api_list():
    alarm_type = request.args.get("type") or None
    limit = request.args.get("limit", 50)
    offset = request.args.get("offset", 0)
    try:
        items = db.list_alarms(
            alarm_type=alarm_type, limit=int(limit), offset=int(offset)
        )
    except ValueError:
        return jsonify({"ok": False, "error": "limit/offset 无效"}), 400
    return jsonify({"ok": True, "count": len(items), "items": items})


@app.route("/api/alarms/<int:alarm_id>", methods=["GET"])
def api_get(alarm_id: int):
    item = db.get_alarm(alarm_id)
    if not item:
        return jsonify({"ok": False, "error": "not found"}), 404
    return jsonify({"ok": True, "item": item})


@app.route("/api/alarms", methods=["POST"])
def api_create():
    """
    两种写法：
    1) multipart/form-data: type, score?, note?, image?(文件)
    2) application/json: {type, score?, note?, image_rel?}
       image_rel 仅用于图片已在 data/images/ 下的情况
    """
    try:
        if request.content_type and "multipart/form-data" in request.content_type:
            alarm_type = request.form.get("type", "")
            score_raw = request.form.get("score")
            note = request.form.get("note")
            score = float(score_raw) if score_raw not in (None, "") else None
            image_rel = None
            if "image" in request.files and request.files["image"].filename:
                image_rel = _save_upload(request.files["image"])
        else:
            data = request.get_json(silent=True) or {}
            alarm_type = data.get("type", "")
            score = data.get("score")
            note = data.get("note")
            image_rel = data.get("image_rel")
            if image_rel:
                # 只允许文件名，且文件必须已存在
                image_rel = Path(str(image_rel)).name
                if not (db.IMAGES_DIR / image_rel).is_file():
                    return jsonify(
                        {"ok": False, "error": f"图片不存在: {image_rel}"}
                    ), 400

        alarm_id = db.insert_alarm(
            alarm_type=alarm_type,
            score=score,
            image_rel=image_rel,
            note=note,
        )
    except ValueError as e:
        return jsonify({"ok": False, "error": str(e)}), 400
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)}), 500

    item = db.get_alarm(alarm_id)
    return jsonify({"ok": True, "item": item}), 201


@app.route("/api/health")
def api_health():
    return jsonify({"ok": True, "service": "alarms", "db": str(db.DB_PATH)})


def main() -> None:
    db.init_db()
    print(f"alarms DB  : {db.DB_PATH}")
    print(f"images dir : {db.IMAGES_DIR}")
    print(f"listen     : http://{HOST}:{PORT}/")
    # 板端调试用；生产可换 waitress/gunicorn
    app.run(host=HOST, port=PORT, debug=False, threaded=True)


if __name__ == "__main__":
    main()
