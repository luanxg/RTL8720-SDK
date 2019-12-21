#include "platform_stdlib.h"
#include "basic_types.h"
#include "diag.h"
#include "ameba_soc.h"
#include "rtl8721d_lcdc.h"
#include "bsp_rgb_lcd.h"

TaskHandle_t TOUCHTask_Handler;
TaskHandle_t RGBDEMOTask_Handler;

#define TOUCH_STK_SIZE	1024
#define TOUCH_TASK_PRIO	  6

void app_init_psram(void)
{
	u32 temp;
	PCTL_InitTypeDef  PCTL_InitStruct;

	/*set rwds pull down*/
	temp = HAL_READ32(PINMUX_REG_BASE, 0x104);
	temp &= ~(PAD_BIT_PULL_UP_RESISTOR_EN | PAD_BIT_PULL_DOWN_RESISTOR_EN);
	temp |= PAD_BIT_PULL_DOWN_RESISTOR_EN;
	HAL_WRITE32(PINMUX_REG_BASE, 0x104, temp);

	PSRAM_CTRL_StructInit(&PCTL_InitStruct);
	PSRAM_CTRL_Init(&PCTL_InitStruct);

	PSRAM_PHY_REG_Write(REG_PSRAM_CAL_PARA, 0x02030316);

	/*check psram valid*/
	HAL_WRITE32(PSRAM_BASE, 0, 0);
	assert_param(0 == HAL_READ32(PSRAM_BASE, 0));

	if(SYSCFG_CUT_VERSION_A != SYSCFG_CUTVersion()) {
		if(_FALSE == PSRAM_calibration())
			return;

		if(FALSE == psram_dev_config.psram_dev_cal_enable) {
			temp = PSRAM_PHY_REG_Read(REG_PSRAM_CAL_CTRL);
			temp &= (~BIT_PSRAM_CFG_CAL_EN);
			PSRAM_PHY_REG_Write(REG_PSRAM_CAL_CTRL, temp);
		}
	}

	/*init psram bss area*/
	memset(__psram_bss_start__, 0, __psram_bss_end__ - __psram_bss_start__);
}

void TOUCH_task(void *pvParameters)
{
	while(1){
		GUI_TOUCH_Exec();
		GUI_Delay(5);
	}
}

void RGB_DEMO_test_task(void *pvParameters)
{
	while(1){
		RealtekDemo();
	}
}

void LcdcRGBIFDemoShow(void)
{
	RgbLcdInit();
	TP_Init();
	GUI_Init();

    	xTaskCreate((TaskFunction_t )TOUCH_task,     	
                (const char*    )"TOUCH_task",   	
                (uint16_t       )TOUCH_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )TOUCH_TASK_PRIO,	
                (TaskHandle_t*  )&TOUCHTask_Handler);  


    	xTaskCreate((TaskFunction_t )RGB_DEMO_test_task,     	
                (const char*    )"TOUCH_task",   	
                (uint16_t       )4096, 
                (void*          )NULL,				
                (UBaseType_t    )TOUCH_TASK_PRIO,	
                (TaskHandle_t*  )&RGBDEMOTask_Handler);  
}

void main(void)
{
	app_init_psram();
	/*pinmux config*/
	u16 Pin_Func = PINMUX_FUNCTION_LCD;
	Pinmux_Config(_PA_19, Pin_Func);  /*D0*/
	Pinmux_Config(_PA_20, Pin_Func);  /*D1*/
	Pinmux_Config(_PA_23, Pin_Func);  /*D2*/	
	Pinmux_Config(_PA_24, Pin_Func);  /*D3*/
	Pinmux_Config(_PA_25, Pin_Func);  /*D9*/
	Pinmux_Config(_PA_26, Pin_Func);  /*D8*/
	Pinmux_Config(_PA_28, Pin_Func);  /*D7*/
	Pinmux_Config(_PA_30, Pin_Func);  /*D6*/
	Pinmux_Config(_PA_31, Pin_Func);  /*D4*/
	Pinmux_Config(_PB_0, Pin_Func);  /*D5*/
	Pinmux_Config(_PB_8, Pin_Func);  /*D10*/
	Pinmux_Config(_PB_9, Pin_Func);  /*D11*/
	Pinmux_Config(_PB_10, Pin_Func);  /*D12*/
	Pinmux_Config(_PB_11, Pin_Func);  /*D13*/
	Pinmux_Config(_PB_18, Pin_Func);  /*D14*/
	Pinmux_Config(_PB_19, Pin_Func);  /*D15*/
	Pinmux_Config(_PB_20, Pin_Func);  /*VSYNC*/
	Pinmux_Config(_PB_21, Pin_Func);  /*RS*/
	Pinmux_Config(_PB_22, Pin_Func);  /*HSYNC*/
	Pinmux_Config(_PB_23, Pin_Func);  /*DCLK*/
	Pinmux_Config(_PB_28, Pin_Func);  /*ENABLE*/

	LcdcRGBIFDemoShow();

	vTaskStartScheduler();
	while(1);
	
}
