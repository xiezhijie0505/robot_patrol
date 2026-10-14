#include "chassis_motor.h"
#include "main.h"

/* TB6612 控制脚 */
#define M_AIN1_GPIO   GPIOB
#define M_AIN1_PIN    GPIO_PIN_12
#define M_AIN2_GPIO   GPIOB
#define M_AIN2_PIN    GPIO_PIN_13
#define M_BIN1_GPIO   GPIOB
#define M_BIN1_PIN    GPIO_PIN_14
#define M_BIN2_GPIO   GPIOB
#define M_BIN2_PIN    GPIO_PIN_15
#define M_STBY_GPIO   GPIOB
#define M_STBY_PIN    GPIO_PIN_8

#define M_PWM_PERIOD  999U

static TIM_HandleTypeDef s_htim3;

static void Motor_GpioInit(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* 方向 + STBY：推挽输出 */
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin   = M_AIN1_PIN | M_AIN2_PIN | M_BIN1_PIN | M_BIN2_PIN | M_STBY_PIN;
  HAL_GPIO_Init(GPIOB, &gpio);

  HAL_GPIO_WritePin(M_STBY_GPIO, M_STBY_PIN, GPIO_PIN_SET);

  /* PWMA/PWMB：TIM3_CH1/CH2 -> PA6/PA7 */
  gpio.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);
}

static void Motor_PwmInit(void)
{
  TIM_OC_InitTypeDef oc = {0};

  __HAL_RCC_TIM3_CLK_ENABLE();

  s_htim3.Instance               = TIM3;
  s_htim3.Init.Prescaler         = 71;   /* 72MHz/(71+1)=1MHz */
  s_htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  s_htim3.Init.Period            = M_PWM_PERIOD; /* 1kHz PWM */
  s_htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  s_htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&s_htim3) != HAL_OK) {
    Error_Handler();
  }

  oc.OCMode     = TIM_OCMODE_PWM1;
  oc.Pulse      = 0;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&s_htim3, &oc, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&s_htim3, &oc, TIM_CHANNEL_2) != HAL_OK) {
    Error_Handler();
  }

  HAL_TIM_PWM_Start(&s_htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&s_htim3, TIM_CHANNEL_2);
}

static void SetOneWheel(GPIO_TypeDef *in1_port, uint16_t in1_pin,
                        GPIO_TypeDef *in2_port, uint16_t in2_pin,
                        uint32_t channel, int16_t speed)
{
  uint32_t duty;

  if (speed > 100) {
    speed = 100;
  } else if (speed < -100) {
    speed = -100;
  }

  if (speed > 0) {
    HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
    duty = (uint32_t)speed * M_PWM_PERIOD / 100U;
  } else if (speed < 0) {
    HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_SET);
    duty = (uint32_t)(-speed) * M_PWM_PERIOD / 100U;
  } else {
    HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
    duty = 0U;
  }

  __HAL_TIM_SET_COMPARE(&s_htim3, channel, duty);
}

void ChassisMotor_Init(void)
{
  Motor_GpioInit();
  Motor_PwmInit();
  ChassisMotor_Stop();
}

void ChassisMotor_Stop(void)
{
  __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_1, 0U);
  __HAL_TIM_SET_COMPARE(&s_htim3, TIM_CHANNEL_2, 0U);

  HAL_GPIO_WritePin(M_AIN1_GPIO, M_AIN1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(M_AIN2_GPIO, M_AIN2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(M_BIN1_GPIO, M_BIN1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(M_BIN2_GPIO, M_BIN2_PIN, GPIO_PIN_RESET);
}

void ChassisMotor_SetSpeed(int16_t left, int16_t right)
{
  SetOneWheel(M_AIN1_GPIO, M_AIN1_PIN, M_AIN2_GPIO, M_AIN2_PIN,
              TIM_CHANNEL_1, left);
  SetOneWheel(M_BIN1_GPIO, M_BIN1_PIN, M_BIN2_GPIO, M_BIN2_PIN,
              TIM_CHANNEL_2, right);
}

void ChassisMotor_Poll(void)
{
  /* V1：速度已在 SetSpeed 里立刻生效，此处留空 */
}