/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_event.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Key functions to achieve.
 */
#include <stdio.h>
#include <string.h>

#include "duerapp.h"
#include "duerapp_event.h"
#include "lightduer_dcs.h"
#include "lightduer_voice.h"
#include "lightduer_types.h"
#include "duerapp_recorder.h"
#include "duerapp_media.h"
#include "duerapp_config.h"

#define VOLUME_STEP (20)

extern T_APP_DUER *pDuerAppInfo;

void event_play_pause()
{
	DUER_LOGI ("KEY_PAUSE");
	static bool play_pause = false;
	duer_media_audio_pause();
	if (play_pause) {
		duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
		play_pause = false;
	} else {
		duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
		play_pause = true;
	}
}

static void event_record_start()
{
	DUER_LOGI ("KEY_RECORD");
	voice_trigger_irq_handler(0, IRQ_NONE);
}

static void event_record_stop(void)
{
	int ret = 0;
	
	DUER_LOGI ("KEY_RECORD Stop");
	audio_record_stop();
	duer_voice_stop();
	duer_voice_terminate();

	ret = duer_dcs_on_listen_stopped();
	if (ret != 0) {
		DUER_LOGI("KEY_RECORD Stop listern fail!");
	}
}

static void event_previous_song()
{
	DUER_LOGI ("KEY_PREVIOUS");
	//duer_media_speak_stop();
	pDuerAppInfo->duer_play_stop_flag = 1;
	duer_dcs_send_play_control_cmd(DCS_PREVIOUS_CMD);
}

static void event_next_song()
{
	DUER_LOGI ("KEY_NEXT");
	//duer_media_speak_stop();
	pDuerAppInfo->duer_play_stop_flag = 1;
	duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
}

static void event_volume_incr()
{
	DUER_LOGI ("KEY_VOL_INC");
	duer_media_volume_change(VOLUME_STEP);
}

static void event_volume_decr()
{
	DUER_LOGI ("KEY_VOL_DEC");
	duer_media_volume_change(-VOLUME_STEP);
}

static void event_volune_mute()
{

	DUER_LOGI ("KEY_VOL_MUTE : %d", pDuerAppInfo->mute);
	if (pDuerAppInfo->mute) {
		pDuerAppInfo->mute = false;
	} else {
		pDuerAppInfo->mute = true;
	}
	duer_media_set_mute(pDuerAppInfo->mute);
}


/**
 * [duer_event_test description]
 * @param kbd_event :
 *   PLAY_PAUSE	   = 0x7A,
     RECORD_START  = 0x78,
     PREVIOUS_SONG = 0x61,
     NEXT_SONG     = 0x64,
     VOLUME_INCR   = 0x77,
     VOLUME_DECR   = 0x73,
     VOLUME_MUTE   = 0x65;
 */
void duer_event_test(char kbd_event)
{
	switch ( kbd_event ) {
	case PLAY_PAUSE :
		event_play_pause();
		break;
	/*case RECORD_START :
		voice_trigger_irq_handler (0, IRQ_NONE);
		break;*/
	case RECORD_START : 		
		if (pDuerAppInfo->duer_rec_state == RECORDER_STOP) {
			event_record_start();
		} else {
			/* stop recoder earlier */
			DUER_LOGI("recoder earlier!!\n");
		}
		break;
	case RECORD_STOP:
		if (pDuerAppInfo->duer_rec_state == RECORDER_START) {
			event_record_stop();
		}
		break;

	case PREVIOUS_SONG :
		event_previous_song();
		break;
	case NEXT_SONG :
		event_next_song();
		break;
	case VOLUME_INCR :
		event_volume_incr();
		break;
	case VOLUME_DECR :
		event_volume_decr();
		break;
	case VOLUME_MUTE :
		event_volune_mute();
		break;
	default :
		break;
	}
}

