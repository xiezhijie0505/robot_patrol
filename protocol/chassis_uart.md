# 底盘串口协议 · V1

双向，115200 8N1，一行一条，`\n` 结尾。

## 下行：RK / PC → STM32

| 指令 | 格式 | 说明 |
|------|------|------|
| 停车 | `STOP` | 两轮停止；若在巡逻则结束巡逻 |
| 前进 | `FWD <v>` | `<v>`：0~100，两轮同速（手控会退出巡逻） |
| 转向 | `TURN <d>` | `<d>`：-100~100，正左负右 |
| 心跳 | `HB` | 刷新存活，不改速度 |
| 开始巡逻 | `PATROL START` | 矩形绕圈（约 2.0m×1.2m，时间开环） |
| 停止巡逻 | `PATROL STOP` | 结束自动巡逻并停车 |

示例：

```text
PATROL START
PATROL STOP
STOP
FWD 60
TURN -30
HB
```

## 心跳

任意合法指令都刷新计时。  
固件当前调试默认 **3000 ms** 无新指令 → 自动停车（`chassis_cmd.c` 的 `CMD_TIMEOUT_MS`）；正式巡逻可改回 **300 ms**。

## 上行：STM32 → RK / PC

| 事件 | 格式 | 说明 |
|------|------|------|
| 烟雾报警 | `EVT SMOKE` | 检测到烟雾边沿时发一次；STM32 侧已蜂鸣并硬停 |
| 巡逻 | `EVT PATROL START/STOP/LAP/...` | 自动巡逻状态提示 |

RK 上可用 `alarms/tools/uart_alarm_bridge.py` 监听后 `POST` 到本机报警 HTTP 服务入库。

## 矩形巡逻（约 2.0 m × 1.2 m）

开环：长边时间 → 左转 ≈90° → 短边时间 → 左转 → 循环。  
默认宏在 `chassis_patrol.c`：`PATROL_LONG_MS=5000`（2m）、`PATROL_SHORT_MS=3000`（1.2m），按实车速度微调。  
超声波过近会暂停，障碍离开后重跑当前段；烟雾则中止巡逻。
