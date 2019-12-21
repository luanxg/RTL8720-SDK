#include "FreeRTOS.h"
#include "task.h"

#include "duerapp.h"
#include "duerapp_media.h"
#include "duerapp_audio.h"
#include "duerapp_event.h"
#include "lightduer_types.h"
#include "lightduer_memory.h"

#define VOLUME_MAX (10)
#define VOLUME_MIX (0)

SDRAM_BSS_SECTION duer_media_info_t MP3Info = {0};
SDRAM_BSS_SECTION static duer_media_param_t audio_param = {0};
SDRAM_BSS_SECTION static duer_media_param_t speak_param = {0};

extern T_APP_DUER *pDuerAppInfo;
extern void duer_http_audio_thread(void *param);

//call this if stop dueros
void MediaURLFree(){
	SAFE_FREE(speak_param.download_url);
	SAFE_FREE(audio_param.download_url);
}

void duer_media_speak_play(const char *url)  //
{
	size_t len = 0;
	size_t copy_len = 0;
	
	if (!url || strstr( url, ".m4a")) {
		DUER_LOGW("Not mp3, NO support!");
		return;
	}
	
	if (pDuerAppInfo->voice_is_triggered || pDuerAppInfo->audio_is_pocessing) {
		DUER_LOGI("status: (%d,%d,%d)", pDuerAppInfo->voice_is_triggered, pDuerAppInfo->audio_is_playing, pDuerAppInfo->audio_is_pocessing);
		return;
	}

	memset(&speak_param, 0x00, sizeof(speak_param));
	len = strlen(url) + 1;
	SAFE_FREE(speak_param.download_url);
	speak_param.download_url = (char *)DUER_MALLOC_SDRAM(len);
	if (!speak_param.download_url) {
		DUER_LOGE("malloc sdram for speak_url fail");
		return;
	}
	snprintf(speak_param.download_url, len, "%s", url);
	
	speak_param.ddt = SPEAK;
	copy_len =  MIN(len, URL_LEN - 1);
	strncpy(MP3Info.url, speak_param.download_url,copy_len);
	
	pDuerAppInfo->duer_play_stop_flag = 1;

	DUER_LOGD("start tts: %s %p\n", speak_param.download_url,speak_param.download_url);
	if (xTaskCreate(duer_http_audio_thread, ((const char*)"speak_thread"), 1024, &speak_param, tskIDLE_PRIORITY + 4, NULL) != pdPASS)
		DUER_LOGE("Create Speak thread failed!");
}

void duer_media_speak_stop()
{
	if (MP3Info.ddt == SPEAK && pDuerAppInfo->audio_is_playing) {
		pDuerAppInfo->duer_play_stop_flag = 1;

		xSemaphoreTake(pDuerAppInfo->xSemaphoreHTTPAudioThread, portMAX_DELAY);
		xSemaphoreGive(pDuerAppInfo->xSemaphoreHTTPAudioThread);
		DUER_LOGI("stop tts\n");
	}
}

void duer_media_audio_start(const char *url, int offset)
{
	size_t len = 0;
	size_t copy_len = 0;
	
	if (!url || strstr( url, ".m4a")) {
		DUER_LOGW("not mp3, don't support now!");
		duer_event_test(NEXT_SONG);
		return;
	}

	if (pDuerAppInfo->voice_is_triggered || pDuerAppInfo->audio_is_pocessing) {
		DUER_LOGI("status: (%d,%d,%d)", pDuerAppInfo->voice_is_triggered, pDuerAppInfo->audio_is_playing, pDuerAppInfo->audio_is_pocessing);
		return;
	}

	memset(&audio_param, 0x00, sizeof(audio_param));
	len = strlen(url) + 1;
	SAFE_FREE(audio_param.download_url);
	audio_param.download_url = (char *)DUER_MALLOC_SDRAM(len);
	if (!audio_param.download_url) {
		DUER_LOGE("malloc sdram for audio_url fail");
		return;
	}
	snprintf(audio_param.download_url, len, "%s", url);
	
	audio_param.offset = offset;
	audio_param.ddt = AUDIO;

	copy_len =  MIN(len, URL_LEN - 1);
	strncpy(MP3Info.url, audio_param.download_url,copy_len);
	
	pDuerAppInfo->duer_play_stop_flag = 1;

	DUER_LOGD("start audio: %s(%p), offset %d\n", audio_param.download_url,audio_param.download_url, offset);
	if (xTaskCreate(duer_http_audio_thread, ((const char*)"audio_thread"), 1024, &audio_param, tskIDLE_PRIORITY + 4, NULL) != pdPASS)
		DUER_LOGE("Create Audio thread failed!");

}

