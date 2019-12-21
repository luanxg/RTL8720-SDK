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
 * File: duerapp_profile_config.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Read baidu profile.
 */

#include <stdio.h>

#include "duerapp_config.h"

const char *duer_load_profile(const char *path)
{
    char *data = NULL;

    FILE *file = NULL;

    do {
        int rs;

        file = fopen(path, "rb");
        if (file == NULL) {
            DUER_LOGE("Failed to open file: %s", path);
            break;
        }

        rs = fseek(file, 0, SEEK_END);
        if (rs != 0) {
            DUER_LOGE("Seek to file tail failed, rs = %d", rs);
            break;
        }

        long size = ftell(file);
        if (size < 0) {
            DUER_LOGE("Seek to file tail failed, rs = %d", rs);
            break;
        }

        rs = fseek(file, 0, SEEK_SET);
        if (rs != 0) {
            DUER_LOGE("Seek to file head failed, rs = %d", rs);
            break;
        }

        data = (char *)malloc(size + 1);
        if (data == NULL) {
            DUER_LOGE("Alloc the profile data failed with size = %ld", size);
            break;
        }

        rs = fread(data, 1, size, file);
        if (rs < 0) {
            DUER_LOGE("Read file failed, rs = %d", rs);
            free(data);
            data = NULL;
            break;
        }

        data[size] = '\0';
    } while (0);

    if (file) {
        fclose(file);
        file = NULL;
    }

    return data;
}
