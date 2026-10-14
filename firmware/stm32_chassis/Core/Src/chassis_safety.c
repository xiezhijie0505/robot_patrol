#include "chassis_safety.h"
#include "chassis_motor.h"
#include "chassis_ultra.h"
#include "chassis_smoke.h"
#include "usart.h"

void ChassisSafety_Init(void)
{
  ChassisUltra_Init();
  ChassisSmoke_Init();
}

void ChassisSafety_Poll(void)
{
  ChassisUltra_Poll();
  ChassisSmoke_Poll();

  if (ChassisUltra_NeedStop() || ChassisSmoke_IsAlarm()) {
    ChassisMotor_Stop();
  }

  if (ChassisSmoke_TakeEvent()) {
    const char msg[] = "EVT SMOKE\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, sizeof(msg) - 1U, 50);
  }

  /* 可选：周期性上报距离，调试用（默认注释，防刷屏）
  {
    static uint32_t last;
    uint32_t now = HAL_GetTick();
    if ((now - last) > 500U) {
      char buf[24];
      int n;
      last = now;
      n = snprintf(buf, sizeof(buf), "EVT ULTRA %u\n",
                   (unsigned)ChassisUltra_GetCm());
      if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 50);
      }
    }
  }
  */
}