void duer_media_audio_pause()
{
	if (MP3Info.ddt == AUDIO && pDuerAppInfo->audio_is_playing) {
		pDuerAppInfo->duer_play_stop_flag = 1;
		xSemaphoreTake(pDuerAppInfo->xSemaphoreHTTPAudioThread, portMAX_DELAY);
		xSemaphoreGive(pDuerAppInfo->xSemaphoreHTTPAudioThread);
		DUER_LOGI("pause audio\n");
	}
}

void duer_media_audio_resume(const char *url, int offset) {

	if (pDuerAppInfo->voice_is_triggered == 0) {
		pDuerAppInfo->is_resume_play = true;
		duer_media_audio_start(url, offset);	
	}else {
		DUER_LOGW("voice_is_triggered now...");
	}
}

void duer_media_audio_stop()
{
	if (MP3Info.ddt == AUDIO && pDuerAppInfo->audio_is_playing) {
		pDuerAppInfo->duer_play_stop_flag = 1;
		audio_param.offset = 0;
		xSemaphoreTake(pDuerAppInfo->xSemaphoreHTTPAudioThread, portMAX_DELAY);
		xSemaphoreGive(pDuerAppInfo->xSemaphoreHTTPAudioThread);
		DUER_LOGI("stop audio\n");
	}
}

void duer_media_player_rehabilitation() {
	if (pDuerAppInfo->stop_by_voice_irq) {
		pDuerAppInfo->voice_is_triggered = 1;
		tone_enqueue(TONE_HELLO);
	}
	pDuerAppInfo->stop_by_voice_irq = 0;
}

int duer_media_audio_get_position() {
	return MP3Info.cur_played_offset;
}

int duera_media_get_current_time() {
	if (MP3Info.ddt == AUDIO && MP3Info.is_parsed) {
		MP3Info.cur_time = (u32)(MP3Info.cur_played_offset / (float)(MP3Info.range_len) * (float)(MP3Info.duration));
		DUER_LOGI("mp3 cur time : %d\n", MP3Info.cur_time);
		return MP3Info.cur_time;
	} else {
		DUER_LOGE("MP3 Play Get Info Failed, Try play start first\n");
		return -1;
	}
}

int duer_media_get_total_time() {
	if (MP3Info.ddt == AUDIO && MP3Info.is_parsed) {
		DUER_LOGI("mp3 total time : %d\n", MP3Info.duration);
		return MP3Info.duration;
	} else {
		DUER_LOGE("MP3 Play Get Info Failed, Try play start first\n");
		return -1;
	}
}

int duer_media_get_total_len() {
	return MP3Info.range_len;
}

void duer_media_seekbytime(u32 time) {

	int offset = 0;
	int total_time = 0;
	int total_len = 0;
	if (MP3Info.ddt != AUDIO || MP3Info.is_parsed != 1) {
		DUER_LOGE("MP3 Play Seek Failed, Try play start first\n");
		return;
	}
	total_time = duer_media_get_total_time();
	if (time >= MP3Info.duration || time < 0)
	{
		DUER_LOGE("MP3 Play Seek Failed, Out of MP3 time range, total time %d, seek time %d\n", MP3Info.duration, time);
		return;
	}
	pDuerAppInfo->duer_play_stop_flag = 1;
	vTaskDelay(200);
	total_len = duer_media_get_total_len();
	offset = (u32)((time) / ((float)total_time) * total_len);
	DUER_LOGI("audio seek time %d\n", time);

	duer_media_audio_start(MP3Info.url, offset);	
}

