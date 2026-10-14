#ifndef CHASSIS_ULTRA_H
#define CHASSIS_ULTRA_H

#include <stdint.h>

/**
 * HC-SR04 超声波（引脚可按接线修改）：
 *   TRIG = PA0   ECHO = PA1
 * 距离单位 cm；测距失败返回 0xFFFF。
 * 小于 CHASSIS_ULTRA_STOP_CM 时置位急停请求（由 safety 停车）。
 */

#ifndef CHASSIS_ULTRA_STOP_CM
#define CHASSIS_ULTRA_STOP_CM  25U
#endif

void ChassisUltra_Init(void);
void ChassisUltra_Poll(void);

/** 最近一次有效距离(cm)；无效为 0xFFFF */
uint16_t ChassisUltra_GetCm(void);

/** 1 = 过近需要硬刹停 */
uint8_t ChassisUltra_NeedStop(void);

#endif /* CHASSIS_ULTRA_H */
