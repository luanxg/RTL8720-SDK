
#include "ameba_soc.h"
#include "ftl_int.h"
#include "main.h"

#if defined ( __ICCARM__ )
#pragma section=".image2.psram.bss"

u8* __psram_bss_start__;
u8* __psram_bss_end__;
#endif

#if defined(CONFIG_FTL_ENABLED)
extern const u8 ftl_phy_page_num;
extern const u32 ftl_phy_page_start_addr;

void app_ftl_init(void)
{
	ftl_init(ftl_phy_page_start_addr, ftl_phy_page_num);
}
#endif

#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
extern VOID wlan_network(VOID);
extern u32 GlobalDebugEnable;
#endif
void app_init_psram(void);
void app_captouch_init(void);
void app_keyscan_init(u8 reset_status);

void app_init_debug(void)
{
	u32 debug[4];

	debug[LEVEL_ERROR] = BIT(MODULE_BOOT);
	debug[LEVEL_WARN]  = 0x0;
	debug[LEVEL_INFO]  = BIT(MODULE_BOOT);
	debug[LEVEL_TRACE] = 0x0;

	debug[LEVEL_ERROR] = 0xFFFFFFFF;

	LOG_MASK(LEVEL_ERROR, debug[LEVEL_ERROR]);
	LOG_MASK(LEVEL_WARN, debug[LEVEL_WARN]);
	LOG_MASK(LEVEL_INFO, debug[LEVEL_INFO]);
	LOG_MASK(LEVEL_TRACE, debug[LEVEL_TRACE]);
}

u32 app_psram_suspend(u32 expected_idle_time, void *param)
{
	/* To avoid gcc warnings */
	( void ) expected_idle_time;
	( void ) param;
	
	u32 temp;
	u32 temps;
	u8 * psram_ca;
	u32 PSRAM_datard;
	u32 PSRAM_datawr;
	u8 PSRAM_CA[6];
	psram_ca = &PSRAM_CA[0];

	if((SLEEP_PG == pmu_get_sleep_type()) || (FALSE == psram_dev_config.psram_dev_retention)) {
		/*Close PSRAM 1.8V power to save power*/
		temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1);
		temp &= ~(BIT_LDO_PSRAM_EN);
		HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1, temp);	
	} else {
		/*set psram enter halfsleep mode*/
		PSRAM_CTRL_CA_Gen(psram_ca, (BIT11 | BIT0), PSRAM_LINEAR_TYPE, PSRAM_REG_SPACE, PSRAM_READ_TRANSACTION);
		PSRAM_CTRL_DPin_Reg(psram_ca, &PSRAM_datard, PSRAM_READ_TRANSACTION);
		PSRAM_datard |= BIT5;
		temp = PSRAM_datard & 0xff;
		temps = (PSRAM_datard>>8) & 0xff;
		PSRAM_datawr = (temp<<8 | temp<<24) | (temps | temps<<16);
		PSRAM_CTRL_CA_Gen(psram_ca, (BIT11 | BIT0), PSRAM_LINEAR_TYPE, PSRAM_REG_SPACE, PSRAM_WRITE_TRANSACTION);
		PSRAM_CTRL_DPin_Reg(psram_ca, &PSRAM_datawr, PSRAM_WRITE_TRANSACTION);
	}

	return TRUE;
}

u32 app_psram_resume(u32 expected_idle_time, void *param)
{
	/* To avoid gcc warnings */
	( void ) expected_idle_time;
	( void ) param;

	//u32 temp;

	if((SLEEP_PG == pmu_get_sleep_type()) || (FALSE == psram_dev_config.psram_dev_retention)) {
		app_init_psram();
	} else {
		/*dummy read to make psram exit half sleep mode*/
		//temp = HAL_READ32(PSRAM_BASE, 0);

		/*wait psram exit half sleep mode*/
		DelayUs(100);

		DCache_Invalidate((u32)PSRAM_BASE, 32);
	}

	return TRUE;
}

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

#if defined ( __ICCARM__ )
	__psram_bss_start__ = (u8*)__section_begin(".image2.psram.bss");
	__psram_bss_end__   = (u8*)__section_end(".image2.psram.bss");	
