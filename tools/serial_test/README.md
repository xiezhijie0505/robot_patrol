# serial_test · 电脑串口测底盘

向 STM32（USART1，115200 8N1）发 `STOP` / `FWD` / `TURN` / `HB`，验证电机与心跳停车。

## 接线

电脑 USB‑TTL ↔ STM32 USART1（常见 PA9=TX、PA10=RX，共地）。

## 方式 A：串口助手（不写代码）

1. 打开 SSCOM / 串口调试助手，选对 COM 口，115200  
2. 勾选「发送新行」或手动在末尾加 `\n`  
3. 依次发送：

```text
STOP
FWD 40
HB
TURN 30
STOP
PATROL START
PATROL STOP
```

说明：固件调试版心跳超时约 **3 s**（见 `chassis_cmd.c` 的 `CMD_TIMEOUT_MS`）。只发一次 `FWD` 后若不继续发 `HB`/`FWD`，几秒后会自动停。

## 方式 B：PowerShell（Windows，无需 Python）

```powershell
cd D:\time\robot_patrol\tools\serial_test
# 改成你的 COM 口，例如 COM5
.\send_cmd.ps1 -Port COM5 -Cmd "FWD 40"
.\send_cmd.ps1 -Port COM5 -Cmd "STOP"
.\demo_move.ps1 -Port COM5
```

## 方式 C：Python（可选）

```bash
pip install pyserial
python send_cmd.py COM5 "FWD 40"
python demo_move.py COM5
```