struct duer_media_info_t* duer_media_get_info() {
	if (MP3Info.is_parsed)
		return (struct duer_media_info_t*)(&MP3Info);
	return NULL;
}

void duer_media_volume_change(int volume)
{
	pDuerAppInfo->volume += volume / 20.0;
	if (pDuerAppInfo->volume < VOLUME_MIX) {
		pDuerAppInfo->volume = 0.0;
	} else if (pDuerAppInfo->volume > VOLUME_MAX) {
		pDuerAppInfo->volume = VOLUME_MAX;
	}

	if(pDuerAppInfo->mute ){
		return;
	}

	duerapp_play_mp3_volume_set(pDuerAppInfo->volume);
	DUER_LOGD ("volume : %d", pDuerAppInfo->volume);
	if (DUER_OK == duer_dcs_on_volume_changed()) {
		DUER_LOGI ("volume change OK");
	}
}

void duer_media_set_volume(int volume)
{
	pDuerAppInfo->volume = volume;

	if(pDuerAppInfo->mute){
		return;
	}
	duerapp_play_mp3_volume_set(pDuerAppInfo->volume);

	DUER_LOGD ("volume : %d", pDuerAppInfo->volume);
	if (DUER_OK == duer_dcs_on_volume_changed()) {
		DUER_LOGI ("volume change OK");
	}
}

void duer_media_set_mute(bool mute)
{
	pDuerAppInfo->mute = mute;

	if (mute) {
		duerapp_play_mp3_volume_set(0);
	} else {		
		duerapp_play_mp3_volume_set(pDuerAppInfo->volume);
		DUER_LOGD ("volume : %d", pDuerAppInfo->volume);
	}
	duer_dcs_on_mute();
}

int duer_media_get_volume()
{
	int vol = 0;
	vol = duerapp_play_mp3_volume_get();
	if(vol == 0){
		pDuerAppInfo->mute = true;
	}
	else{
		pDuerAppInfo->mute = false;
		pDuerAppInfo->volume = vol;
	}
	DUER_LOGI("current vol %d, mute %d\n", pDuerAppInfo->volume, pDuerAppInfo->mute);
	return pDuerAppInfo->mute ? 0 : pDuerAppInfo->volume;
}

bool duer_media_get_mute()
{
	return pDuerAppInfo->mute;
}

void duerapp_media_volume_up()
{
    pDuerAppInfo->volume ++;
    if(pDuerAppInfo->volume < VOLUME_MIX)
        pDuerAppInfo->volume = VOLUME_MIX;
    if(pDuerAppInfo->volume > VOLUME_MAX)
        pDuerAppInfo->volume = VOLUME_MAX;

    duer_media_set_volume(pDuerAppInfo->volume);
}

void duerapp_media_volume_down()
{
    pDuerAppInfo->volume--;
    if(pDuerAppInfo->volume < VOLUME_MIX)
        pDuerAppInfo->volume = VOLUME_MIX;
    if(pDuerAppInfo->volume > VOLUME_MAX)
        pDuerAppInfo->volume = VOLUME_MAX;

    duer_media_set_volume(pDuerAppInfo->volume);
}

void duerapp_media_volume_max()
{
    pDuerAppInfo->volume = VOLUME_MAX;
    duer_media_set_volume(pDuerAppInfo->volume);
}

void duerapp_media_volume_min()
{
    pDuerAppInfo->volume = VOLUME_MIX;
    duer_media_set_volume(pDuerAppInfo->volume);
}
