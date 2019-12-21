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
 * File: lightduer_dcs_voice_input_test.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Unit test for DCS voice input interface
 */

#include "test.h"

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"

static const int MAX_LISTEN_TIME = 20000;
static duer_timer_handler s_listen_timer;
static duer_mutex_t s_mutex;

void duer_speech_on_stop_internal()
{
}

int duer_dcs_on_listen_started_normal_test(void)
{
    baidu_json *data = 0x01;
    baidu_json *client_context = 0x02;
    baidu_json *event = 0x03;
    baidu_json *header = 0x04;
    baidu_json *dialog_id_item = 0x05;
    baidu_json *payload = 0x06;
    baidu_json *format = 0x06;
    int rs = DUER_OK;
    char buf[10];

    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_timer_start, handle, s_listen_timer);
    expect_value(duer_timer_start, delay, MAX_LISTEN_TIME);
    will_return(duer_timer_start, 0);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    will_return(baidu_json_CreateObject, data);
    will_return(duer_get_client_context_internal, client_context);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "clientContext");
    expect_value(baidu_json_AddItemToObject, item, client_context);

    expect_string(duer_create_dcs_event, namespace, "ai.dueros.device_interface.voice_input");
    expect_string(duer_create_dcs_event, name, "ListenStarted");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "header");
    will_return(baidu_json_GetObjectItem, header);

    expect_not_value(baidu_json_CreateString, string, buf);
    will_return(baidu_json_CreateString, dialog_id_item);
    expect_value(baidu_json_AddItemToObject, object, header);
    expect_string(baidu_json_AddItemToObject, string, "dialogRequestId");
    expect_value(baidu_json_AddItemToObject, item, dialog_id_item);

    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "payload");
    will_return(baidu_json_GetObjectItem, payload);

    expect_string(baidu_json_CreateString, string, "AUDIO_L16_RATE_16000_CHANNELS_1");
    will_return(baidu_json_CreateString, format);
    expect_value(baidu_json_AddItemToObject, object, payload);
    expect_string(baidu_json_AddItemToObject, string, "format");
    expect_value(baidu_json_AddItemToObject, item, format);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_on_listen_started();
    assert_int_equal(rs, DUER_OK);
}

int duer_dcs_on_listen_started_abnormal_test1(void)
{
    baidu_json *data = NULL;
    int rs = DUER_OK;

    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    will_return(baidu_json_CreateObject, data);

    rs = duer_dcs_on_listen_started();
    assert_int_equal(rs, DUER_ERR_FAILED);
}

int duer_dcs_on_listen_started_abnormal_test2(void)
{
    baidu_json *data = 0x01;
    baidu_json *client_context = 0x02;
    baidu_json *event = NULL;
    int rs = DUER_OK;

    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    will_return(baidu_json_CreateObject, data);
    will_return(duer_get_client_context_internal, client_context);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "clientContext");
    expect_value(baidu_json_AddItemToObject, item, client_context);

    expect_string(duer_create_dcs_event, namespace, "ai.dueros.device_interface.voice_input");
    expect_string(duer_create_dcs_event, name, "ListenStarted");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_on_listen_started();
    assert_int_equal(rs, DUER_ERR_FAILED);
}

CMOCKA_UNIT_TEST(duer_dcs_on_listen_started_normal_test);
CMOCKA_UNIT_TEST(duer_dcs_on_listen_started_abnormal_test1);
CMOCKA_UNIT_TEST(duer_dcs_on_listen_started_abnormal_test2);
