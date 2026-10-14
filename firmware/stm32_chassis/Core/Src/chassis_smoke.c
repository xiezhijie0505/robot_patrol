#include "chassis_smoke.h"
#include "main.h"

/* ---- 接线：不对就改这里 ---- */
#define SMOKE_GPIO    GPIOA
#define SMOKE_PIN     GPIO_PIN_2
#define BUZZ_GPIO     GPIOA
#define BUZZ_PIN      GPIO_PIN_3

#define SMOKE_DEBOUNCE_MS  50U

static uint8_t  s_alarm;
static uint8_t  s_event;
static uint8_t  s_last_raw;
static uint32_t s_last_change_ms;

static uint8_t ReadRawAlarm(void)
{
  GPIO_PinState st = HAL_GPIO_ReadPin(SMOKE_GPIO, SMOKE_PIN);
#if CHASSIS_SMOKE_ACTIVE_LOW
  return (st == GPIO_PIN_RESET) ? 1U : 0U;
#else
  return (st == GPIO_PIN_SET) ? 1U : 0U;
#endif
}

static void BuzzerSet(uint8_t on)
{
  HAL_GPIO_WritePin(BUZZ_GPIO, BUZZ_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void ChassisSmoke_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  gpio.Pin  = SMOKE_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SMOKE_GPIO, &gpio);

  gpio.Pin   = BUZZ_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZ_GPIO, &gpio);
  BuzzerSet(0);

  s_alarm = 0U;
  s_event = 0U;
  s_last_raw = ReadRawAlarm();
  s_last_change_ms = HAL_GetTick();
}

void ChassisSmoke_Poll(void)
{
  uint8_t raw = ReadRawAlarm();
  uint32_t now = HAL_GetTick();

  if (raw != s_last_raw) {
    s_last_raw = raw;
    s_last_change_ms = now;
    return;
  }
  if ((now - s_last_change_ms) < SMOKE_DEBOUNCE_MS) {
    return;
  }

  if (raw && !s_alarm) {
    s_event = 1U; /* 上升沿：新报警 */
  }
  s_alarm = raw;
  BuzzerSet(s_alarm);
}

uint8_t ChassisSmoke_IsAlarm(void)
{
  return s_alarm;
}

uint8_t ChassisSmoke_TakeEvent(void)
{
  uint8_t e = s_event;
  s_event = 0U;
  return e;
}
