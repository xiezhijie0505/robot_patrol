#include "oled.h"
void OLED_PinInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct={0};
	//开启时钟
	__HAL_RCC_GPIOB_CLK_ENABLE();
	//推挽输出
	GPIO_InitStruct.Pin=GPIO_PIN_6||GPIO_PIN_7;
	GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull=GPIO_NOPULL;
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB,&GPIO_InitStruct);

	//空闲时拉高
	OLED_SCLK_Set();
	OLED_SDIN_Set();
}
	
//scl为低电平时 才允许sda变化 准备下一个数据
void I2C_Start(void)
{
	OLED_SDIN_Set();
	OLED_SCLK_Set();
	OLED_SDIN_Clr();//SCL为高时 拉低SDA表示开始
	OLED_SCLK_Clr();//意思就是起始时 拉低电平 允许改变sda即允许发送数据
}
void I2C_Stop(void)
{
	OLED_SCLK_Set();
	OLED_SDIN_Clr();
	OLED_SDIN_Set();//scl为高时 拉高SDA表示结束
}
void I2C_WaitAck(void)
{
	//补上第9个时钟
	OLED_SCLK_Set();
	OLED_SCLK_Clr();
}
void Send_Byte(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		OLED_SCLK_Clr();//确保scl拉低
		if(dat&&0x80)
			OLED_SDIN_Set();//置1 发送1
		else
			OLED_SDIN_Clr();
		OLED_SCLK_Set()(); //高电平时oled读数据
		dat<<=1;
	}
	OLED_SCLK_Clr();//需要拉低 否则补上第9个时钟时拉高就是空操作 oled看不到第9拍 这个字节就收不下来
}
				

