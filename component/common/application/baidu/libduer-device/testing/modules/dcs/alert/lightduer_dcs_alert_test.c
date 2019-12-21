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
 * File: lightduer_dcs_alert_test.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Unit test for dcs alert interface.
 */
#include "test.h"

#include "lightduer_dcs_alert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lightduer_dcs.h"
#include "lightduer_dcs_local.h"
#include "lightduer_dcs_router.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"

static const char* const s_event_name_tab[] = {"SetAlertSucceeded",
                                               "SetAlertFailed",
                                               "DeleteAlertSucceeded",
                                               "DeleteAlertFailed",
                                               "AlertStarted",
                                               "AlertStopped"};

int duer_dcs_report_alert_event_normal_test()
{
    baidu_json *data = 0x01;
    baidu_json *event = 0x02;
    baidu_json *payload = 0x03;
    baidu_json *token_item = 0x4;
    char *token = "xxxxxxxxxxx";
    duer_dcs_alert_event_type type = SET_ALERT_SUCCESS;
    int rs = DUER_OK;

    will_return(baidu_json_CreateObject, data);

    expect_string(duer_create_dcs_event, namespace, "ai.dueros.device_interface.alerts");
    expect_string(duer_create_dcs_event, name, s_event_name_tab[type]);
    expect_value(duer_create_dcs_event, msg_id, NULL);
    will_return(duer_create_dcs_event, event);

    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);

    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "payload");
    will_return(baidu_json_GetObjectItem, payload);

    expect_string(baidu_json_CreateString, string, token);
    will_return(baidu_json_CreateString, token_item);

    expect_value(baidu_json_AddItemToObject, object, payload);
    expect_string(baidu_json_AddItemToObject, string, "token");
    expect_value(baidu_json_AddItemToObject, item, token_item);

    expect_value(duer_data_report, data, data);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, data);

    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_OK);
}

int duer_dcs_report_alert_event_abnormal_test()
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    baidu_json *token_item = NULL;
    char *token = NULL;
    duer_dcs_alert_event_type type = SET_ALERT_SUCCESS;
    int rs = DUER_OK;

    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_ERR_FAILED);

    token = "xxxxxxxxxxx";
    type = -1;
    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_ERR_FAILED);

    type = SET_ALERT_FAIL;
    will_return(baidu_json_CreateObject, data);
    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_ERR_FAILED);

    data = 0x01;
    will_return_count(baidu_json_CreateObject, data, 2);
    expect_string_count(duer_create_dcs_event, namespace, "ai.dueros.device_interface.alerts", 2);
    expect_string_count(duer_create_dcs_event, name, s_event_name_tab[type], 2);
    expect_value_count(duer_create_dcs_event, msg_id, NULL, 2);
    will_return(duer_create_dcs_event, event);
    expect_value_count(baidu_json_Delete, c, data, 2);
    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_ERR_FAILED);

    event = 0x2;
    will_return(duer_create_dcs_event, event);
    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "event");
    expect_value(baidu_json_AddItemToObject, item, event);
    expect_value(baidu_json_GetObjectItem, object, event);
    expect_string(baidu_json_GetObjectItem, string, "payload");
    will_return(baidu_json_GetObjectItem, payload);
    rs = duer_dcs_report_alert_event(token, type);
    assert_int_equal(rs, DUER_ERR_FAILED);
}

void duer_dcs_report_alert_test()
{
    baidu_json *alert_array = NULL;
    baidu_json *alert = NULL;
    baidu_json *token_item = 0x01;
    baidu_json *type_item = 0x02;
    baidu_json *time_item = 0x03;
    char *token = NULL;
    char *type = NULL;
    char *time = NULL;

    duer_dcs_report_alert(alert_array, token, type, time);

    alert_array = 0x04;
    token = "token";
    type = "type";
    time = "time";
    will_return(baidu_json_CreateObject, alert);
    duer_dcs_report_alert(alert_array, token, type, time);

    alert = 0x05;
    will_return(baidu_json_CreateObject, alert);
    expect_string(baidu_json_CreateString, string, token);
    will_return(baidu_json_CreateString, token_item);
    expect_value(baidu_json_AddItemToObject, object, alert);
    expect_string(baidu_json_AddItemToObject, string, "token");
    expect_value(baidu_json_AddItemToObject, item, token_item);

    expect_string(baidu_json_CreateString, string, type);
    will_return(baidu_json_CreateString, type_item);
    expect_value(baidu_json_AddItemToObject, object, alert);
    expect_string(baidu_json_AddItemToObject, string, "type");
    expect_value(baidu_json_AddItemToObject, item, type_item);

    expect_string(baidu_json_CreateString, string, time);
    will_return(baidu_json_CreateString, time_item);
    expect_value(baidu_json_AddItemToObject, object, alert);
    expect_string(baidu_json_AddItemToObject, string, "scheduledTime");
    expect_value(baidu_json_AddItemToObject, item, time_item);

    expect_value(baidu_json_AddItemToArray, array, alert_array);
    expect_value(baidu_json_AddItemToArray, item, alert);
    duer_dcs_report_alert(alert_array, token, type, time);
}

static duer_status_t duer_alert_set_cb(const baidu_json *directive)
{
    check_expected(directive);
    return mock_type(duer_status_t);
}

static duer_status_t duer_alert_delete_cb(const baidu_json *directive)
{
    check_expected(directive);
    return mock_type(duer_status_t);
}

void duer_dcs_alert_init_test()
{
    duer_directive_list res[] = {
        {"SetAlert", duer_alert_set_cb},
        {"DeleteAlert", duer_alert_delete_cb},
    };

    expect_value(duer_add_dcs_directive, count, sizeof(res) / sizeof(res[0]));

    duer_dcs_alert_init();
    duer_dcs_alert_init();
}

CMOCKA_UNIT_TEST(duer_dcs_alert_init_test);
CMOCKA_UNIT_TEST(duer_dcs_report_alert_event_normal_test);
CMOCKA_UNIT_TEST(duer_dcs_report_alert_event_abnormal_test);
CMOCKA_UNIT_TEST(duer_dcs_report_alert_test);

