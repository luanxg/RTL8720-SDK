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
 * File: lightduer_dcs_speaker_control_test.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Unit test for DCS speaker control interface
 */
#include "test.h"
#include "lightduer_dcs.h"
#include "stdbool.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"

static const char *SPEAKER_CONTROL_NAMESPACE = "ai.dueros.device_interface.speaker_controller";

void duer_report_speaker_event_normal_test()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    baidu_json *payload = 0x03;
    baidu_json *volume_item = 0x04;
    baidu_json *mute_item = 0x05;
    int volume = 0;
    bool is_mute = false;
    int rs = DUER_OK;

    duer_dcs_speaker_control_init();

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, SPEAKER_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "VolumeChanged");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "payload");
    will_return(baidu_json_GetObjectItem, payload);

    expect_value(baidu_json_CreateNumber, num, volume);
    will_return(baidu_json_CreateNumber, volume_item);
    expect_value(baidu_json_AddItemToObject, object, payload);
    expect_string(baidu_json_AddItemToObject, string, "volume");
    expect_value(baidu_json_AddItemToObject, item, volume_item);

    expect_value(baidu_json_CreateBool, b, is_mute);
    will_return(baidu_json_CreateBool, mute_item);
    expect_value(baidu_json_AddItemToObject, object, payload);
    expect_string(baidu_json_AddItemToObject, string, "mute");
    expect_value(baidu_json_AddItemToObject, item, mute_item);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_on_volume_changed();
    assert_int_equal(rs, DUER_OK);
}

void duer_report_speaker_event_abnormal_test1()
{
    baidu_json *data = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    rs = duer_dcs_on_mute();
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_report_speaker_event_abnormal_test2()
{
    baidu_json *data = 0x01;
    baidu_json *event = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, SPEAKER_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "MuteChanged");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_on_mute();
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_report_speaker_event_abnormal_test3()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, SPEAKER_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "MuteChanged");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "payload");
    will_return(baidu_json_GetObjectItem, payload);
    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_on_mute();
    assert_int_equal(rs, DUER_ERR_FAILED);
}

CMOCKA_UNIT_TEST(duer_report_speaker_event_normal_test);
CMOCKA_UNIT_TEST(duer_report_speaker_event_abnormal_test1);
CMOCKA_UNIT_TEST(duer_report_speaker_event_abnormal_test2);
CMOCKA_UNIT_TEST(duer_report_speaker_event_abnormal_test3);
