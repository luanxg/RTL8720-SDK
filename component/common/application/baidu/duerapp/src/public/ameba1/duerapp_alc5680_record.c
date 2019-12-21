#include "FreeRTOS.h"
#include "semphr.h"

#include "duerapp.h"
#include "duerapp_media.h"
#include "duerapp_audio.h"
#include "lightduer_types.h"

static char* i2s_rx_pbuf;
char *path = "/Ameba_voice";

_sema i2s_rx_sema = NULL;

extern T_APP_DUER *pDuerAppInfo;
extern int sdcard_record_len;
_sema i2s_rx_sema = NULL;

#if 1
static void i2s_recoder_rx_complete(void *data, char* pbuf)
{
	i2s_rx_pbuf = pbuf;
	xSemaphoreGiveFromISR(i2s_rx_sema, NULL);
}

void i2s_rx_thread()
{
	int ret = 0;
	while(1){		
		if(xSemaphoreTake(i2s_rx_sema, 0xFFFFFFFF) != pdTRUE) continue;

		if(RECORDER_START != pDuerAppInfo->duer_rec_state){
			vTaskDelay(100);
			continue;
		}
		//i2s_recv_page(&i2s_obj);

#if SDCARD_RECORD
		duer_sdcard_send(REC_DATA, i2s_rx_pbuf, I2S_RX_PAGE_SIZE);
		sdcard_record_len += I2S_RX_PAGE_SIZE;
#endif
		ret = duer_voice_send(i2s_rx_pbuf, I2S_RX_PAGE_SIZE);
		i2s_recv_page(&i2s_obj);
#if 1
		if (-0x78 == ret){
			DUER_LOGE("send fail\r\n");
			audio_record_stop();
			duer_voice_stop();
			duer_voice_terminate();
		}
#endif
		
	}
}

int initialize_audio_as_recorder()
{
	if(pDuerAppInfo->i2s_has_inited){
		i2s_deinit(&i2s_obj);
		pDuerAppInfo->i2s_has_inited = 0;
	}

	DUER_LOGI("Initialize audio as recorder.");

	i2s_obj.channel_num = I2S_CH_MONO;
	i2s_obj.sampling_rate = SR_16KHZ;
	i2s_obj.word_length = WL_16b;
	i2s_obj.direction = I2S_DIR_RX;
	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	pDuerAppInfo->i2s_has_inited = 1;
	i2s_set_dma_buffer(&i2s_obj, NULL, (char*)i2s_comm_buf, I2S_DMA_PAGE_NUM, I2S_RX_PAGE_SIZE);
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_recoder_rx_complete, (uint32_t)&i2s_obj);

	//clear i2s buf
	memset(i2s_comm_buf,0x00,AUDIO_MAX_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM);
	
	DUER_LOGI("Initialize audio as recorder DONE.");
	i2s_recv_page(&i2s_obj);

	return 0;
}
	
void audio_record_thread_create()
{
	rtw_init_sema(&i2s_rx_sema, 0);
	if (xTaskCreate(i2s_rx_thread, ((const char*)"audio_rx_thread"), 1024, NULL, tskIDLE_PRIORITY + 6, NULL) != pdPASS)
		DUER_LOGE("Create audio_rx_thread failed!");
}

void audio_record_start()
{
	if (RECORDER_STOP == pDuerAppInfo->duer_rec_state) {
		duerapp_gpio_led_mode(DUER_LED_ON);

		pDuerAppInfo->duer_rec_state = RECORDER_START;		
		initialize_audio_as_recorder();

		pDuerAppInfo->voice_is_triggered = 0;
		pDuerAppInfo->audio_is_pocessing = 1;
		
		DUER_LOGI("start send voice: %s", (const char *)path);

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

void audio_record_stop()
{
	pDuerAppInfo->audio_is_pocessing = 0;
	if(pDuerAppInfo->i2s_has_inited){
		i2s_deinit(&i2s_obj);
		pDuerAppInfo->i2s_has_inited = 0;
	}

	DUER_LOGI("Recorder Stop, state:%d", pDuerAppInfo->duer_rec_state);
	if (RECORDER_START == pDuerAppInfo->duer_rec_state)
	{
		pDuerAppInfo->duer_rec_state = RECORDER_STOP;
		duerapp_gpio_led_mode(DUER_LED_OFF);
#if SDCARD_RECORD
		duer_sdcard_send(REC_STOP, NULL, sdcard_record_len);
#endif
	}
}
#endif
