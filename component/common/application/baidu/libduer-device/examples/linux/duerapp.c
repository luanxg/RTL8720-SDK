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
 * File: duerapp.c
 * Auth: Zhang Leliang(zhanglelaing@baidu.com)
 * Desc: Duer Application Main.
 */

#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include "lightduer.h"
#include "lightduer_voice.h"
#include "lightduer_net_ntp.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"

#include "duerapp_args.h"
#include "duerapp_config.h"
#include "lightduer_key.h"
#include "lightduer_dcs.h"
#include "lightduer_system_info.h"

#include <unistd.h>

duer_system_static_info_t g_system_static_info = {
    .os_version         = "Ubuntu 16.04.4",
    .sw_version         = "dcs3.0-demo",
    .brand              = "Baidu",
    .hardware_version   = "1.0.0",
    .equipment_type     = "PC",
    .ram_size           = 4096 * 1024,
    .rom_size           = 2048 * 1024,
};

extern int duer_init_play(void);

static void duer_test_start(const char *profile);

static bool s_started = false;
static const char *s_pro_path = NULL;
static int s_reconnect_time = 1;

static void duer_record_event_callback(int event, const void *data, size_t param)
{
    DUER_LOGD("record callback : %d", event);
    switch (event) {
    case REC_START:
    {
        DUER_LOGD("start send voice: %s", (const char *)data);
        duer_dcs_on_listen_started();
        duer_voice_start(param);
        break;
    }
    case REC_DATA:
        duer_voice_send(data, param);
        break;
    case REC_STOP:
        DUER_LOGD("stop send voice: %s", (const char *)data);
        duer_voice_stop();
        break;
    default:
        DUER_LOGE("event not supported: %d!", event);
        break;
    }
}

static void duer_dcs_init()
{
    duer_dcs_framework_init();
    duer_dcs_voice_input_init();
    duer_dcs_voice_output_init();
    duer_dcs_speaker_control_init();
    duer_dcs_audio_player_init();
    duer_dcs_screen_init();
    duer_dcs_sync_state();
}

void duer_txt2tts()
{
    duer_status_t result = DUER_OK;
    baidu_json *speech = NULL;
    baidu_json *log = NULL;
    baidu_json *data = NULL;
    speech = baidu_json_CreateObject();
    log = baidu_json_CreateObject();
    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_LOGE("data json create fail");
        return;
    }
    baidu_json_AddStringToObjectCS(data, "txt", "hello world");
    baidu_json_AddNumberToObjectCS(log, "id", 123);
    baidu_json_AddItemToObjectCS(log, "data", (baidu_json*)data);
    baidu_json_AddStringToObjectCS(log, "extra", "liulili");
    baidu_json_AddItemToObjectCS(speech, "duer_speech", (baidu_json*)log);
    result = duer_data_report(speech);
    if (result != DUER_OK) {
        DUER_LOGE("report speech fail: %d", result);
    }
    if (speech != NULL) {
        baidu_json_Delete(speech);
    }
}

static void duer_reconnect_thread()
{
    pthread_detach(pthread_self());
    if (s_pro_path) {
        if (s_reconnect_time >= 16) {
            s_reconnect_time = 1;
        }
        sleep(s_reconnect_time);
        s_reconnect_time *= 2;
        DUER_LOGI("Start reconnect baidu cloud!");
        duer_test_start(s_pro_path);
    } else {
        DUER_LOGI("No profile path, can not be reconnected");
    }
}

static void duer_event_hook(duer_event_t *event)
{
    if (!event) {
        DUER_LOGE("NULL event!!!");
        return;
    }

    DUER_LOGD("event: %d", event->_event);
    switch (event->_event) {
    case DUER_EVENT_STARTED:
        duer_dcs_init();
        //duer_dcs_capability_declare(DCS_TTS_HTTPS_PROTOCAL_SUPPORTED);
        s_reconnect_time = 1;
        s_started = true;
        break;
    case DUER_EVENT_STOPPED:
        s_started = false;
        if (s_pro_path) {
            pthread_t reconnect_threadid;
            int ret = pthread_create(&reconnect_threadid, NULL,
                          (void *)duer_reconnect_thread, NULL);
            if(ret != 0)
            {
                DUER_LOGE("Create reconnect pthread error!");
            } else {
                pthread_setname_np(reconnect_threadid, "reconnect");
            }
        }
        break;
    }
}

static void duer_voice_result(struct baidu_json *result)
{
    char* str_result = baidu_json_PrintUnformatted(result);
    DUER_LOGI("duer_voice_result:%s", str_result);
    DUER_FREE(str_result);

}

// It called in duerapp_wifi_config.c, when the Wi-Fi is connected.
void duer_test_start(const char* profile)
{
    const char *data = duer_load_profile(profile);
    if (data == NULL) {
        DUER_LOGE("load profile failed!");
        return;
    }

    DUER_LOGD("profile: \n%s", data);

    // We start the duer with duer_start(), when network connected.
    duer_start(data, strlen(data));

    free((void *)data);
}

int main(int argc, char* argv[])
{
    //initialize_sdcard();

    //initialize_gpio();
    //get NTP time
    //get_ntp_time();

    DUER_LOGI("before initialize");// will not print due to before initialize

    // Initialize the Duer OS
    duer_args_t duer_args = DEFAULT_ARGS;
    duer_args_parse(argc, argv, &duer_args, false);

    DUER_LOGI("profile_path : [%s]", duer_args.profile_path);
    DUER_LOGI("voice_record_path: [%s]", duer_args.voice_record_path);
    DUER_LOGI("interval_between_query: [%d]", duer_args.interval_between_query);

    duer_initialize();
    DUER_LOGI("after initialize");

    s_pro_path = duer_args.profile_path;

    // Set the Duer Event Callback
    duer_set_event_callback(duer_event_hook);

    duer_test_start(duer_args.profile_path);
    duer_record_set_event_callback(duer_record_event_callback);
    duer_dcs_screen_init();

    sleep(5); // wait for ca started
    while(1) {
        //initialize_wifi();
        DUER_LOGI("in main thread send record file");

        if (s_started) {
            duer_record_all(duer_args.voice_record_path, duer_args.interval_between_query);
            //duer_txt2tts();
#ifdef  LINUX_DEMO_TEST_ONLY_ONCE
            s_pro_path = NULL;
            sleep(2);
            duer_dcs_uninitialize();
            duer_stop();
            sleep(2);
            duer_finalize();
            sleep(3);

            duer_initialize();
            s_pro_path = duer_args.profile_path;

            // Set the Duer Event Callback
            duer_set_event_callback(duer_event_hook);

            duer_test_start(duer_args.profile_path);
            duer_record_set_event_callback(duer_record_event_callback);
            duer_dcs_screen_init();
            sleep(5);
            //break;
#endif
        } else {
            DUER_LOGI("not start yet!!");
        }
        sleep(duer_args.interval_between_query);
    }

    s_pro_path = NULL;

    return 0;
}

