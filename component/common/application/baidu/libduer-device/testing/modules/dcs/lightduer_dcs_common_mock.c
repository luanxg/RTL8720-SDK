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
 * File: lightduer_dcs_local_mock.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Provide mock functions for dcs module local.
 */

#include "test.h"

#include "lightduer_dcs_local.h"
#include "lightduer_dcs_router.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"

static volatile int s_dialog_req_id = 0;

__attribute__((weak)) void duer_user_activity_internal(void)
{
}

int duer_get_request_id_internal()
{
    return s_dialog_req_id++;
}

__attribute__((weak)) void duer_pause_audio_internal()
{
}

char *duer_strdup_internal(const char *str)
{
    check_expected(str);
    return (char *)mock();
}

baidu_json* duer_get_client_context_internal(void)
{
    return (baidu_json *)mock();
}

baidu_json *duer_create_dcs_event(const char *namespace, const char *name, const char *msg_id)
{
    check_expected(namespace);
    check_expected(name);
    check_expected(msg_id);

    return (baidu_json *)mock();
}

int duer_data_report(const baidu_json *data)
{
    check_expected(data);
    return mock_type(int);
}

void duer_add_dcs_directive(const duer_directive_list *directive, size_t count)
{
    check_expected(count);
}
