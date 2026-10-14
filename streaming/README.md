# streaming · RTSP 推流

## 目标

```text
摄像头 → GStreamer(mpph264enc) → UDP:5004 → MediaMTX → RTSP → 电脑 VLC
```

VLC 打开：`rtsp://<板子IP>:8554/cam`

## 本仓库（Windows）有什么

| 路径 | 说明 |
|------|------|
| `configs/mediamtx.yml` | 路径 `cam`，收本机 UDP 5004 |
| `scripts/start_rtsp.sh` | 板端启动 MediaMTX + GStreamer |
| `scripts/stop_rtsp.sh` | 停止推流 |
| `bin/README.md` | 说明如何放置 arm64 的 `mediamtx`（二进制默认不进 Git） |

## 同步到板子

```bash
# 示例：配置与脚本
scp -r configs scripts root@<板子IP>:/root/robot_patrol/streaming/

# MediaMTX 二进制（从 releases 下载 arm64 后）
scp mediamtx root@<板子IP>:/root/robot_patrol/streaming/bin/
```

板端：

```sh
cd /root/robot_patrol/streaming
chmod +x bin/mediamtx scripts/*.sh
sh scripts/start_rtsp.sh
```

## 说明

- **运行必须在 RK3568 板上**（硬编码与摄像头在板端）。
- Windows 保留配置/脚本，便于改代码和上传 GitHub，避免空目录。
