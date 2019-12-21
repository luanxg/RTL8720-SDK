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
/*
 * File: lightduer_ota_unpacker.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA Unpacker
 */

#include "lightduer_ota_unpacker.h"
#include <stdint.h>
#include <stdbool.h>
#include "zlib.h"
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"

int duer_ota_unpacker_set_unpacker_mode(
        duer_ota_unpacker_t *unpacker,
        duer_ota_unpacker_mode_t mode)
{
    if (unpacker == NULL || mode >= UNPACKER_MODE_MAX || mode < 0) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_FAILED;
    }

    // Internal function do not lock
    unpacker->state.mode = mode;

    return DUER_OK;
}

duer_ota_unpacker_mode_t duer_ota_unpacker_get_unpacker_mode(duer_ota_unpacker_t const *unpacker)
{
    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return UNPACKER_MODE_MAX;
    }

    // Internal function do not lock
    return unpacker->state.mode;
}

static void duer_ota_unpacker_show_package_header(duer_ota_package_header_t const *header)
{
    int i = 0;
    char signature[KEY_LEN + 1];

    if (header == NULL) {
        DUER_LOGE("Argument error");

        return;
    }

    DUER_MEMSET(signature, 0, KEY_LEN + 1);

    DUER_LOGI("Package Header Information");

    DUER_LOGI("Tag: %c %c %c %c",
            header->tag[0],
            header->tag[1],
            header->tag[2],
            header->tag[3]);

    DUER_LOGI("package header size: %d", header->package_header_size);

    DUER_LOGI("package signature size: %d", header->package_signature_size);

    DUER_STRNCPY(signature, header->package_signature, KEY_LEN);

    DUER_LOGI("package signature: %s", signature);

    DUER_LOGI("meta signature size: %d", header->meta_signature_size);

    DUER_STRNCPY(signature, header->meta_signature, KEY_LEN);

    DUER_LOGI("meta signature: %s", signature);

    DUER_LOGI("meta data size: %d", header->meta_size);

    DUER_LOGI("original package size: %d", header->ori_package_size);
}

static void duer_ota_unpacker_show_package_basic_info(duer_ota_package_basic_info_t const *info)
{
    if (info == NULL) {
        DUER_LOGE("Argument error");

        return;
    }

    DUER_LOGI("Package Basic Information:");

    DUER_LOGI("Package name: %s", info->package_name);
}

static void duer_ota_unpacker_show_package_install_info(
        duer_ota_package_install_info_t const *info)
{
    int i = 0;

    if (info == NULL) {
        DUER_LOGE("Argument error");

        return;
    }

    DUER_LOGI("Package Install Information:");

    DUER_LOGI("Module count: %d", info->module_count);

    for (i = 0; i < info->module_count; i++) {
        DUER_LOGI("Module[%d] name: %s", i, info->module_list[i].module_name);
        DUER_LOGI("Module[%d] size: %d", i, info->module_list[i].module_size);
        DUER_LOGI("Module[%d] module signature: %s", i, info->module_list[i].module_signature);
        DUER_LOGI("Module[%d] module version: %s", i, info->module_list[i].module_version);
        DUER_LOGI("Module[%d] module HW version: %s",
                i,
                info->module_list[i].module_support_hardware_version);
        // DUER_LOGI("Module[%d] module install path: %s", i,
        // info->module_list[i].module_install_path);
    }
}

static int duer_ota_unpacker_save_package_header(
        duer_ota_unpacker_t *unpacker,
        uint8_t const *data,
        size_t size)
{
    size_t package_header_size = sizeof(duer_ota_package_header_t);
    size_t remaining_size = 0;

    if (unpacker == NULL || data == NULL || size <= 0) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (unpacker->state.package_header_data_offset + size < package_header_size) {
        DUER_MEMMOVE(
                (char *)&unpacker->package_header + unpacker->state.package_header_data_offset,
                data,
                size);

        unpacker->state.package_header_data_offset += size;
        unpacker->state.processed_data_size += size;
        unpacker->state.used_data_offset += size;
    } else {
        remaining_size = package_header_size - unpacker->state.package_header_data_offset;

        DUER_MEMMOVE(
                (char *)&unpacker->package_header + unpacker->state.package_header_data_offset,
                data,
                remaining_size);

        unpacker->state.package_header_data_offset += remaining_size;
        unpacker->state.processed_data_size += remaining_size;
        unpacker->state.used_data_offset += remaining_size;

        duer_ota_unpacker_set_unpacker_mode(unpacker, PARSE_PACKAGE_HEADER);
    }

    return DUER_OK;
}

