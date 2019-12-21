#include "duerapp_audio.h"
#include "duerapp_rl6548_audio.h"
#include "rtl8721d_audio.h"
#include <platform/platform_stdlib.h>
#include "rl6548.h"
#include "duerapp.h"
#include "duerapp_media.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "lightduer_types.h"


static SP_RX_INFO sp_rx_info;
static _sema audio_rx_sema = NULL;
static u32 recorder_has_inited = 0;
static char* audio_rx_pbuf;
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];
SRAM_NOCACHE_DATA_SECTION 
static u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_full_buf[SP_FULL_BUF_SIZE];

static u8 *sp_get_ready_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	if (prx_block->rx_gdma_own)
		return NULL;
	else{
		return prx_block->rx_addr;
	}
}

static void sp_read_rx_page(u8 *dst, u32 length)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	memcpy(dst, prx_block->rx_addr, length);
	prx_block->rx_gdma_own = 1;
	sp_rx_info.rx_usr_cnt++;
	if (sp_rx_info.rx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_rx_info.rx_usr_cnt = 0;
	}
}

static void sp_release_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (sp_rx_info.rx_full_flag){
	}
	else{
		prx_block->rx_gdma_own = 0;
		sp_rx_info.rx_gdma_cnt++;
		if (sp_rx_info.rx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_rx_info.rx_gdma_cnt = 0;
		}
	}
}

static u8 *sp_get_free_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (prx_block->rx_gdma_own){
		sp_rx_info.rx_full_flag = 0;
		return prx_block->rx_addr;
	}
	else{
		sp_rx_info.rx_full_flag = 1;
		return sp_rx_info.rx_full_block.rx_addr;	//for audio buffer full case
	}
}

static u32 sp_get_free_rx_length(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

	if (sp_rx_info.rx_full_flag){
		return sp_rx_info.rx_full_block.rx_length;
	}
	else{
		return prx_block->rx_length;
	}
}

static void sp_rx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 rx_addr;
	u32 rx_length;
	
	GDMA_InitStruct = &(gs->SpRxGdmaInitStruct);
	
	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

	sp_release_rx_page();

	audio_rx_pbuf = sp_get_ready_rx_page();
	xSemaphoreGiveFromISR(audio_rx_sema, NULL);
	
	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	GDMA_SetDstAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_length>>2);	
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	//PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k

	RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);	

	//set clock divider to gen clock sample_rate*256 from 98.304M.
	switch(psp_obj->sample_rate){
		case SR_48K:
			div = 8;
			break;
		case SR_96K:
			div = 4;
			break;
		case SR_32K:
			div = 12;
			break;
		case SR_16K:
			div = 24;
			break;
		case SR_8K:
			div = 48;
			break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);	

#if CONFIG_DMIC_SEL
	Pinmux_Config(_PB_24, PINMUX_FUNCTION_DMIC);
	Pinmux_Config(_PB_25, PINMUX_FUNCTION_DMIC);
#endif
}

static void sp_init_rx_variables(void)
{
	int i;

	assert_param(app_mpu_nocache_check(sp_rx_buf));

	sp_rx_info.rx_full_block.rx_addr = (u32)sp_full_buf;
	sp_rx_info.rx_full_block.rx_length = (u32)SP_FULL_BUF_SIZE;
	
	sp_rx_info.rx_gdma_cnt = 0;
	sp_rx_info.rx_usr_cnt = 0;
	sp_rx_info.rx_full_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_rx_info.rx_block[i].rx_gdma_own = 1;
		sp_rx_info.rx_block[i].rx_addr = sp_rx_buf+i*SP_DMA_PAGE_SIZE;
		sp_rx_info.rx_block[i].rx_length = SP_DMA_PAGE_SIZE;
	}
}

static void audio_gdma_start()
{
	u32 rx_addr;
	u32 rx_length;

	AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_RxStart(AUDIO_SPORT_DEV, ENABLE);	

	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	AUDIO_SP_RXGDMA_Init(0, &SPGdmaStruct.SpRxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_rx_complete, rx_addr, rx_length);
	recorder_has_inited = 1;

}

static void audio_adc_init_first()
{
	pSP_OBJ psp_obj;
	sp_obj.sample_rate = SR_16K;
	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo =  CH_MONO; //CH_STEREO; //
	sp_obj.mono_channel_sel = RX_CH_LR;

#if CONFIG_DMIC_SEL
	sp_obj.direction =APP_DMIC_IN;
#else
	sp_obj.direction = APP_LINE_OUT | APP_AMIC_IN;
	if(sp_obj.mono_stereo == CH_MONO ) {
		sp_obj.mono_channel_sel = RX_CH_RR;
	}
#endif
	psp_obj = (pSP_OBJ)&sp_obj;

	sp_init_hal(psp_obj);
	CODEC_SetAdcGain(0x6F, 0x6F);
	sp_init_rx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;
	SP_InitStruct.SP_SelRxCh = psp_obj->mono_channel_sel;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
}

