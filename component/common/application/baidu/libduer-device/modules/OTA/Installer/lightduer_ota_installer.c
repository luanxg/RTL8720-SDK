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
 * File: lightduer_ota_installer.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA installer
 */

#include "lightduer_ota_installer.h"
#include <stdint.h>
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"

static duer_ota_installer_t *s_installer_list_head = NULL;
static duer_mutex_t s_lock_ota_installer = NULL;

int duer_ota_init_installer(void)
{
    int ret = DUER_OK;

    if (s_lock_ota_installer == NULL) {
        s_lock_ota_installer = duer_mutex_create();
        if (s_lock_ota_installer == NULL) {
            DUER_LOGE("OTA Installer: Create lock failed");

            ret = DUER_ERR_FAILED;
        }
    }

    return ret;
}

duer_ota_installer_t *duer_ota_installer_create_installer(char const *name)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer = NULL;

    if (name == NULL || DUER_STRLEN(name) > INSTALLER_NAME_LEN) {
       DUER_LOGE("OTA Installer: Argument error");

       return NULL;
    }

    do {
        installer = DUER_MALLOC(sizeof(*installer));
        if (installer == NULL) {
            DUER_LOGE("OTA Installer: Malloc failed");

            ret = DUER_ERR_MEMORY_OVERLOW;

            break;
        }

        DUER_MEMSET(installer, 0, sizeof(*installer));

        DUER_MEMMOVE(installer->name, name, DUER_STRLEN(name));

        installer->lock = duer_mutex_create();
        if (installer->lock == NULL) {
            DUER_LOGE("OTA Installer: Create lock failed");

            ret = DUER_ERR_FAILED;

            break;
        }

        return installer;
    } while(0);

    if (installer != NULL) {
        DUER_FREE(installer);
    }

    return NULL;
}

int duer_ota_installer_destroy_installer(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;

    if (installer == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    // Waring: May be memory leak
    if (installer->custom_data != NULL) {
        ret = duer_ota_installer_notify_ota_end(installer);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Installer: Notify OTA end failed");
        }

        return DUER_ERR_FAILED;
    }

    if (installer->lock != NULL) {
        duer_mutex_destroy(installer->lock);
    }

    DUER_FREE(installer);

    return ret;
}

int duer_ota_installer_register_installer(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer_obj = NULL;
    duer_ota_installer_t *installer_tail = NULL;

    if (installer == NULL
            || installer->lock == NULL
            || installer->notify_ota_begin == NULL
            || installer->notify_ota_end == NULL
            || installer->get_module_info == NULL
            || installer->get_module_data == NULL
            || installer->verify_module_data == NULL
            || installer->update_image_begin == NULL
            || installer->update_image == NULL
            || installer->cancel_ota_update == NULL
            || installer->verify_image == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer list failed");

        return DUER_ERR_FAILED;
    }

    if (s_installer_list_head == NULL) {
        s_installer_list_head = installer;
    } else {
        installer_obj = s_installer_list_head;

        while (installer_obj) {
            ret = DUER_STRNCMP(installer_obj->name, installer->name, INSTALLER_NAME_LEN);
            if (ret == 0) {
                DUER_LOGE("OTA Installer: The installer have already registered");

                ret = DUER_ERR_FAILED;

                break;
            }
            installer_tail = installer_obj;
            installer_obj = installer_obj->next;
        }

        if (ret != DUER_ERR_FAILED) {
            installer_tail->next = installer;
            installer->next = NULL;
        }
    }

    ret = duer_mutex_unlock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer list failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

int duer_ota_installer_unregister_installer(char const *name)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer_obj = NULL;
    duer_ota_installer_t *installer_obj_previous = NULL;

    if (name == NULL || s_installer_list_head == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer list failed");

        return DUER_ERR_FAILED;
    }

    do {
        ret = DUER_STRNCMP(name, s_installer_list_head->name, INSTALLER_NAME_LEN);
        if (ret == 0) {
            DUER_LOGI("OTA Installer: Find the installer");
            installer_obj = s_installer_list_head;
            s_installer_list_head = s_installer_list_head->next;

            ret = duer_ota_installer_destroy_installer(installer_obj);
            if (ret != DUER_OK) {
                DUER_LOGE("OTA Installer: Destroy installer failed");
            }

            break;
        }

        installer_obj = s_installer_list_head->next;
        installer_obj_previous = s_installer_list_head;

        while (installer_obj) {
            ret = DUER_STRNCMP(name, installer_obj->name, INSTALLER_NAME_LEN);
            if (ret == 0) {
                DUER_LOGI("OTA Installer: Find the installer");

                installer_obj_previous->next = installer_obj->next;
                ret = duer_ota_installer_destroy_installer(installer_obj);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Installer: Destroy installer failed");
                }

                break;
            }
            installer_obj_previous = installer_obj;
            installer_obj = installer_obj->next;
        }
    } while (0);

    ret = duer_mutex_unlock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer list failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

duer_ota_installer_t *duer_ota_installer_get_installer(char const *name)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer_obj = NULL;

    if (name == NULL || s_installer_list_head == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return NULL;
    }

    ret = duer_mutex_lock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer list failed");

        return NULL;
    }

    installer_obj = s_installer_list_head;

    while (installer_obj) {
        ret = DUER_STRNCMP(installer_obj->name, name, INSTALLER_NAME_LEN);
        if (ret == 0) {
            DUER_LOGE("OTA Installer: Find the installer name: %s", name);

            break;
        }
        installer_obj = installer_obj->next;
    }

    ret = duer_mutex_unlock(s_lock_ota_installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer list failed");

        return NULL;
    }

    return installer_obj;
}

int duer_ota_installer_notify_ota_begin(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->notify_ota_begin == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->notify_ota_begin(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Notify OTA begin failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_send_module_info(duer_ota_installer_t *installer, duer_ota_module_info_t const *module_info)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || module_info == NULL
            || installer->lock == NULL
            || installer->get_module_info == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->get_module_info(installer, &installer->custom_data, module_info);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Notify module info failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_send_module_data(
        duer_ota_installer_t *installer,
        unsigned int offset,
        unsigned char const *data,
        size_t size)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || data == NULL
            || size <= 0
            || installer->lock == NULL
            || installer->get_module_data == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->get_module_data(installer, installer->custom_data,
            offset, data, size);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Send module data failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_verify_module_data(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->verify_module_data == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->verify_module_data(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Verify image failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}


int duer_ota_installer_update_image_begin(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->update_image_begin == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->update_image_begin(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Update image begin failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_update_image(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->update_image == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->update_image(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Update image failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_verify_image(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->verify_image == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->verify_image(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Verify image failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_notify_ota_end(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->notify_ota_end == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->notify_ota_end(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Notify OTA begin failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

int duer_ota_installer_cancel_ota_update(duer_ota_installer_t *installer)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (installer == NULL
            || installer->lock == NULL
            || installer->cancel_ota_update == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Lock installer failed");

        return DUER_ERR_FAILED;
    }

    err = installer->cancel_ota_update(installer, installer->custom_data);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Installer: Cancel OTA update failed");
    }

    ret = duer_mutex_unlock(installer->lock);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Installer: Unlock installer failed");

        return DUER_ERR_FAILED;
    }

    return err;
}

const char *duer_ota_installer_get_err_msg(duer_ota_installer_t const *installer)
{
    if (installer == NULL || installer->err_msg == NULL) {
        DUER_LOGE("OTA Installer: Argument error");

        return NULL;
    }

    return installer->err_msg;
}
