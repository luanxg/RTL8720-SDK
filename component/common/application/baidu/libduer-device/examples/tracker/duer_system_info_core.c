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
// Author: Zhong Shuai (zhongshuai@baidu.com)
//
// Desc: Provide System Information

#include "duer_system_info.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_log.h"

static duer_system_info_ops_t *s_sys_info_ops = NULL;
volatile static duer_mutex_t s_lock_sys_info = NULL;

int duer_system_info_register_system_info_ops(duer_system_info_ops_t *info_ops)
{
    if (info_ops == NULL
            || info_ops->get_system_static_info == NULL
            || info_ops->get_system_dynamic_info == NULL
            || info_ops->get_memory_info == NULL
            || info_ops->get_disk_info == NULL
            || info_ops->free_disk_info == NULL
            || info_ops->get_network_info == NULL
            || info_ops->free_network_info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_lock_sys_info == NULL) {
        s_lock_sys_info = duer_mutex_create();
        if (s_lock_sys_info == NULL) {
            DUER_LOGE("Sys Info: Create lock failed");

            return DUER_ERR_FAILED;
        }
    }

    s_sys_info_ops = info_ops;

    return DUER_OK;
}

int duer_system_info_init(void)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->init();
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Init failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_get_static_system_info(duer_system_static_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->get_system_static_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Get system static info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_get_dynamic_system_info(duer_system_dynamic_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->get_system_dynamic_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Get system dynamic info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_get_mem_info(duer_memory_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->get_memory_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Get memmory info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_get_disk_info(duer_disk_info_t **info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->get_disk_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Get disk info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_free_disk_info(duer_disk_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->free_disk_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Free disk info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_get_network_info(duer_network_card_info_t **info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->get_network_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Get network info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

int duer_system_info_free_network_info(duer_network_card_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->free_network_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Free network info failed");
    }

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Unlock sys info failed");

        return DUER_ERR_FAILED;
    }

    return status;
}

void duer_system_info_show_system_static_info(duer_system_static_info_t const *info)
{
    if (info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Static Info");
    DUER_LOGI("OS Version: %s", info->os_version);
    DUER_LOGI("Duer OS Version: %s", info->duer_os_version);
    DUER_LOGI("SDK Version :%s", info->sdk_version);
    DUER_LOGI("Brand :%s", info->brand);
    DUER_LOGI("Equipment Type :%s", info->equipment_type);
    DUER_LOGI("Hardware Version :%s", info->hardware_version);
    DUER_LOGI("RAM :%d", info->ram_size);
    DUER_LOGI("ROM :%d", info->rom_size);
    DUER_LOGI("\n");
}

void duer_system_info_show_system_dynamic_info(duer_system_dynamic_info_t const *info)
{
    if (info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Dynamic Info");
    DUER_LOGI("system up time: %d", info->system_up_time);
    DUER_LOGI("System load average: %f %f %f", info->system_load_average[0], info->system_load_average[1], info->system_load_average[2]);
    DUER_LOGI("Running progress: %d", info->running_progresses);
    DUER_LOGI("Total progress : %d", info->total_progresses);
    DUER_LOGI("\n");
}

void duer_system_info_show_memory_info(duer_memory_info_t const *memory_info)
{
    DUER_LOGI("System Memory Info");
    DUER_LOGI("Total Memory Size: %d", memory_info->total_memory_size);
    DUER_LOGI("Available Memory Size: %d", memory_info->available_memory_size);
    DUER_LOGI("Shared Memory Size: %d", memory_info->shared_memory_size);
    DUER_LOGI("Buffer Memory Size: %d", memory_info->buffer_memory_size);
    DUER_LOGI("Swap Size: %d", memory_info->swap_size);
    DUER_LOGI("Free Swap Size: %d", memory_info->free_swap_size);
    DUER_LOGI("Memory peak Size: %d", memory_info->peak);
    DUER_LOGI("Memory Trough Size: %d", memory_info->trough);
    DUER_LOGI("Memory Average Size: %d", memory_info->average);
    DUER_LOGI("\n");
}

void duer_system_info_show_disk_info(duer_disk_info_t const *disk_info)
{
    duer_disk_info_t const *partition = NULL;

    if (disk_info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Disk Info");

    partition = disk_info;
    while (partition) {
        DUER_LOGI("Partition Name: %s", partition->partition_name);
        DUER_LOGI("Total Size: %d", partition->total_size);
        DUER_LOGI("Used Size: %d", partition->used_size);
        DUER_LOGI("Available Size: %d", partition->available_size);
        DUER_LOGI("Unilization: %s", partition->unilization);
        DUER_LOGI("Mount Info: %s", partition->mount_info);
        DUER_LOGI("\n");

        partition = partition->next;
    }

    DUER_LOGI("\n");
}

void duer_system_info_show_networkcard_info(duer_network_card_info_t const *network_info)
{
    duer_network_card_info_t const *net_dev = NULL;

    if (network_info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Network Info");
    net_dev = network_info;
    while (net_dev) {
        DUER_LOGI("Network card name: %s", net_dev->network_card_name);
        if (net_dev->quality != NULL) {
            DUER_LOGI("Wireless Quality");
            DUER_LOGI("Link: %d", net_dev->quality->link);
            DUER_LOGI("Level: %d", net_dev->quality->level);
            DUER_LOGI("noise: %d", net_dev->quality->noise);
        }
        DUER_LOGI("Link encap: %s", net_dev->link_encap);
        DUER_LOGI("HW address: %s", net_dev->hw_address);
        DUER_LOGI("ipv4: %s", net_dev->ipv4_address);
        DUER_LOGI("Bcast: %s", net_dev->bcast);
        DUER_LOGI("Mask: %s", net_dev->mask);
        DUER_LOGI("ipv6: %s", net_dev->ipv6_address);
        DUER_LOGI("MTU: %d", net_dev->mtu);
        DUER_LOGI("RX packets: %d", net_dev->rx_packet);
        DUER_LOGI("RX error packets: %d", net_dev->rx_error_packet);
        DUER_LOGI("RX dropped packets: %d", net_dev->rx_dropped_packet);
        DUER_LOGI("RX overruns: %d", net_dev->rx_overruns);
        DUER_LOGI("RX frame: %d", net_dev->rx_frame);
        DUER_LOGI("TX packets: %d", net_dev->tx_packet);
        DUER_LOGI("TX error packets: %d", net_dev->tx_error_packet);
        DUER_LOGI("TX dropped packets: %d", net_dev->tx_dropped_packet);
        DUER_LOGI("TX overruns: %d", net_dev->tx_overruns);
        DUER_LOGI("\n");

        net_dev = net_dev->next;
    }
}
