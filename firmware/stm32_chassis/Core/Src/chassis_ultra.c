#include "chassis_ultra.h"
#include "main.h"

/* ---- 接线：不对就改这里 ---- */
#define ULTRA_TRIG_GPIO   GPIOA
#define ULTRA_TRIG_PIN    GPIO_PIN_0
#define ULTRA_ECHO_GPIO   GPIOA
#define ULTRA_ECHO_PIN    GPIO_PIN_1

#define ULTRA_TIMEOUT_US  30000U  /* 约 5m 上限 */
#define ULTRA_PERIOD_MS   80U

static uint16_t s_cm = 0xFFFFU;
static uint8_t  s_need_stop;
static uint32_t s_last_ms;

static void DelayUs(uint32_t us)
{
  /* 72MHz 粗延时：约 72 周期/us，空循环足够 HC-SR04 触发脉宽 */
  uint32_t cycles = us * 72U / 4U;
  while (cycles--) {
    __NOP();
  }
}

static uint32_t MicrosNow(void)
{
  /* 用 DWT 周期计数（Cortex-M3）；Init 里打开 */
  return DWT->CYCCNT / 72U;
}

static void DwtInit(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint16_t MeasureOnce(void)
{
  uint32_t t0;
  uint32_t t1;
  uint32_t us;
  uint32_t guard;

  /* 10us 触发 */
  HAL_GPIO_WritePin(ULTRA_TRIG_GPIO, ULTRA_TRIG_PIN, GPIO_PIN_RESET);
  DelayUs(2);
  HAL_GPIO_WritePin(ULTRA_TRIG_GPIO, ULTRA_TRIG_PIN, GPIO_PIN_SET);
  DelayUs(10);
  HAL_GPIO_WritePin(ULTRA_TRIG_GPIO, ULTRA_TRIG_PIN, GPIO_PIN_RESET);

  /* 等 ECHO 变高 */
  guard = MicrosNow();
  while (HAL_GPIO_ReadPin(ULTRA_ECHO_GPIO, ULTRA_ECHO_PIN) == GPIO_PIN_RESET) {
    if ((MicrosNow() - guard) > ULTRA_TIMEOUT_US) {
      return 0xFFFFU;
    }
  }
  t0 = MicrosNow();

  /* 等 ECHO 变低 */
  while (HAL_GPIO_ReadPin(ULTRA_ECHO_GPIO, ULTRA_ECHO_PIN) == GPIO_PIN_SET) {
    if ((MicrosNow() - t0) > ULTRA_TIMEOUT_US) {
      return 0xFFFFU;
    }
  }
  t1 = MicrosNow();

  us = t1 - t0;
  /* cm = us / 58 */
  return (uint16_t)(us / 58U);
}

void ChassisUltra_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  DwtInit();

  gpio.Pin   = ULTRA_TRIG_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ULTRA_TRIG_GPIO, &gpio);
  HAL_GPIO_WritePin(ULTRA_TRIG_GPIO, ULTRA_TRIG_PIN, GPIO_PIN_RESET);

  gpio.Pin  = ULTRA_ECHO_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(ULTRA_ECHO_GPIO, &gpio);

  s_cm = 0xFFFFU;
  s_need_stop = 0U;
  s_last_ms = HAL_GetTick();
}

void ChassisUltra_Poll(void)
{
  uint32_t now = HAL_GetTick();
  uint16_t cm;

  if ((now - s_last_ms) < ULTRA_PERIOD_MS) {
    return;
  }
  s_last_ms = now;

  cm = MeasureOnce();
  s_cm = cm;
  if (cm != 0xFFFFU && cm < CHASSIS_ULTRA_STOP_CM) {
    s_need_stop = 1U;
  } else {
    s_need_stop = 0U;
  }
}

uint16_t ChassisUltra_GetCm(void)
{
  return s_cm;
}

uint8_t ChassisUltra_NeedStop(void)
{
  return s_need_stop;
}
