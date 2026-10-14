# robot_patrol · 安防巡逻机器人

Windows 工程目录（开发与保存）。**运行推流 / 插摄像头在 RK3568 开发板上。**

当前阶段：RTSP、报警查询、底盘巡逻/防撞/烟雾、YOLO 陌生人抓拍骨架均已落地，需板端编译与实车调参。

## 目录说明

| 路径 | 含义 |
|------|------|
| `streaming/` | 视频推流（MediaMTX / GStreamer） |
| `alarms/` | 报警记录：SQLite 存库 + HTTP 网页/API 查询 |
| `detect/` | YOLO(RKNN) 人检测 → 陌生人连拍 → POST 入库 |
| `firmware/stm32_chassis/` | STM32 底盘：运动 / 巡逻 / 超声 / 烟雾 |
| `protocol/` | RK↔STM32 指令与帧格式 |
| `tools/serial_test/` | 电脑串口测底盘运动（PowerShell / 串口助手） |
| `docs/` | 文档与笔记（含 `硬件接线.md`） |

## 推荐工作流

1. 在本仓库（Windows）改脚本与配置，拷到板子 `/root/robot_patrol/`  
2. 详细步骤见 **`docs/演示步骤.md`**  
3. 推流：`streaming/scripts/start_rtsp.sh` → VLC `rtsp://<板子IP>:8554/cam`  
4. **单终端一键**：`sh scripts/start_all.sh` → 浏览器 `http://<板子IP>:8080/`  
5. 底盘：Keil 先烧固件；可选 `CHASSIS_UART=/dev/ttyS3 sh scripts/start_all.sh` 自动发 `PATROL START`

## 与其它项目关系

- 独立于 `D:\time\Qt\SmartCockpitDMS`（座舱 DMS / Qt）；YOLO/RKNN 写法可参考 DMS，代码在本仓库 `detect/`  

