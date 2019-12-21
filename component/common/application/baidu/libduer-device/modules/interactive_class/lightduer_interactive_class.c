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
 * File: lightduer_interactive_class.c
 * Auth: Chen Xihao (chenxihao@baidu.com)
 * Desc: Interactive class procedure.
 */

#include "lightduer_interactive_class.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lightduer_connagent.h"
#include "lightduer_dcs.h"
#include "lightduer_dcs_local.h"
#include "lightduer_dcs_router.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_voice.h"

static const char LC_NAMESPACE[] = "ai.dueros.device_interface.extensions.light_controller";
static const char OPEN_URL[] = "dueros://interactive_class.bot.dueros.ai/control?action=open";
static const char CLOSE_URL[] = "dueros://interactive_class.bot.dueros.ai/control?action=close";
static duer_interactive_class_handler_t s_interactive_class_handler;
static volatile bool s_is_living = false;

static duer_status_t duer_control_light(const baidu_json *directive)
{
    int ret = DUER_OK;
    baidu_json *payload = NULL;
    baidu_json *light_name = NULL;
    baidu_json *action = NULL;
    bool on = false;
    DUER_LOGI("duer_control_light");
    do {
        payload = baidu_json_GetObjectItem(directive, "payload");
        if (!payload) {
            DUER_LOGE("Failed to get payload");
            ret = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        light_name = baidu_json_GetObjectItem(payload, "lightName");
        if (!light_name || !light_name->valuestring) {
            DUER_LOGE("Failed to get lightName");
            ret = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        action = baidu_json_GetObjectItem(payload, "action");
        if (!action || !action->valuestring) {
            DUER_LOGE("Failed to get action");
            ret = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (DUER_STRCMP("INTERACTIVE_COURSE", light_name->valuestring) != 0) {
            DUER_LOGE("lightName is not INTERACTIVE_COURSE:%s", light_name->valuestring);
            ret = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (DUER_STRCMP("TURN_ON", action->valuestring) != 0
                && DUER_STRCMP("TURN_OFF", action->valuestring) != 0) {
            DUER_LOGE("action is not TURN_ON or TURN_OFF:%s", action->valuestring);
            ret = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (DUER_STRCMP("TURN_ON", action->valuestring) == 0) {
            on = true;
        }

        if (!s_is_living && on) {
            DUER_LOGW("interactive_class is not living");
            break;
        }

        if (s_interactive_class_handler.handle_control_light != NULL) {
            s_interactive_class_handler.handle_control_light(on);
        }

        // interactive class is closed by cloud
        if (s_is_living && !on) {
            s_is_living = false;
            duer_voice_set_mode(DUER_VOICE_MODE_DEFAULT);
        }
    } while (0);

    return ret;
}

static duer_status_t duer_interactive_notice(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    duer_status_t rs = DUER_OK;
    baidu_json *value = NULL;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;

    do {
        if (!msg) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (!msg->payload || msg->payload_len <= 0) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        DUER_LOGV("duer_interactive_notice payload: %s", msg->payload);

        value = baidu_json_Parse(msg->payload);
        if (value == NULL) {
            DUER_LOGE("Failed to parse payload");
            rs = DUER_ERR_FAILED;
            break;
        }

        if (s_interactive_class_handler.handle_notice != NULL) {
            rs = s_interactive_class_handler.handle_notice(value);
        }
    } while (0);

    if (rs != DUER_OK) {
        msg_code = DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_response(msg, msg_code, NULL, 0);

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

void duer_interactive_class_init(void)
{
    static bool is_first_time = true;

    duer_res_t res[] = {
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT, "duer_interactive_notice",
                duer_interactive_notice},
    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));

    if (!is_first_time) {
        return;
    }

    is_first_time = false;

    duer_directive_list directive[] = {
        {"Control", duer_control_light},
    };

    duer_add_dcs_directive(directive, sizeof(directive) / sizeof(directive[0]), LC_NAMESPACE);
}

duer_status_t duer_interactive_class_open(void)
{
    duer_status_t ret = DUER_OK;
    if (!s_is_living) {
        s_is_living = true;
        duer_voice_set_mode(DUER_VOICE_MODE_INTERACTIVE_CLASS);
        ret = duer_dcs_on_link_clicked(OPEN_URL);
    }

    return ret;
}

duer_status_t duer_interactive_class_close(void)
{
    duer_status_t ret = DUER_OK;
    if (s_is_living) {
        ret = duer_dcs_on_link_clicked(CLOSE_URL);
    }

    return ret;
}

bool duer_interactive_class_is_living(void)
{
    return s_is_living;
}

void duer_interactive_class_set_handler(duer_interactive_class_handler_t *handler)
{
    if (handler == NULL) {
        return;
    }

    s_interactive_class_handler.handle_control_light = handler->handle_control_light;
    s_interactive_class_handler.handle_notice = handler->handle_notice;
}
