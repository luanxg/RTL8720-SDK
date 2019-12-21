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
 * File: lightduer_dcs_device_contorl.c.c
 * Auth: Jiangfeng Huang (huangjianfeng02@baidu.com)
 * Desc: apply APIs to support DCS device control interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static const char *DEVICE_CONTROL_NAMESPACE =
        "ai.dueros.device_interface.extensions.device_control";
static volatile duer_bool s_is_initialized;

static duer_status_t duer_bluetooth_set_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *bt_setting = NULL;
    baidu_json *target = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    bt_setting = baidu_json_GetObjectItem(payload, DCS_BLUETOOTH_KEY);
    if (!bt_setting) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    target = baidu_json_GetObjectItem(payload, DCS_TARGET_KEY);
    if (!target) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_bluetooth_set_handler(bt_setting->type == baidu_json_True, target->valuestring);

    return DUER_OK;
}

static duer_status_t duer_bluetooth_connection_set_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *bt_conn = NULL;
    baidu_json *target = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    bt_conn = baidu_json_GetObjectItem(payload, DCS_CONNECTION_SWITCH_KEY);
    if (!bt_conn) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    target = baidu_json_GetObjectItem(payload, DCS_TARGET_KEY);
    if (!target) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_bluetooth_connect_handler(bt_conn->type == baidu_json_True, target->valuestring);

    return DUER_OK;
}

void duer_dcs_device_control_init(void)
{
    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_directive_list res[] = {
        {DCS_SET_BLUETOOTH_NAME, duer_bluetooth_set_cb},
        {DCS_SET_BLUETOOTH_CONNECTION_NAME, duer_bluetooth_connection_set_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), DEVICE_CONTROL_NAMESPACE);
    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_device_control_uninitialize_internal(void)
{
    s_is_initialized = DUER_FALSE;    
}

