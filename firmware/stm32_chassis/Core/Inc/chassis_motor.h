#ifndef CHASSIS_MOTOR_H
#define CHASSIS_MOTOR_H

#include <stdint.h>

/**
 * TB6612 引脚约定（代码内初始化，勿与 USART1/SWD 冲突）：
 *   AIN1=PB12  AIN2=PB13  BIN1=PB14  BIN2=PB15  STBY=PB8
 *   PWMA=PA6(TIM3_CH1)  PWMB=PA7(TIM3_CH2)
 * 速度约定：-100~100，正转/反转，0 停车。
 */

void ChassisMotor_Init(void);
void ChassisMotor_Stop(void);
void ChassisMotor_SetSpeed(int16_t left, int16_t right);
void ChassisMotor_Poll(void);

#endif /* CHASSIS_MOTOR_H */
