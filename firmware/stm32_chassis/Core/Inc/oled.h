#ifndef __OLED_H
#define __OLED_H

#include"stm32f1xx.h"

//pb6--scl pb7--sda 定义宏拉低/高
#define OLED_SCLK_Clr() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,GPIO_PIN_RESET)
#define OLED_SCLK_Set() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)
#define OLED_SDIN_Clr() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,GPIO_PIN_RESET)
#define OLED_SDIN_Set() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)

void OLED_PinInit(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_WaitAck(void);
void Send_Byte(unsigned char dat);
#endif
