#ifndef CHASSIS_SMOKE_H
#define CHASSIS_SMOKE_H

#include <stdint.h>

/**
 * 烟雾模块数字输出 + 蜂鸣器（引脚可改）：
 *   SMOKE_DO = PA2（低电平=报警，若模块相反改 CHASSIS_SMOKE_ACTIVE_LOW）
 *   BUZZER   = PA3（高电平响）
 *
 * 报警时：蜂鸣、置位急停、串口上报 EVT SMOKE（由 safety/上报逻辑处理）。
 */

#ifndef CHASSIS_SMOKE_ACTIVE_LOW
#define CHASSIS_SMOKE_ACTIVE_LOW  1
#endif

void ChassisSmoke_Init(void);
void ChassisSmoke_Poll(void);

/** 1 = 当前检测到烟雾 */
uint8_t ChassisSmoke_IsAlarm(void);

/** 1 = 需要上报一次（边沿），读后清零 */
uint8_t ChassisSmoke_TakeEvent(void);

#endif /* CHASSIS_SMOKE_H */
