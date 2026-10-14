#ifndef CHASSIS_CMD_H
#define CHASSIS_CMD_H

#include <stdint.h>

/**
 * @brief  串口指令模块上电初始化
 * @note   在 chassisCmd 任务启动时调用一次。
 *         应完成：行缓冲清零、心跳计时初值等软件状态；
 *         不要在这里初始化 USART（main 里已 MX_USART1_UART_Init）。
 */
void ChassisCmd_Init(void);

/**
 * @brief  串口指令周期处理
 * @note   由 FreeRTOS 任务周期调用（如每 10ms）。
 *         应完成：非阻塞读 UART → 拼行 → 解析 STOP/FWD/TURN/HB/PATROL
 *         → 调用 ChassisMotor_* / ChassisPatrol_* → 检查心跳超时并停车。
 */
void ChassisCmd_Poll(void);

#endif /* CHASSIS_CMD_H */
