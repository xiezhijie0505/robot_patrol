#ifndef CHASSIS_SAFETY_H
#define CHASSIS_SAFETY_H

/**
 * 安全任务：轮询超声波 + 烟雾。
 * - 过近 / 烟雾 → ChassisMotor_Stop()
 * - 烟雾边沿 → UART 上报 "EVT SMOKE\n"（供 RK 入库）
 */

void ChassisSafety_Init(void);
void ChassisSafety_Poll(void);

#endif /* CHASSIS_SAFETY_H */
