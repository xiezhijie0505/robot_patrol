#include "chassis_cmd.h"
#include "usart.h"
#include "chassis_motor.h"
#include "chassis_patrol.h"
#include <string.h>
#include <stdlib.h>

#define CMD_LINE_MAX    32
/* 手控调试用 3s；自动巡逻可改为 300 */
#define CMD_TIMEOUT_MS  3000U
#define CMD_TURN_BASE   50

static char     s_line[CMD_LINE_MAX];  /* 当前正在拼的一行指令 */
static uint8_t  s_line_len;            /* 已收到的字符个数 */
static uint32_t s_last_cmd_ms;         /* 上次合法指令的时刻 */

/* 把速度限制在 -100~100 */
static int16_t ClampSpeed(int v)
{
  if (v > 100) {
    return 100;
  }
  if (v < -100) {
    return -100;
  }
  return (int16_t)v;
}

/* 刷新心跳：记下“刚刚还活着”的时间 */
static void TouchHeartbeat(void)
{
  s_last_cmd_ms = HAL_GetTick();
}

/* 解析已经收完整的一行（不含 '\n'） */
static void HandleLine(void)
{
  /* 自动巡逻启停 */
  if (strcmp(s_line, "PATROL START") == 0) {
    ChassisPatrol_Start();
    TouchHeartbeat();
    return;
  }
  if (strcmp(s_line, "PATROL STOP") == 0) {
    ChassisPatrol_Stop();
    TouchHeartbeat();
    return;
  }

  /* STOP：停车并结束巡逻 */
  if (strcmp(s_line, "STOP") == 0) {
    if (ChassisPatrol_IsActive()) {
      ChassisPatrol_Stop();
    } else {
      ChassisMotor_Stop();
    }
    TouchHeartbeat();
    return;
  }

  /* HB：只续命，不改速度 */
  if (strcmp(s_line, "HB") == 0) {
    TouchHeartbeat();
    return;
  }

  /* 手控运动时若在巡逻，先退出自动模式 */
  if (strncmp(s_line, "FWD ", 4) == 0 || strncmp(s_line, "TURN ", 5) == 0) {
    if (ChassisPatrol_IsActive()) {
      ChassisPatrol_Stop();
    }
  }

  /* FWD <v>：两轮同速直行 */
  if (strncmp(s_line, "FWD ", 4) == 0) {
    int v = atoi(s_line + 4); /* 跳过 "FWD " 解析数字 */
    if (v < 0) {
      v = 0;
    }
    if (v > 100) {
      v = 100;
    }
    ChassisMotor_SetSpeed((int16_t)v, (int16_t)v);
    TouchHeartbeat();
    return;
  }

  /* TURN <d>：差速转弯，正左负右；基础速度 CMD_TURN_BASE */
  if (strncmp(s_line, "TURN ", 5) == 0) {
    int d = atoi(s_line + 5);
    if (d > 100) {
      d = 100;
    }
    if (d < -100) {
      d = -100;
    }
    {
      int16_t left  = ClampSpeed(CMD_TURN_BASE - d);
      int16_t right = ClampSpeed(CMD_TURN_BASE + d);
      ChassisMotor_SetSpeed(left, right);
    }
    TouchHeartbeat();
    return;
  }
  /* 无法识别的行：忽略，不刷新心跳 */
}

/**
 * @brief  串口指令模块上电初始化
 */
void ChassisCmd_Init(void)
{
  s_line_len = 0;
  memset(s_line, 0, sizeof(s_line));
  s_last_cmd_ms = HAL_GetTick();

  {
    const char msg[] = "cmd ready\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, sizeof(msg) - 1U, 100);
  }
}

/**
 * @brief  串口指令周期处理
 */
void ChassisCmd_Poll(void)
{
  uint8_t ch;

  /* 把当前 FIFO 里已到的字节尽量读完（timeout=0 表示不阻塞） */
  while (HAL_UART_Receive(&huart1, &ch, 1U, 0U) == HAL_OK) {
    if (ch == '\r') {
      continue; /* Windows 常发 \r\n，丢掉 \r */
    }
    if (ch == '\n') {
      s_line[s_line_len] = '\0'; /* 补字符串结束符，便于 strcmp */
      if (s_line_len > 0U) {
        HandleLine();           /* 完整一行 → 解析并执行 */
      }
      s_line_len = 0;           /* 准备收下一行 */
      continue;
    }
    if (s_line_len < (CMD_LINE_MAX - 1U)) {
      s_line[s_line_len++] = (char)ch; /* 普通字符拼进缓冲 */
    } else {
      s_line_len = 0; /* 行太长，丢弃，避免溢出 */
    }
  }

  /* 自动巡逻中由巡逻状态机控速，不因手控心跳超时停车 */
  if (ChassisPatrol_IsActive()) {
    return;
  }

  /* 太久没收到合法指令 → 安全停车 */
  if ((HAL_GetTick() - s_last_cmd_ms) > CMD_TIMEOUT_MS) {
    ChassisMotor_Stop();
  }
}