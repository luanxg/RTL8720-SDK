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
 * File: lightduer_dcs_screen.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS screen interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static volatile duer_bool s_is_initialized;

static duer_status_t duer_render_input_text_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *text = NULL;
    baidu_json *type = NULL;
    duer_status_t ret = DUER_OK;

    DUER_LOGV("Entry");

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    text = baidu_json_GetObjectItem(payload, DCS_TEXT_KEY);
    type = baidu_json_GetObjectItem(payload, DCS_TYPE_KEY);
    if (!text || !type) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    ret = duer_dcs_input_text_handler(text->valuestring, type->valuestring);

    return ret;
}

static duer_status_t duer_render_card_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    duer_status_t ret = DUER_OK;

    DUER_LOGV("Entry");

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    ret = duer_dcs_render_card_handler(payload);

    return ret;
}

int duer_dcs_on_link_clicked(const char *url)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();

    do {
        if (!s_is_initialized) {
            DUER_LOGW("DCS module has not been initialized");
            rs = DUER_ERR_FAILED;
            break;
        }

        if (!url) {
            rs = DUER_ERR_INVALID_PARAMETER;
            DUER_LOGE("Invalid param: url is null");
            break;
        }

        data = baidu_json_CreateObject();
        if (data == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            break;
        }

        event = duer_create_dcs_event(DCS_SCREEN_NAMESPACE,
                                      DCS_LINK_CLICKED_NAME,
                                      DUER_TRUE);
        if (event == NULL) {
            rs = DUER_ERR_FAILED;
            break;
        }

        baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);

        payload = baidu_json_GetObjectItem(event, DCS_PAYLOAD_KEY);
        if (!payload) {
            rs = DUER_ERR_FAILED;
            break;
        }
        baidu_json_AddStringToObjectCS(payload, DCS_URL_KEY, url);

        rs = duer_dcs_data_report_internal(data, DUER_FALSE);
    } while (0);

    DUER_DCS_CRITICAL_EXIT();

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

void duer_dcs_screen_init(void)
{
    DUER_DCS_CRITICAL_ENTER();

    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_directive_list res[] = {
        {DCS_RENDER_VOICE_INPUT_TEXT_NAME, duer_render_input_text_cb},
        {DCS_RENDER_CARD_NAME, duer_render_card_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), DCS_SCREEN_NAMESPACE);
    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_screen_uninitialize_internal(void)
{
    s_is_initialized = DUER_FALSE;
}

