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
 * File: lightduer_dcs_playback_control_test.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Unit test for DCS playback control interface
 */
#include "test.h"
#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"

static const char *PLAYBACK_CONTROL_NAMESPACE = "ai.dueros.device_interface.playback_controller";

void duer_dcs_send_play_control_cmd_normal_test()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    baidu_json *client_context = 0x03;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    will_return(duer_get_client_context_internal, client_context);
    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "clientContext");
    expect_value(baidu_json_AddItemToObject, item, client_context);

    expect_string(duer_create_dcs_event, namespace, PLAYBACK_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "PlayCommandIssued");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
    assert_int_equal(rs, DUER_OK);
}

void duer_dcs_send_play_control_cmd_abnormal_test1()
{
    int rs = DUER_OK;

    rs = duer_dcs_send_play_control_cmd(-1);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_dcs_send_play_control_cmd_abnormal_test2()
{
    baidu_json *data = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    rs = duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
    assert_int_equal(rs, DUER_ERR_MEMORY_OVERLOW);
}

void duer_dcs_send_play_control_cmd_abnormal_test3()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    baidu_json *client_context = NULL;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    will_return(duer_get_client_context_internal, client_context);

    expect_string(duer_create_dcs_event, namespace, PLAYBACK_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "PauseCommandIssued");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
    assert_int_equal(rs, DUER_OK);
}

void duer_dcs_send_play_control_cmd_abnormal_test4()
{
    baidu_json *data = 0x01;
    baidu_json *event = NULL;
    baidu_json *client_context = 0x03;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    will_return(duer_get_client_context_internal, client_context);
    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "clientContext");
    expect_value(baidu_json_AddItemToObject, item, client_context);

    expect_string(duer_create_dcs_event, namespace, PLAYBACK_CONTROL_NAMESPACE);
    expect_string(duer_create_dcs_event, name, "NextCommandIssued");
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

CMOCKA_UNIT_TEST(duer_dcs_send_play_control_cmd_normal_test);
CMOCKA_UNIT_TEST(duer_dcs_send_play_control_cmd_abnormal_test1);
CMOCKA_UNIT_TEST(duer_dcs_send_play_control_cmd_abnormal_test2);
CMOCKA_UNIT_TEST(duer_dcs_send_play_control_cmd_abnormal_test3);
CMOCKA_UNIT_TEST(duer_dcs_send_play_control_cmd_abnormal_test4);
