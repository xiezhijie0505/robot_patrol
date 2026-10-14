# stm32_chassis · 底盘运动固件

巡逻机器人 V1：STM32F103 + FreeRTOS + TB6612，串口收运动指令；超声波防撞 + 烟雾蜂鸣硬停。

## 打开工程

```text
CubeMX：stm32_chassis.ioc
Keil  ：MDK-ARM/stm32_chassis.uvprojx
```

新增 `.c` 后若 Keil 未显示，确认工程里已加入：

- `chassis_ultra.c` / `chassis_smoke.c` / `chassis_safety.c`

## 模块

| 文件 | 用途 |
|------|------|
| `chassis_cmd.c` | 串口解析 STOP / FWD / TURN / HB + 心跳超时停车 |
| `chassis_motor.c` | TB6612 左右轮 PWM + 方向 |
| `chassis_ultra.c` | HC-SR04 测距，过近置位急停 |
| `chassis_smoke.c` | 烟雾 DO + 蜂鸣器 |
| `chassis_safety.c` | 综合急停；烟雾边沿上报 `EVT SMOKE` |
| `chassis_patrol.c` | 矩形自动巡逻（约 2.0×1.2 m，`PATROL START/STOP`） |
| `freertos.c` | cmd / motor / safety / patrol 任务 |

## 默认引脚

见仓库 `docs/硬件接线.md`。电机：PB12–15 / PB8 / PA6–7；超声 PA0/PA1；烟雾 PA2；蜂鸣 PA3。

## 联调

1. Keil 编译下载  
2. USB‑TTL 接 USART1，电脑用 `tools/serial_test/`（PowerShell 或串口助手）发 `FWD 40`  
3. 手放超声前应自动停；烟雾触发应蜂鸣并停，串口可见 `EVT SMOKE`  
4. 发 `PATROL START` 开始绕 2.0×1.2 m 矩形；`PATROL STOP` 结束。时间不准就改 `chassis_patrol.c` 里 `PATROL_LONG_MS` / `PATROL_SHORT_MS` / `PATROL_TURN_MS`

## 与仓库其它目录

| 路径 | 关系 |
|------|------|
| `../../protocol/chassis_uart.md` | 上下行协议 |
| `../../tools/serial_test/` | 电脑串口测运动 |
| `../../alarms/` | RK 报警 HTTP；`tools/uart_alarm_bridge.py` 收 `EVT SMOKE` 入库 |
| `../../streaming/` | 推流，与本固件独立 |