static int duer_ota_unpacker_get_int_val(
        char const *lable,
        baidu_json const *json,
        int *buf)
{
    baidu_json *json_obj = NULL;

    if (lable == NULL || json == NULL || buf == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    json_obj = baidu_json_GetObjectItem(json, lable);
    if (json_obj == NULL) {
        DUER_LOGE("OTA Unpacker: Get %s json obj failed", lable);

        return DUER_ERR_FAILED;
    }

    *buf = json_obj->valueint;

    return DUER_OK;
}

static int duer_ota_unpacker_get_string_val(
        char const *lable,
        baidu_json const *json,
        size_t buf_len,
        char *buf)
{
    size_t str_len = 0;
    baidu_json *json_obj = NULL;

    if (lable == NULL || json == NULL || buf == NULL || buf_len <= 0) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    json_obj = baidu_json_GetObjectItem(json, lable);
    if (json_obj == NULL) {
        DUER_LOGE("OTA Unpacker: Get %s json obj failed", lable);

        return DUER_ERR_FAILED;
    }

    str_len = DUER_STRLEN(json_obj->valuestring);
    if (str_len > buf_len) {

        DUER_LOGE("OTA Unpacker: The length of the string is too long");

        return DUER_ERR_FAILED;
    }

    DUER_STRNCPY(buf, json_obj->valuestring, str_len);

    return DUER_OK;
}

static int duer_ota_unpacker_save_meta_data(
        duer_ota_unpacker_t *unpacker,
        uint8_t const *data,
        size_t size)
{
    size_t remaining_size = 0;

    if (unpacker == NULL || data == NULL || size <= 0) {
        DUER_LOGE("OTA Unpacker: Argument Error");

        return DUER_ERR_FAILED;
    }

    if (unpacker->meta_data == NULL) {
        unpacker->meta_data = DUER_MALLOC(unpacker->package_header.meta_size + 1);
        if (unpacker->meta_data == NULL) {
            DUER_LOGE("OTA Unpacker: Malloc meta data buffer failed");

            return DUER_ERR_MEMORY_OVERLOW;
        } else {
            DUER_MEMSET(unpacker->meta_data, 0, unpacker->package_header.meta_size + 1);
        }
    }

    if (unpacker->state.meta_data_offset + size < unpacker->package_header.meta_size) {
        DUER_MEMMOVE((char *)unpacker->meta_data + unpacker->state.meta_data_offset, data, size);
        unpacker->state.meta_data_offset += size;
        // Data size after unzip
        unpacker->state.processed_data_size += size;
        unpacker->state.used_decompress_data_offset += size;
    } else {
        remaining_size = unpacker->package_header.meta_size - unpacker->state.meta_data_offset;

        DUER_MEMMOVE(
                unpacker->meta_data + unpacker->state.meta_data_offset,
                data,
                remaining_size);

        unpacker->state.meta_data_offset += remaining_size;
        unpacker->state.processed_data_size += remaining_size;
        unpacker->state.used_decompress_data_offset += remaining_size;

        duer_ota_unpacker_set_unpacker_mode(unpacker, PARSE_META_DATA);
    }

    return DUER_OK;
}

static int duer_ota_unpacker_parse_install_info(
        duer_ota_unpacker_t *unpacker,
        baidu_json const *meta_json)
{
    int i = 0;
    int ret = 0;
    size_t module_count = 0;
    baidu_json *install_json = NULL;
    baidu_json *modules_json = NULL;
    baidu_json *module_json = NULL;

    if (unpacker == NULL || meta_json == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    install_json = baidu_json_GetObjectItem(meta_json, "inst_and_uninst_info");
    if (install_json == NULL) {
        DUER_LOGE("OTA Unpacker: Get install json failed");

        return DUER_ERR_FAILED;
    }

    modules_json = baidu_json_GetObjectItem(install_json, "module_list");
    if (modules_json == NULL) {
        DUER_LOGE("OTA Unpacker: Get modules json failed");

        return DUER_ERR_FAILED;
    }

    module_count = baidu_json_GetArraySize(modules_json);
    if (module_count > 0) {
        unpacker->install_info.module_count = module_count;
        unpacker->install_info.module_list =
            (duer_ota_module_info_t *)DUER_MALLOC(module_count * sizeof(duer_ota_module_info_t));
        if (unpacker->install_info.module_list == NULL) {
            DUER_LOGE("OTA Unpacker: Malloc module list failed");

            return DUER_ERR_MEMORY_OVERLOW;
        } else {
            DUER_MEMSET(
                    unpacker->install_info.module_list,
                    0,
                    module_count * sizeof(duer_ota_module_info_t));
        }

        for (i = 0; i < module_count; i++) {
            module_json = baidu_json_GetArrayItem(modules_json, i);
            if (modules_json != NULL) {
                ret = duer_ota_unpacker_get_string_val(
                        "name",
                        module_json,
                        MODULE_NAME_LENGTH,
                        unpacker->install_info.module_list[i].module_name);
                if (ret != DUER_OK) {
                    DUER_FREE(unpacker->install_info.module_list);

                    return DUER_ERR_FAILED;
                }

                ret = duer_ota_unpacker_get_string_val(
                        "signature",
                        module_json,
                        MODULE_SIGNATURE_LENGTH,
                        unpacker->install_info.module_list[i].module_signature);
                if (ret != DUER_OK) {
                    DUER_FREE(unpacker->install_info.module_list);

                    return DUER_ERR_FAILED;
                }

                ret = duer_ota_unpacker_get_string_val(
                        "version",
                        module_json,
                        MODULE_VERSION_LENGTH,
                        unpacker->install_info.module_list[i].module_version);
                if (ret != DUER_OK) {
                    DUER_FREE(unpacker->install_info.module_list);

                    return DUER_ERR_FAILED;
                }

                ret = duer_ota_unpacker_get_string_val(
                        "hw_version",
                        module_json,
                        MODULE_VERSION_LENGTH,
                        unpacker->install_info.module_list[i].module_support_hardware_version);
                if (ret != DUER_OK) {
                    DUER_FREE(unpacker->install_info.module_list);

                    return DUER_ERR_FAILED;
                }
#if 0
                ret = duer_ota_unpacker_get_string_val(
                        "install_path",
                        module_json,
                        INSTALL_PATH_LENGTH,
                        unpacker->install_info.module_list[i].module_install_path);
                if (ret != DUER_OK) {
                    // Free module list
                    return DUER_ERR_FAILED;
                }
#endif

                ret = duer_ota_unpacker_get_int_val(
                        "size",
                        module_json,
                        &unpacker->install_info.module_list[i].module_size);
                if (ret != DUER_OK) {
                    DUER_FREE(unpacker->install_info.module_list);

                    return DUER_ERR_FAILED;
                }
            }
        }
    } else {
        DUER_LOGE("OTA Unpacker: No module information");

        return DUER_ERR_FAILED;
    }

    return DUER_OK;
}

static int duer_ota_unpacker_parse_basic_info(
        duer_ota_unpacker_t *unpacker,
        baidu_json const *meta_json)
{
    int ret = 0;
    int update = 0;
    size_t str_len = 0;
    char *app_name = NULL;
    char *package_type = NULL;
    char *meta_version = NULL;
    baidu_json *update_json = NULL;
    baidu_json *app_name_json = NULL;
    baidu_json *basic_info_json = NULL;
    baidu_json *package_type_json = NULL;
    baidu_json *meta_version_json = NULL;

    if (unpacker == NULL || meta_json == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    basic_info_json = baidu_json_GetObjectItem(meta_json, "basic_info");
    if (basic_info_json == NULL) {
        DUER_LOGW("OTA Unpacker: Parse basic info json failed");
    }

    app_name_json = baidu_json_GetObjectItem(basic_info_json, "app_name");
    if (app_name_json == NULL) {
        DUER_LOGW("OTA Unpacker: Parse app name json failed");
    } else {
        app_name = app_name_json->valuestring;

        str_len = DUER_STRLEN(app_name);
        if (str_len > PACKAGE_NAME_LENGTH) {
            DUER_LOGE("OTA Unpacker: The length of app name is too long");

            return DUER_ERR_FAILED;
        }

        DUER_MEMMOVE(unpacker->basic_info.package_name, app_name, str_len);
    }

    update_json = baidu_json_GetObjectItem(basic_info_json, "update");
    if (update_json == NULL) {
        DUER_LOGW("OTA Unpacker: Parse update json failed");
    } else {
        update = update_json->valueint;
        if (update == 1) {
            unpacker->basic_info.update = true;
        } else {
            unpacker->basic_info.update = false;
        }
    }

    package_type_json = baidu_json_GetObjectItem(basic_info_json, "package_type");
    if (package_type_json == NULL) {
        DUER_LOGW("OTA Unpacker: Parse package type json failed");
    } else {
        package_type = package_type_json->valuestring;

        ret = DUER_STRNCMP(package_type, "App", 3);
        if (ret == 0) {
            unpacker->basic_info.package_type = PACKAGE_TYPE_APP;
        } else {
            ret = DUER_STRNCMP(package_type, "OS", 2);
            if (ret == 0) {
                unpacker->basic_info.package_type = PACKAGE_TYPE_OS;
            } else {
                ret = DUER_STRNCMP(package_type, "Firmware", 8);
                if (ret == 0) {
                    unpacker->basic_info.package_type = PACKAGE_TYPE_FIRMWARE;
                } else {
                    unpacker->basic_info.package_type = PACKAGE_TYPE_MIXED;
                }
            }
        }
    }

#if 0
    meta_version_json = baidu_json_GetObjectItem(meta_json, "meta_version");
    if (meta_version == NULL) {
        DUER_LOGE("OTA Unpacker: Parse meta version json failed");
    } else {
        meta_version =meta_version_json->valuestring;

        str_len = DUER_STRLEN(meta_version);
        if (str_len > META_VERSION_LENGTH) {
            DUER_LOGE("OTA Unpacker: The length of meta version is too long");

            return DUER_ERR_FAILED;
        }

        DUER_MEMMOVE(unpacker->basic_info.meta_version, meta_version, str_len);
    }
#endif

    return DUER_OK;
}

static int duer_ota_unpacker_parse_meta_data(duer_ota_unpacker_t *unpacker)
{
    int ret = DUER_OK;
    unsigned int length = 0;
    unsigned int module_count = 0;
    baidu_json *meta_json = NULL;

    if (unpacker == NULL || unpacker->meta_data == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    DUER_LOGE("OTA Unpacker: Meta data: %s", unpacker->meta_data);

    meta_json = baidu_json_Parse(unpacker->meta_data);
    if (meta_json == NULL) {
        DUER_LOGE("OTA Unpacker: Parse meta json failed");

        return DUER_ERR_FAILED;
    }

    do {
        ret = duer_ota_unpacker_parse_basic_info(unpacker, meta_json);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Unpacker: Parse basic info failed");

            break;
        }

        ret = duer_ota_unpacker_parse_install_info(unpacker, meta_json);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Unpacker: Parse install info failed");

            break;
        }
    } while(0);

    baidu_json_Delete(meta_json);

    return ret;
}

duer_ota_unpacker_t *duer_ota_unpacker_create_unpacker(void)
{
    int ret = 0;
    char const *err_msg = NULL;
    duer_ota_unpacker_t *unpacker = NULL;

    do {
        unpacker = (duer_ota_unpacker_t *)DUER_MALLOC(sizeof(*unpacker));
        if (unpacker == NULL) {
            DUER_LOGE("OTA Unpacker: Malloc failed");

            return NULL;
        }

        DUER_MEMSET(unpacker, 0, sizeof(*unpacker));

        unpacker->lock = duer_mutex_create();
        if (unpacker->lock == NULL) {
            DUER_LOGE("OTA Unpacker: Create lock failed");

            break;
        }

        // Now we only use Zlib
        unpacker->decompression = duer_ota_unpacker_get_decompression(ZLIB);
        if (unpacker->decompression == NULL) {
            DUER_LOGE("OTA Unpacker: Get decompression failed");

            break;
        }

        ret = duer_ota_unpacker_init_decompression(unpacker->decompression);
        if (ret != DUER_OK) {
            err_msg = duer_ota_decompression_get_err_msg(unpacker->decompression);
            if (err_msg != NULL) {
                DUER_LOGE("OTA Unpacker: Init decompression failed %s", err_msg);
            } else {
                DUER_LOGE("OTA Unpacker: Init decompression failed");
            }

            break;
        }

        unpacker->verifier = duer_ota_verification_create_verifier();
        if (unpacker->verifier == NULL) {
            DUER_LOGE("OTA Updater: Create verifier failed");

            ret = DUER_ERR_FAILED;

            break;
        }

        ret = duer_ota_unpacker_set_unpacker_mode(unpacker, SAVE_PACKAGE_HEADER);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Unpacker: Set unpacker mode failed");

            break;
        }

        return unpacker;
    } while (0);

    if (unpacker->lock != NULL) {
        duer_mutex_destroy(unpacker->lock);
    }

    if (unpacker->verifier != NULL) {
        duer_ota_verification_destroy_verifier(unpacker->verifier);
    }

    if (unpacker != NULL) {
        DUER_FREE(unpacker);
    }

    return NULL;
}

int duer_ota_unpacker_destroy_unpacker(duer_ota_unpacker_t *unpacker)
{
    int ret = 0;

    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_ota_unpacker_uninit_decompression(unpacker->decompression);
    if (ret != Z_OK) {
        DUER_LOGE("OTA Unpacker: Uninit decompression failed ret: %d", ret);

        return DUER_ERR_FAILED;
    }

    if (unpacker->meta_data != NULL) {
        DUER_FREE(unpacker->meta_data);
    }

    if (unpacker->lock != NULL) {
        duer_mutex_destroy(unpacker->lock);
    }

    if (unpacker->verifier != NULL) {
        DUER_LOGI("OTA Updater: Destroy verifier");

        duer_ota_verification_destroy_verifier(unpacker->verifier);
    }

    DUER_FREE(unpacker);

    return DUER_OK;
}

duer_ota_package_basic_info_t const *duer_ota_unpacker_get_package_basic_info(
        duer_ota_unpacker_t const *unpacker)
{
    int ret = DUER_OK;
    duer_ota_package_basic_info_t const *info = NULL;

    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return NULL;
    }

    ret = duer_mutex_lock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpacker: Lock unpacker failed");

        return NULL;
    }

    info = &unpacker->basic_info;

    ret = duer_mutex_unlock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpacker: Unlock unpacker failed");

        return NULL;
    }

    return info;
}

