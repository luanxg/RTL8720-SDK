#include "lightduer_log.h"
#include "lightduer_dcs.h"
#include "stdbool.h"

#include "lightduer_ds_log_audio.h"
#include "duerapp.h"
#include "duerapp_audio.h"

extern T_APP_DUER *pDuerAppInfo;


void open_mic_thread(void *param) {
	audio_record_start();
	vTaskDelete(NULL);
}

void duer_dcs_listen_handler(void)
{
	DUER_LOGI ("Listen start");
	
	//audio_record_start();
	if (xTaskCreate(open_mic_thread, ((const char*)"open_mic_thread"), 256, NULL, tskIDLE_PRIORITY + 5, NULL) != pdPASS) {
		printf("\n\r%s xTaskCreate(open_mic_thread) failed", __FUNCTION__);
	}
}

void duer_dcs_stop_listen_handler(void)
{	
	if(pDuerAppInfo->duer_rec_state != RECORDER_STOP){
		DUER_LOGI ("Listen stop");
		audio_record_stop();
		duer_voice_stop();
		duer_voice_terminate();
	} else{
		DUER_LOGI ("recorder has stopped");
	}
	
}

void duer_dcs_speak_handler(const char *url)
{
	DUER_LOGI("Speak url: %s", url);	
	duer_media_speak_play(url);
}

void duer_dcs_stop_speak_handler(void)
{
	DUER_LOGI ("speech stop");
	//duer_media_speak_stop();
}

#if 0
//static char* new_url = "http://pdhrk6z9u.bkt.clouddn.com/U%20R%2011025%20%E5%8D%95%E5%A3%B0%E9%81%93.mp3";
static char* new_url = "http://pdhrk6z9u.bkt.clouddn.com/U%20R%2011025%20%E5%8F%8C%E5%A3%B0%E9%81%93.mp3";

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
	DUER_LOGI ("Audio url: %s, 0", new_url);
	duer_media_audio_start(new_url,audio_info->offset);
}
#else
void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
	DUER_LOGI("Audio url: %s,offset %d", audio_info->url,audio_info->offset);
	duer_media_audio_start(audio_info->url,audio_info->offset);
}
#endif

void duer_dcs_audio_stop_handler(void)
{
	DUER_LOGI ("Audio stop");
	//duer_media_audio_stop();
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
	DUER_LOGI ("Audio seek : %s,offset %d", audio_info->url,audio_info->offset);
	duer_media_audio_resume(audio_info->url,audio_info->offset);	
}

void duer_dcs_audio_pause_handler(void)
{
	DUER_LOGI ("PAUSE HANDLER ...");
	duer_media_audio_pause();
}

void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
	if (volume) {
		*volume = duer_media_get_volume();
	}
	if (is_mute) {
		*is_mute = duer_media_get_mute();
	}
}

void duer_dcs_volume_set_handler(int volume)
{
	DUER_LOGI ("Set Volume Handler ...");
	duer_media_set_volume(volume);
}

void duer_dcs_volume_adjust_handler(int volume)
{	
	DUER_LOGI ("Adjust Volume Handler ...");
	duer_media_volume_change(volume);
}

void duer_dcs_mute_handler(duer_bool is_mute)
{
	HTTP_LOGI("Mute Handler...");
	duer_media_set_mute(is_mute);
}

int duer_dcs_audio_get_play_progress(void)
{
	int pos = duer_media_audio_get_position();
	DUER_LOGI ("PAUSE offset : %d", pos);
	return pos;
}

duer_status_t duer_dcs_render_card_handler(baidu_json *payload)
{
	baidu_json *type = NULL;
	baidu_json *content = NULL;
	duer_status_t ret = DUER_OK;

	do {
		if (!payload) {
			ret = DUER_ERR_FAILED;
			break;
		}

		type = baidu_json_GetObjectItem(payload, "type");
		if (!type) {
			ret = DUER_ERR_FAILED;
			break;
		}

		if (strcmp("TextCard", type->valuestring) == 0) {
			content = baidu_json_GetObjectItem(payload, "content");
			if (!content) {
				ret = DUER_ERR_FAILED;
				break;
			}
			DUER_LOGI("Render card content: %s", content->valuestring);
		}
	} while (0);

	return ret;
}

duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    DUER_LOGI("ASR result: %s", text);
    return DUER_OK;
}



