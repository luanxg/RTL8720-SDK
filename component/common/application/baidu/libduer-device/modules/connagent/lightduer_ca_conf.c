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
// Author: Su Hao (suhao@baidu.com)
//
// Description: Baidu CA configuration.

#include "lightduer_ca_conf.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "baidu_json.h"
#include "lightduer_lib.h"

DUER_INT_IMPL duer_conf_handler duer_conf_create(const void* data, duer_size_t size) {
    DUER_LOGV("conf data: %s", data);
    return baidu_json_Parse((const char*)data);
}

DUER_INT_IMPL const char* duer_conf_get_string(duer_conf_handler hdlr,
        const char* key) {
    baidu_json* rs = baidu_json_GetObjectItem((baidu_json*)hdlr, key);

    if (rs) {
        if (DUER_MEMCMP(key, "uuid", DUER_STRLEN(key)) == 0
                || DUER_MEMCMP(key, "serverAddr", DUER_STRLEN(key)) == 0) {
            DUER_LOGI("    duer_conf_get_string: %s = %s", key, rs->valuestring);
        } else {
            DUER_LOGD("    duer_conf_get_string: %s = %s", key, rs->valuestring);
        }
        return rs->valuestring;
    }

    return NULL;
}

DUER_INT_IMPL duer_u16_t duer_conf_get_ushort(duer_conf_handler hdlr, const char* key) {
    baidu_json* rs = baidu_json_GetObjectItem((baidu_json*)hdlr, key);

    if (rs) {
        DUER_LOGD("    duer_conf_get_string: %s = %d", key, rs->valueint);
        return (duer_u16_t)(rs->valueint & 0xFFFF);
    }

    return 0;
}

DUER_INT_IMPL duer_u32_t duer_conf_get_uint(duer_conf_handler hdlr, const char* key) {
    baidu_json* rs = baidu_json_GetObjectItem((baidu_json*)hdlr, key);

    if (rs) {
        DUER_LOGD("    duer_conf_get_string: %s = %d", key, rs->valueint);
        return (duer_u32_t)rs->valueint;
    }

    return 0;
}

DUER_INT_IMPL void duer_conf_destroy(duer_conf_handler hdlr) {
    baidu_json_Delete((baidu_json*)hdlr);
}