duer_ota_package_header_t const *duer_ota_unpacker_get_package_header_info(
        duer_ota_unpacker_t const *unpacker)
{
    int ret = DUER_OK;
    duer_ota_package_header_t const *header = NULL;

    if (unpacker == NULL) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return NULL;
    }

    ret = duer_mutex_lock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpacker: Lock unpacker failed");

        return NULL;
    }

    header = &unpacker->package_header;

    ret = duer_mutex_unlock(unpacker->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Unpacker: Unlock unpacker failed");

        return NULL;
    }

    return header;
}

static int decompress_data_handler(
        void *custom_data,
        char const *buf,
        size_t decompress_data_size)
{
    int ret = DUER_OK;
    size_t module_size = 0;
    duer_ota_unpacker_t *unpacker = NULL;
    duer_ota_installer_t *installer = NULL;
    duer_ota_unpacker_mode_t previous_state;
    duer_ota_unpacker_mode_t current_state;
    duer_ota_package_header_t const *header_info = NULL;

    if (custom_data == NULL || buf == NULL || decompress_data_size <= 0) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    unpacker = (duer_ota_unpacker_t *)custom_data;

    installer = unpacker->installer;

    unpacker->state.decompress_data_size += decompress_data_size;

    if (unpacker->state.meta_data_offset < unpacker->package_header.meta_size) {
        duer_ota_unpacker_set_unpacker_mode(unpacker, SAVE_META_DATA);
    } else {
        duer_ota_unpacker_set_unpacker_mode(unpacker, DISTRIBUTE_MODULE_DATA);
    }

    current_state = duer_ota_unpacker_get_unpacker_mode(unpacker);
    if (current_state < 0 || current_state >= UNPACKER_MODE_MAX) {
        DUER_LOGE("OTA Unpacker: The state of the unpacker is wrong %d", unpacker->state.mode);

        return DUER_ERR_FAILED;
    }

    previous_state = current_state;

    while(1) {
        switch (current_state) {
            case SAVE_META_DATA:
                ret = duer_ota_unpacker_save_meta_data(unpacker, buf, decompress_data_size);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Process meta failed ret: %d", ret);

                    unpacker->err_msg = "Save meta data failed";

                    return ret;
                }

                break;
            case PARSE_META_DATA:
                ret = duer_ota_unpacker_parse_meta_data(unpacker);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Parse meta data failed");

                    unpacker->err_msg = "Parse meta data failed";

                    return ret;
                }

                duer_ota_unpacker_show_package_basic_info(&unpacker->basic_info);

                duer_ota_unpacker_show_package_install_info(&unpacker->install_info);

                duer_ota_unpacker_set_unpacker_mode(unpacker, GET_OTA_INSTALLER);

                break;
            case GET_OTA_INSTALLER:
                // Now we only support one module
                installer = duer_ota_installer_get_installer(
                        unpacker->install_info.module_list[0].module_name);
                if (installer == NULL) {
                    DUER_LOGE("OTA Unpacker: Get %s installer failed",
                            unpacker->install_info.module_list[0].module_name);

                    unpacker->err_msg = "Get OTA installer failed";

                    return DUER_ERR_FAILED;
                }

                unpacker->installer = installer;

                duer_ota_unpacker_set_unpacker_mode(unpacker, DISTRIBUTE_MODULE_INFO);

                break;
            case DISTRIBUTE_MODULE_INFO:
                ret = duer_ota_installer_send_module_info(
                        installer,
                        &unpacker->install_info.module_list[0]);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Send module information failed");

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Distribute module information failed";
                    }

                    break;
                }

                duer_ota_unpacker_set_unpacker_mode(unpacker, NOTIFY_OTA_BEGIN);

                break;
            case NOTIFY_OTA_BEGIN:
                ret = duer_ota_installer_notify_ota_begin(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Notify OTA begin failed");

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Notify OTA begin failed";
                    }

                    return DUER_ERR_FAILED;
                }

                duer_ota_unpacker_set_unpacker_mode(unpacker, DISTRIBUTE_MODULE_DATA);

                break;
            case DISTRIBUTE_MODULE_DATA:
                module_size = decompress_data_size - unpacker->state.used_decompress_data_offset;

                if (unpacker->state.module_data_offset + module_size <
                        unpacker->install_info.module_list[0].module_size) {
                    ret = duer_ota_installer_send_module_data(
                            installer,
                            unpacker->state.module_data_offset,
                            buf + unpacker->state.used_decompress_data_offset,
                            module_size);
                    if (ret != DUER_OK) {
                        DUER_LOGE("OTA Unpacker: Send module data failed");

                        duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                        unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                        if (unpacker->err_msg == NULL) {
                            unpacker->err_msg = "Distribute module data failed";
                        }

                        break;
                    }

                    unpacker->state.module_data_offset += module_size;

                    unpacker->state.used_decompress_data_offset = 0;

                    duer_ota_unpacker_set_unpacker_mode(unpacker, UNZIP_DATA);
                } else {
                    ret = duer_ota_installer_send_module_data(
                            installer,
                            unpacker->state.module_data_offset,
                            buf + unpacker->state.used_decompress_data_offset,
                            unpacker->install_info.module_list[0].module_size -
                            unpacker->state.module_data_offset);
                    if (ret != DUER_OK) {
                        DUER_LOGE("OTA Unpacker: Send module data failed");

                        duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                        unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                        if (unpacker->err_msg == NULL) {
                            unpacker->err_msg = "Distribute module data failed";
                        }

                        break;
                    }

                    unpacker->state.module_data_offset +=
                        unpacker->install_info.module_list[0].module_size -
                        unpacker->state.module_data_offset;

                    unpacker->state.used_decompress_data_offset = 0;

                    duer_ota_unpacker_set_unpacker_mode(unpacker, VERIFY_MODULE_DATA);
                }

                break;
            case VERIFY_MODULE_DATA:
                ret = duer_ota_installer_verify_module_data(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Verify module data failed");

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Verify module data failed";
                    }

                    break;
                }

                /*
                 * Now we support only one module in one package
                 * so we are going to use public key to check the OTA package
                 */
                duer_ota_unpacker_set_unpacker_mode(unpacker, PUBLIC_KEY_VERIFICATION);

                break;
            case PUBLIC_KEY_VERIFICATION:
                header_info = duer_ota_unpacker_get_package_header_info(unpacker);
                if (header_info == NULL) {
                    DUER_LOGE("OTA Unpacker: Get package header failed");

                    unpacker->err_msg = "Get package header information failed";

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    break;
                }

                ret = duer_ota_verification_verify(unpacker->verifier, header_info);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Public key verification failed ret: %d", ret);

                    unpacker->err_msg = "Failure of public key verification";

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);
                } else {
                    duer_ota_unpacker_set_unpacker_mode(unpacker, UPDATE_IMAGE_BEGIN);
                }

                break;
            case CANCEL_OTA_UPDATE:
                ret = duer_ota_installer_cancel_ota_update(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Cancel OTA update failed");

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Cancel OTA update failed";
                    }
                }

                break;
            case UPDATE_IMAGE_BEGIN:
                ret = duer_ota_installer_update_image_begin(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Update image begin failed");

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Update image begin failed";
                    }

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    break;
                }

                duer_ota_unpacker_set_unpacker_mode(unpacker, UPDATE_IMAGE);

                break;
            case UPDATE_IMAGE:
                ret = duer_ota_installer_update_image(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Update image failed");

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Updating image failed";
                    }

                    break;
                }

                duer_ota_unpacker_set_unpacker_mode(unpacker, VERIFY_IMAGE);

                break;
            case VERIFY_IMAGE:
                ret = duer_ota_installer_verify_image(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Verify image failed");

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Verify image failed";
                    }

                    break;
                }

                // Now support one module
                duer_ota_unpacker_set_unpacker_mode(unpacker, NOTIFY_OTA_END);

                break;
            case UNZIP_DATA_DONE:
                DUER_LOGI("OTA Unpacker: Unzip data done");

                break;
            case NOTIFY_OTA_END:
                ret = duer_ota_installer_notify_ota_end(installer);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Notify OTA end failed");

                    duer_ota_unpacker_set_unpacker_mode(unpacker, CANCEL_OTA_UPDATE);

                    unpacker->err_msg = duer_ota_installer_get_err_msg(installer);
                    if (unpacker->err_msg == NULL) {
                        unpacker->err_msg = "Notify OTA end failed";
                    }

                    break;
                }

                // Now support one module
                duer_ota_unpacker_set_unpacker_mode(unpacker, UNZIP_DATA_DONE);

                break;
            default:
                // Do nothing
                break;
        }

        current_state = duer_ota_unpacker_get_unpacker_mode(unpacker);
        if (current_state < 0 || current_state >= UNPACKER_MODE_MAX) {
            DUER_LOGE("OTA Unpacker: The state of the unpacker is wrong %d", unpacker->state.mode);

            ret = DUER_ERR_FAILED;

            break;
        }

        if (previous_state == current_state) {
            break;
        } else {
            previous_state = current_state;
        }
    }

    return ret;
}

