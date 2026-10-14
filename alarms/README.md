# alarms · 报警记录（SQLite + HTTP）

板端保存报警与图片，电脑用浏览器查询。

```text
检测/演示写入 → SQLite + data/images/
                ↓
         Flask HTTP :8080
                ↓
     电脑浏览器 http://板子IP:8080
```

**不含远程控车**（停车等以后再做）。

## 目录

| 路径 | 说明 |
|------|------|
| `server.py` | HTTP 服务入口 |
| `db.py` | SQLite 封装 |
| `schema.sql` | 表结构 |
| `templates/` | 列表 / 详情网页 |
| `data/alarms.db` | 运行后生成 |
| `data/images/` | 抓拍图片 |
| `tools/seed_demo.py` | 写入示例数据 |
| `scripts/start_alarms.sh` | 板端启动 |
| `scripts/stop_alarms.sh` | 板端停止 |

## 同步到板子

```powershell
scp -r D:\time\robot_patrol\alarms root@<板子IP>:/root/robot_patrol/
```

板端安装依赖（二选一）：

```sh
cd /root/robot_patrol/alarms
pip3 install -r requirements.txt
# 或
# apt-get install -y python3-flask
```

## 启动

```sh
cd /root/robot_patrol/alarms
chmod +x scripts/*.sh tools/*.sh
sh scripts/start_alarms.sh
python3 tools/seed_demo.py          # 可选：示例数据
```

电脑浏览器打开：`http://<板子IP>:8080/`

## API

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/` | 网页列表 |
| GET | `/alarms/<id>` | 网页详情 |
| GET | `/api/alarms` | JSON 列表，`?type=smoke&limit=50` |
| GET | `/api/alarms/<id>` | JSON 单条 |
| POST | `/api/alarms` | 入库（JSON 或 multipart 带图） |
| GET | `/media/<文件名>` | 取图片 |
| GET | `/api/health` | 健康检查 |

### 入库示例

JSON（无图，如烟雾）：

```bash
curl -X POST http://127.0.0.1:8080/api/alarms \
  -H "Content-Type: application/json" \
  -d '{"type":"smoke","score":0.95,"note":"走廊烟雾"}'
```

带图（陌生人抓拍）：

```bash
curl -X POST http://127.0.0.1:8080/api/alarms \
  -F type=stranger \
  -F score=0.88 \
  -F note=门口有人 \
  -F image=@/tmp/shot.jpg
```

`type` 仅允许：`smoke` | `stranger` | `other`。

## 烟雾串口桥（STM32 → 本服务）

STM32 报 `EVT SMOKE` 后，在板子上另开终端：

```sh
# 先保证本服务已启动；串口设备按实际改
pip3 install pyserial
python3 tools/uart_alarm_bridge.py /dev/ttyUSB0
```

桥接进程会 `POST` 到 `http://127.0.0.1:8080/api/alarms`。

## 后续对接

人脸/陌生人检测报警时，同样对本机 `POST /api/alarms`（可带图）；本服务负责存库、存图、给电脑查询。
