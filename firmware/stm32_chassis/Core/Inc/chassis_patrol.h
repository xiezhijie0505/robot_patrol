#ifndef CHASSIS_PATROL_H
#define CHASSIS_PATROL_H

#include <stdint.h>

/**
 * 矩形自动巡逻（开环：速度 × 时间）。
 * 场地约：长 2.0 m × 宽 1.2 m，逆时针绕圈。
 *
 * 串口：
 *   PATROL START
 *   PATROL STOP
 *
 * 下列时间按「FWD≈45 时约 0.4 m/s」估算，场地上务必实测改宏：
 *   2.0m / 0.4 ≈ 5.0s；1.2m / 0.4 ≈ 3.0s；原地差速约 90° 另调 TURN 时间。
 */

void ChassisPatrol_Init(void);
void ChassisPatrol_Poll(void);

void ChassisPatrol_Start(void);
void ChassisPatrol_Stop(void);

/** 1 = 正在自动巡逻（此时忽略手控心跳超时停车） */
uint8_t ChassisPatrol_IsActive(void);

#endif /* CHASSIS_PATROL_H */
