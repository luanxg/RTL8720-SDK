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
 * File: lightduer_dcs_audio_player_test.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Unit test for DCS audio player interface
 */

#include "test.h"
#include "lightduer_dcs.h"
#include <stdbool.h>
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ca.h"
#include "lightduer_queue_cache.h"
#include "lightduer_mutex.h"
#include "lightduer_connagent.h"

bool duer_speech_need_play_internal(void)
{
    return mock_type(bool);
}

void duer_dcs_report_play_event_normal_test()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, "ai.dueros.device_interface.audio_player");
    expect_string(duer_create_dcs_event, name, "PlaybackQueueCleared");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_report_play_event(DCS_PLAY_QUEUE_CLEARED);
    assert_int_equal(rs, DUER_OK);
}

void duer_dcs_report_play_event_abnormal_test1()
{
    int rs = DUER_OK;

    rs = duer_dcs_report_play_event(-1);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_dcs_report_play_event_abnormal_test2()
{
    int rs = DUER_OK;

    rs = duer_dcs_report_play_event(DCS_PLAY_FINISHED);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_dcs_report_play_event_abnormal_test3()
{
    baidu_json *data = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);
    rs = duer_dcs_report_play_event(DCS_PLAY_QUEUE_CLEARED);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_dcs_report_play_event_abnormal_test4()
{
    baidu_json *data = 0x01;
    baidu_json *event = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, "ai.dueros.device_interface.audio_player");
    expect_string(duer_create_dcs_event, name, "PlaybackQueueCleared");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_report_play_event(DCS_PLAY_QUEUE_CLEARED);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

CMOCKA_UNIT_TEST(duer_dcs_report_play_event_normal_test);
CMOCKA_UNIT_TEST(duer_dcs_report_play_event_abnormal_test1);
CMOCKA_UNIT_TEST(duer_dcs_report_play_event_abnormal_test2);
CMOCKA_UNIT_TEST(duer_dcs_report_play_event_abnormal_test3);
CMOCKA_UNIT_TEST(duer_dcs_report_play_event_abnormal_test4);
