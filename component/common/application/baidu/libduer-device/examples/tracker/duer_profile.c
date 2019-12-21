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
// Author: suhao@baidu.com(suhao@baidu.com)
//
// Save and provide the profile data.

#include <stdio.h>
#include <stdlib.h>

#include "duer_log.h"
#include "duer_profile.h"

typedef struct _duer_profile_s {
    void *  _data;
    size_t  _size;
} duer_profile_t;

duer_prof_handler duer_profile_create(const char *path) {
    duer_profile_t *profile = NULL;
    FILE *file = NULL;
    int rs = -1;

    do {
        profile = (duer_profile_t *)malloc(sizeof(duer_profile_t));
        if (profile == NULL) {
            LOGE("Create profile failed!");
            break;
        }

        file = fopen(path, "rb");
        if (file == NULL) {
            LOGE("Failed to open file: %s", path);
            break;
        }

        rs = fseek(file, 0, SEEK_END);
        if (rs != 0) {
            LOGE("Seek to file tail failed, rs = %d", rs);
            break;
        }

        profile->_size = ftell(file);
        if (profile->_size < 0) {
            LOGE("Obtain file size failed, rs = %lu", profile->_size);
            rs = -1;
            break;
        }

        rs = fseek(file, 0, SEEK_SET);
        if (rs != 0) {
            LOGE("Seek to file head failed, rs = %d", rs);
            break;
        }

        profile->_data = (char *)malloc(profile->_size + 1);
        if (profile->_data == NULL) {
            LOGE("Alloc the profile profile->_data failed with size = %ld", profile->_size);
            rs = -1;
            break;
        }

        rs = fread(profile->_data, 1, profile->_size, file);
        if (rs < 0) {
            LOGE("Read file failed, rs = %d", rs);
            break;
        }

        ((char *)(profile->_data))[profile->_size] = '\0';
        rs = 0;
    } while (0);

    if (file) {
        fclose(file);
    }

    if (rs != 0 && profile) {
        duer_profile_destroy(profile);
        profile = NULL;
    }

    return profile;
}

const void *duer_profile_get_data(duer_prof_handler prof) {
    duer_profile_t *profile = (duer_profile_t *)prof;
    return profile != NULL ? profile->_data : NULL;
}

size_t duer_profile_get_size(duer_prof_handler prof) {
    duer_profile_t *profile = (duer_profile_t *)prof;
    return profile != NULL ? profile->_size : 0;
}

void duer_profile_destroy(duer_prof_handler prof) {
    duer_profile_t *profile = (duer_profile_t *)prof;

    if (profile) {
        if (profile->_data) {
            free(profile->_data);
            profile->_data = NULL;
        }

        free(profile);
    }
}