#endif

	/*init psram bss area*/
	memset(__psram_bss_start__, 0, __psram_bss_end__ - __psram_bss_start__);

	pmu_register_sleep_callback(PMU_PSRAM_DEVICE, (PSM_HOOK_FUN)app_psram_suspend, NULL, (PSM_HOOK_FUN)app_psram_resume, NULL);
}

static void* app_mbedtls_calloc_func(size_t nelements, size_t elementSize)
{
	size_t size;
	void *ptr = NULL;

	size = nelements * elementSize;
	ptr = pvPortMalloc(size);

	if(ptr)
		_memset(ptr, 0, size);

	return ptr;
}

void app_mbedtls_rom_init(void)
{
	mbedtls_platform_set_calloc_free(app_mbedtls_calloc_func, vPortFree);
	//rom_ssl_ram_map.use_hw_crypto_func = 1;
	rtl_cryptoEngine_init();
}

VOID app_start_autoicg(VOID)
{
	u32 temp = 0;
	
	temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA);
	temp |= BIT_HSYS_PLFM_AUTO_ICG_EN;
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA, temp);
}

VOID app_pmu_init(VOID)
{
	if (BKUP_Read(BKUP_REG0) & BIT_SW_SIM_RSVD){
		return;
	}

	pmu_set_sleep_type(SLEEP_PG);

	/* if wake from deepsleep, that means we have released wakelock last time */
	if (SOCPS_DsleepWakeStatusGet() == TRUE) {
		pmu_set_sysactive_time(2);
		pmu_release_wakelock(PMU_OS);
		pmu_tickless_debug(ENABLE);
	}	
}

/* enable or disable BT shared memory */
/* if enable, KM4 can access it as SRAM */
/* if disable, just BT can access it */
/* 0x100E_0000	0x100E_FFFF	64K */
VOID app_shared_btmem(u32 NewStatus)
{
	u32 temp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA);

	if (NewStatus == ENABLE) {
		temp |= BIT_HSYS_SHARE_BT_MEM;
	} else {
		temp &= ~BIT_HSYS_SHARE_BT_MEM;
	}	
	
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_PLATFORM_PARA, temp);
}


//default main
int main(void)
{
	if (wifi_config.wifi_ultra_low_power &&
		wifi_config.wifi_app_ctrl_tdma == FALSE) {
		SystemSetCpuClk(CLK_KM4_100M);
	}	
	InterruptRegister(IPC_INTHandler, IPC_IRQ, (u32)IPCM0_DEV, 10);		
	InterruptEn(IPC_IRQ, 10);

#ifdef CONFIG_MBED_TLS_ENABLED
	app_mbedtls_rom_init();
#endif
	//app_init_debug();

	/* init console */
	shell_recv_all_data_onetime = 1;
	shell_init_rom(0, 0);	
	shell_init_ram();
	ipc_table_init();

	/* Register Log Uart Callback function */
	InterruptRegister((IRQ_FUN) shell_uart_irq_rom, UART_LOG_IRQ, (u32)NULL, 10);
	InterruptEn(UART_LOG_IRQ_LP,10);

#ifdef CONFIG_FTL_ENABLED
	app_ftl_init();
#endif

#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	rtw_efuse_boot_write();

	/* pre-processor of application example */
	pre_example_entry();

	wlan_network();
	
	/* Execute application example */
	example_entry();
#endif

	app_start_autoicg();
	//app_shared_btmem(ENABLE);

	app_pmu_init();

	if ((BKUP_Read(0) & BIT_KEY_TOUCH_ENABLE)) {
		app_captouch_init(); /* 1uA */
		app_keyscan_init(FALSE); /* 5uA */
	}
	
	app_init_debug();

	if(TRUE == psram_dev_config.psram_dev_enable) {
		app_init_psram();
	}

	DBG_8195A("M4U:%d \n", RTIM_GetCount(TIMM05));
	/* Enable Schedule, Start Kernel */
	vTaskStartScheduler();
}