static void audio_adc_init()
{
	pSP_OBJ psp_obj;
	sp_obj.sample_rate = SR_16K;
	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo =  CH_MONO; //CH_STEREO; //
	sp_obj.mono_channel_sel = RX_CH_LR;

#if CONFIG_DMIC_SEL
	sp_obj.direction =APP_DMIC_IN;
#else
	sp_obj.direction = APP_AMIC_IN;
	if(sp_obj.mono_stereo == CH_MONO ) {
		sp_obj.mono_channel_sel = RX_CH_RR;
	}
#endif
	psp_obj = (pSP_OBJ)&sp_obj;

	sp_init_hal(psp_obj);
	CODEC_SetAdcGain(0x6F, 0x6F);
	sp_init_rx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;
	SP_InitStruct.SP_SelRxCh = psp_obj->mono_channel_sel;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
}

static void audio_recorder_thread(void* param)
{
	int ret;
	while(1){
		if(xSemaphoreTake(audio_rx_sema, 0xFFFFFFFF) != pdTRUE) continue;

		if(RECORDER_START != pDuerAppInfo->duer_rec_state){
			vTaskDelay(100);
			continue;
		}

		sp_read_rx_page((u8 *)audio_rx_pbuf, SP_DMA_PAGE_SIZE);
#if SDCARD_RECORD
		audio_recoder_sdcard_rx_complete((u8*)audio_rx_pbuf, AUDIO_RX_DMA_PAGE_SIZE);
#endif
		ret = duer_voice_send((u8*)audio_rx_pbuf, AUDIO_RX_DMA_PAGE_SIZE);

		if (-0x78 == ret){
			DUER_LOGE("send fail\r\n");
			audio_record_stop();
			duer_voice_stop();
			duer_voice_terminate();
		}
	}
exit:
	vTaskDelete(NULL);
}

void audio_record_thread_create()
{
	rtw_init_sema(&audio_rx_sema, 0);
	recorder_has_inited = 0;
	audio_adc_init_first();
	if(xTaskCreate(audio_recorder_thread, ((const char*)"audio_recorder_thread"), 256, NULL, tskIDLE_PRIORITY + 6, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(audio_recorder_thread) failed", __FUNCTION__);	
}

static void initialize_audio_as_recorder(void)
{

	DUER_LOGI("Initialize audio as recorder.");
	if(pDuerAppInfo->audio_has_inited){
		audio_deinit();
		pDuerAppInfo->audio_has_inited = 0;
	}
	audio_adc_init();
	audio_gdma_start();
	pDuerAppInfo->audio_has_inited = 1;
}

void audio_record_start()
{
	if (RECORDER_STOP == pDuerAppInfo->duer_rec_state) {
		pDuerAppInfo->voice_is_triggered = 0;
		pDuerAppInfo->audio_is_pocessing = 1;
		duerapp_gpio_led_mode(DUER_LED_ON);
		pDuerAppInfo->duer_rec_state = RECORDER_START;	
		initialize_audio_as_recorder();
		
#if SDCARD_RECORD
		duer_sdcard_send(REC_START, NULL, 0);
		sdcard_record_len = 0;
#endif
		if (DUER_OK != duer_dcs_on_listen_started()) {
			DUER_LOGE ("duer_dcs_on_listen_started failed!");
		}		
		duer_voice_start(16000);
	}
}

void audio_record_deinit(void)
{
	if(recorder_has_inited){
		DUER_LOGI("Audio deinit recorder.\n");
		AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, DISABLE);
		AUDIO_SP_RxStart(AUDIO_SPORT_DEV, DISABLE);
		/* Rx GDMA ch disable */
		GDMA_ClearINT(SPGdmaStruct.SpRxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpRxGdmaInitStruct.GDMA_ChNum);
		GDMA_Cmd(SPGdmaStruct.SpRxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpRxGdmaInitStruct.GDMA_ChNum, DISABLE);
		GDMA_ChnlFree(SPGdmaStruct.SpRxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpRxGdmaInitStruct.GDMA_ChNum);
		recorder_has_inited = 0;
	}
}

void audio_record_stop()
{
	DUER_LOGI("Recorder Stop, state:%d", pDuerAppInfo->duer_rec_state);
	if (RECORDER_START == pDuerAppInfo->duer_rec_state)
	{
		if(pDuerAppInfo->audio_has_inited){
		audio_deinit();
		pDuerAppInfo->audio_has_inited = 0;
	}
		pDuerAppInfo->duer_rec_state = RECORDER_STOP;
		duerapp_gpio_led_mode(DUER_LED_OFF);
#if SDCARD_RECORD
		duer_sdcard_send(REC_STOP, NULL, sdcard_record_len);
#endif
		pDuerAppInfo->audio_is_pocessing = 0;
	}
}

