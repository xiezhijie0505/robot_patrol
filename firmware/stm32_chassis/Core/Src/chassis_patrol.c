#include "chassis_patrol.h"
#include "chassis_motor.h"
#include "chassis_ultra.h"
#include "chassis_smoke.h"
#include "usart.h"

/* ========== 按 2.0m × 1.2m 估算，实测后只改这里 ========== */
#define PATROL_SPEED       45          /* FWD 速度 0~100 */
#define PATROL_LONG_MS     5000U       /* 长边 ≈ 2.0 m */
#define PATROL_SHORT_MS    3000U       /* 短边 ≈ 1.2 m */
#define PATROL_TURN_MS     1200U       /* 约 90° 左转时间 */
#define PATROL_TURN_BASE   45          /* 转弯时基础速度 */
#define PATROL_TURN_D      50          /* 差速：正=左转 */
#define PATROL_GAP_MS      200U        /* 段与段之间停顿，减惯性 */

typedef enum {
  PATROL_IDLE = 0,
  PATROL_FWD_LONG,
  PATROL_TURN1,
  PATROL_FWD_SHORT,
  PATROL_TURN2,
  PATROL_PAUSE_ULTRA,   /* 超声波过近：停住，清障后从当前段重跑 */
  PATROL_ABORT_SMOKE
} PatrolState;

static PatrolState s_state;
static PatrolState s_resume_state; /* 超声暂停前的状态 */
static uint32_t    s_state_t0;
static uint8_t     s_active;
static uint8_t     s_leg;          /* 0=长边阶段，1=短边阶段，循环 */

static void UartSay(const char *s)
{
  uint16_t n = 0;
  while (s[n] != '\0') {
    n++;
  }
  HAL_UART_Transmit(&huart1, (uint8_t *)s, n, 50);
}

static void EnterState(PatrolState st)
{
  s_state = st;
  s_state_t0 = HAL_GetTick();
}

static void ApplyFwd(void)
{
  ChassisMotor_SetSpeed((int16_t)PATROL_SPEED, (int16_t)PATROL_SPEED);
}

static void ApplyTurnLeft(void)
{
  /* 与 cmd 一致：TURN +d → left = base - d, right = base + d */
  int16_t left  = (int16_t)(PATROL_TURN_BASE - PATROL_TURN_D);
  int16_t right = (int16_t)(PATROL_TURN_BASE + PATROL_TURN_D);
  if (left < -100) {
    left = -100;
  }
  if (right > 100) {
    right = 100;
  }
  ChassisMotor_SetSpeed(left, right);
}

static uint32_t StateElapsed(void)
{
  return HAL_GetTick() - s_state_t0;
}

void ChassisPatrol_Init(void)
{
  s_state = PATROL_IDLE;
  s_resume_state = PATROL_IDLE;
  s_state_t0 = HAL_GetTick();
  s_active = 0U;
  s_leg = 0U;
}

void ChassisPatrol_Start(void)
{
  if (ChassisSmoke_IsAlarm()) {
    UartSay("EVT PATROL DENY SMOKE\n");
    return;
  }
  s_active = 1U;
  s_leg = 0U;
  EnterState(PATROL_FWD_LONG);
  ApplyFwd();
  UartSay("EVT PATROL START\n");
}

void ChassisPatrol_Stop(void)
{
  s_active = 0U;
  EnterState(PATROL_IDLE);
  ChassisMotor_Stop();
  UartSay("EVT PATROL STOP\n");
}

uint8_t ChassisPatrol_IsActive(void)
{
  return s_active;
}

void ChassisPatrol_Poll(void)
{
  if (!s_active) {
    return;
  }

  /* 烟雾：终止巡逻 */
  if (ChassisSmoke_IsAlarm()) {
    ChassisMotor_Stop();
    s_active = 0U;
    EnterState(PATROL_ABORT_SMOKE);
    UartSay("EVT PATROL ABORT SMOKE\n");
    return;
  }

  /* 超声波过近：暂停，障碍消失后重跑当前段 */
  if (ChassisUltra_NeedStop()) {
    if (s_state != PATROL_PAUSE_ULTRA) {
      s_resume_state = s_state;
      ChassisMotor_Stop();
      EnterState(PATROL_PAUSE_ULTRA);
      UartSay("EVT PATROL PAUSE ULTRA\n");
    }
    return;
  }
  if (s_state == PATROL_PAUSE_ULTRA) {
    /* 恢复：从暂停前的段重新计时 */
    EnterState(s_resume_state);
    if (s_resume_state == PATROL_FWD_LONG || s_resume_state == PATROL_FWD_SHORT) {
      ApplyFwd();
    } else if (s_resume_state == PATROL_TURN1 || s_resume_state == PATROL_TURN2) {
      ApplyTurnLeft();
    }
    UartSay("EVT PATROL RESUME\n");
    return;
  }

  switch (s_state) {
    case PATROL_FWD_LONG:
      ApplyFwd();
      if (StateElapsed() >= PATROL_LONG_MS) {
        ChassisMotor_Stop();
        EnterState(PATROL_TURN1);
      }
      break;

    case PATROL_TURN1:
      if (StateElapsed() < PATROL_GAP_MS) {
        ChassisMotor_Stop();
        break;
      }
      ApplyTurnLeft();
      if (StateElapsed() >= (PATROL_GAP_MS + PATROL_TURN_MS)) {
        ChassisMotor_Stop();
        EnterState(PATROL_FWD_SHORT);
      }
      break;

    case PATROL_FWD_SHORT:
      if (StateElapsed() < PATROL_GAP_MS) {
        ChassisMotor_Stop();
        break;
      }
      ApplyFwd();
      if (StateElapsed() >= (PATROL_GAP_MS + PATROL_SHORT_MS)) {
        ChassisMotor_Stop();
        EnterState(PATROL_TURN2);
      }
      break;

    case PATROL_TURN2:
      if (StateElapsed() < PATROL_GAP_MS) {
        ChassisMotor_Stop();
        break;
      }
      ApplyTurnLeft();
      if (StateElapsed() >= (PATROL_GAP_MS + PATROL_TURN_MS)) {
        ChassisMotor_Stop();
        /* 再走长边，继续绕圈 */
        s_leg ^= 1U;
        EnterState(PATROL_FWD_LONG);
        UartSay("EVT PATROL LAP\n");
      }
      break;

    default:
      break;
  }
}