int duer_ota_unpacker_unpack_package(
        duer_ota_unpacker_t *unpacker,
        uint8_t const *data,
        size_t size)
{
    int ret = DUER_OK;
    char const *err_msg = NULL;
    duer_ota_unpacker_mode_t previous_state;
    duer_ota_unpacker_mode_t current_state;

    if (unpacker == NULL || data == NULL || size <= 0) {
        DUER_LOGE("OTA Unpacker: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    unpacker->state.received_data_size += size;

    current_state = duer_ota_unpacker_get_unpacker_mode(unpacker);
    if (current_state < 0 || current_state >= UNPACKER_MODE_MAX) {
        DUER_LOGE("OTA Unpacker: The state of the unpacker is wrong %d", unpacker->state.mode);

        return DUER_ERR_FAILED;
    }

    previous_state = current_state;

    while (1) {
        switch (current_state) {
            case SAVE_PACKAGE_HEADER:
                ret = duer_ota_unpacker_save_package_header(unpacker, data, size);
                if (ret != DUER_OK) {
                    unpacker->err_msg = "Save package header failed";

                    DUER_LOGE("OTA Unpacker: Save package header failed");
                }

                break;
            case PARSE_PACKAGE_HEADER:
                duer_ota_unpacker_show_package_header(&unpacker->package_header);

                duer_ota_unpacker_set_unpacker_mode(unpacker, UNZIP_DATA);

                break;
            case UNZIP_DATA:
                duer_ota_verification_update(
                        unpacker->verifier,
                        data + unpacker->state.used_data_offset,
                        size - unpacker->state.used_data_offset);

                ret = duer_ota_unpacker_unzip(
                        unpacker->decompression,
                        data + unpacker->state.used_data_offset,
                        size - unpacker->state.used_data_offset,
                        (void *)unpacker,
                        decompress_data_handler);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Unpacker: Unzip failed ret: %d", ret);

                    err_msg = duer_ota_decompression_get_err_msg(unpacker->decompression);
                    if (err_msg == NULL) {
                        unpacker->err_msg = duer_ota_installer_get_err_msg(unpacker->installer);
                        if (unpacker->err_msg == NULL) {
                            unpacker->err_msg = "Unzip failed";
                        }
                    } else {
                        unpacker->err_msg = err_msg;
                    }

                    return ret;
                }

                unpacker->state.used_data_offset = 0;

                break;
            case CANCEL_OTA_UPDATE:
                 DUER_LOGE("OTA Unpacker: Cancel OTA update");

                 return DUER_ERR_FAILED;
            default:
                // Do nothing
                break;
        }

        current_state = duer_ota_unpacker_get_unpacker_mode(unpacker);
        if (current_state < 0 || current_state >= UNPACKER_MODE_MAX) {
            DUER_LOGE("OTA Unpacker: The state of the unpacker is wrong %d", unpacker->state.mode);

            ret = DUER_ERR_FAILED;

            break;
        }

        if (previous_state == current_state) {
            break;
        } else {
            previous_state = current_state;
        }
    }

    return ret;
}

char const *duer_ota_unpacker_get_err_msg(duer_ota_unpacker_t const *unpacker)
{
    if (unpacker == NULL) {
        return NULL;
    }

    return unpacker->err_msg;
}
