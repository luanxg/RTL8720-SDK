#include "duerapp.h"
#include "duerapp_audio.h"
#include <mp3_codec.h>
#include "duerapp_media.h"
#include "Duerapp_peripheral_audio.h"

extern T_APP_DUER *pDuerAppInfo;

mp3_decoder_t mp3 = NULL;
SDRAM_BSS_SECTION signed short pcm_buf[AUDIO_MAX_DMA_PAGE_SIZE];

static int need_pause_audio = 0;
static _sema audio_pause_sema = NULL;
static _sema audio_resume_sema = NULL;

extern void play_tone_thread(void* param);
extern void tone_enqueue(Tones tone);

void play_tone_pause_audio()
{
	DUER_LOGD("play_tone_pause_audio(): audio_is_playing %d\n", pDuerAppInfo->audio_is_playing);

	while (pDuerAppInfo->audio_is_playing) {
		need_pause_audio = 1;
		if (rtw_down_timeout_sema(&audio_pause_sema, 50) == pdTRUE)
			break;
	}

	if (pDuerAppInfo->audio_is_playing == 0)
		need_pause_audio = 0;
}

void play_tone_resume_audio()
{
	DUER_LOGD("play_tone_resume_audio(): need_pause_audio %d\n", need_pause_audio);
	if (need_pause_audio == 1) {
		need_pause_audio = 0;
		rtw_up_sema(&audio_resume_sema);
	}
}

int is_audio_need_pause()
{
	return need_pause_audio;
}

void audio_play_pause_resume()
{
	DUER_LOGD("audio_play_pause_resume(): Paused\n");
	rtw_up_sema(&audio_pause_sema);
	rtw_down_sema(&audio_resume_sema);
	DUER_LOGD("audio_play_pause_resume(): Resumed\n");
}

void duer_reset_media_status() {
	if (pDuerAppInfo->audio_is_pocessing == 1) {
		duer_voice_terminate();
	}
}

void voice_trigger_irq_handler (uint32_t id, gpio_irq_event event)
{	
	DUER_LOGI("enter voice irq(%d,%d)",pDuerAppInfo->audio_is_playing,pDuerAppInfo->audio_is_pocessing);
	if (!pDuerAppInfo->started){
		DUER_LOGI("waiting for starting dueros ...");
		return;
	}
		
	if (pDuerAppInfo->audio_is_playing == 1) {
		pDuerAppInfo->stop_by_voice_irq = 1;
		pDuerAppInfo->duer_play_stop_flag = 1;
		DUER_LOGI("stop media play");
		return;
	}

	if (pDuerAppInfo->audio_is_pocessing == 0 && pDuerAppInfo->voice_is_triggered == 0) {
		pDuerAppInfo->voice_is_triggered = 1;
		tone_enqueue(TONE_HELLO);
	} else {
		// stop recoder earlier
		DUER_LOGI("recoder earlier!! voice irq => started %d,playing %d,recording %d",
				  pDuerAppInfo->started, pDuerAppInfo->audio_is_playing, pDuerAppInfo->audio_is_pocessing);
	}
}

void initialize_audio(void)
{
	DUER_LOGI("Initializing audio codec....\n");

	init_voice_trigger_irq(voice_trigger_irq_handler);

	//init interface with codec

	audio_record_thread_create();
	
	
	rtw_init_sema(&audio_pause_sema, 0);
	rtw_init_sema(&audio_resume_sema, 0);

	mp3 = mp3_create();
	if (!mp3)
	{
		DUER_LOGE("MP3 context create failed.\n");
	}

	if (xTaskCreate(play_tone_thread, ((const char*)"play_tone_thread"), 512, NULL, tskIDLE_PRIORITY + 5, NULL) != pdPASS)
		DUER_LOGE("Create example play_tone_thread failed!");

	DUER_LOGI("Audio codec initialized.\n");
}

