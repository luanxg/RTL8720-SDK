/*
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
 * Author: Zhong Shuai (zhongshuai@baidu.com)
 *
 * Desc: Provide System Information
 */
#include "lightduer_system_info.h"
#include "lightduer_log.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_ds_log.h"
#include "lightduer_ds_report.h"
#include "lightduer_lib.h"

#define INVALID_RSSI    (0x7f)
#define INVALID_SNR     (0x7fffffff)
#define INVALID_LEVEL   (0x7fffffff)

static duer_system_info_ops_t *s_sys_info_ops = NULL;
volatile static duer_mutex_t s_lock_sys_info = NULL;

static int duer_report_system_static_info(void);

static baidu_json *report_system_static_info(void)
{
    int ret = DUER_OK;
    baidu_json *report_data_json = NULL;
    baidu_json *system_static_info_json = NULL;
    duer_system_static_info_t static_system_info;

    ret = duer_system_info_get_static_system_info(&static_system_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get system static info failed\n");

        goto out;
    }

    report_data_json = baidu_json_CreateObject();
    if (report_data_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto out;
    }

    system_static_info_json = baidu_json_CreateObject();
    if (system_static_info_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_report_data_json;
    }

    if (DUER_STRLEN(static_system_info.os_version) > 0) {
        baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "OS_Ver",
                static_system_info.os_version);
    }
    baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "Duer_OS_Ver",
                DUER_OS_VERSION);
    if (DUER_STRLEN(static_system_info.sw_version) > 0) {
        baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "SW_Ver",
                static_system_info.sw_version);
    }
    if (DUER_STRLEN(static_system_info.brand) > 0) {
        baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "Brand",
                static_system_info.brand);
    }
    if (DUER_STRLEN(static_system_info.equipment_type) > 0) {
        baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "Equipment_Type",
                static_system_info.equipment_type);
    }
    if (DUER_STRLEN(static_system_info.hardware_version) > 0) {
        baidu_json_AddStringToObjectCS(
                system_static_info_json,
                "HW_Ver",
                static_system_info.hardware_version);
    }
    if (static_system_info.ram_size > 0) {
        baidu_json_AddNumberToObjectCS(
                system_static_info_json,
                "ram_size",
                static_system_info.ram_size);
    }
    if (static_system_info.rom_size > 0) {
        baidu_json_AddNumberToObjectCS(
                system_static_info_json,
                "rom_size",
                static_system_info.rom_size);
    }
    baidu_json_AddItemToObjectCS(
                report_data_json,
                "Sys_Static_Info",
                system_static_info_json);

    return report_data_json;

free_report_data_json:
    baidu_json_release(report_data_json);

out:
    return NULL;
}

static baidu_json *report_system_dynamic_info(void)
{
    int ret = DUER_OK;
    char *sys_load = NULL;
    duer_task_info_t *element = NULL;
    baidu_json *task_info_json = NULL;
    baidu_json *task_array_json = NULL;
    baidu_json *report_data_json = NULL;
    baidu_json *system_dynamic_info_json = NULL;
    duer_system_dynamic_info_t dynamic_system_info;

    duer_report_system_static_info();

    ret = duer_system_info_get_dynamic_system_info(&dynamic_system_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get system dynamic info failed\n");

        goto out;
    }

    report_data_json = baidu_json_CreateObject();
    if (report_data_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_dynamic_info;
    }

    system_dynamic_info_json = baidu_json_CreateObject();
    if (system_dynamic_info_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_report_data_json;
    }

    if (dynamic_system_info.system_up_time > 0) {
        baidu_json_AddNumberToObject(
                system_dynamic_info_json,
                "Sys_Up_Time",
                dynamic_system_info.system_up_time);
    }
    if (dynamic_system_info.total_task > 0) {
        baidu_json_AddNumberToObject(
                system_dynamic_info_json,
                "Total_Task",
                dynamic_system_info.total_task);
    }
    if (dynamic_system_info.running_task > 0) {
        baidu_json_AddNumberToObject(
                system_dynamic_info_json,
                "Running Task",
                dynamic_system_info.running_task);
    }

    if (dynamic_system_info.system_load != NULL) {
        sys_load = DUER_MALLOC(dynamic_system_info.system_load_len + 1);
        if (sys_load != NULL) {
            DUER_MEMSET(sys_load, 0, dynamic_system_info.system_load_len + 1);

            DUER_STRNCPY(sys_load,
                        dynamic_system_info.system_load,
                        dynamic_system_info.system_load_len);

            baidu_json_AddStringToObject(
                    system_dynamic_info_json,
                    "Sys_Load",
                    sys_load);

            DUER_FREE(sys_load);
        }
    }

    if (dynamic_system_info.task_info != NULL) {

        element = dynamic_system_info.task_info;

        task_array_json = baidu_json_CreateArray();
        if (task_array_json == NULL) {
            DUER_LOGE("Dev Info: Creater Array json failed");

            goto free_system_dynamic_info_json;
        }

        while (element) {
            task_info_json = baidu_json_CreateObject();
            if (task_info_json == NULL) {
                DUER_LOGE("Dev Info: Creater task info json failed");

                goto free_task_info_array;
            }

            baidu_json_AddStringToObjectCS(task_info_json, "Name", element->task_name);

            baidu_json_AddNumberToObjectCS(task_info_json, "Stack", element->stack_size);
            baidu_json_AddNumberToObjectCS(task_info_json, "Free_Stack", element->stack_free_size);

            if (element->cpu_occupancy_rate >= 0) {
                baidu_json_AddNumberToObjectCS(task_info_json, "CPU", element->cpu_occupancy_rate);
            }

            if (element->stack_max_used_size >= 0) {
                baidu_json_AddNumberToObjectCS(
                        task_info_json,
                        "Max_Used_Stack",
                        element->stack_max_used_size);
            }

            baidu_json_AddItemToArray(task_array_json, task_info_json);

            element = element->next;
        }
    }

    baidu_json_AddItemToObjectCS(system_dynamic_info_json, "Task_Info", task_array_json);

    baidu_json_AddItemToObjectCS(
            report_data_json,
            "Sys_Dynamic_Info",
            system_dynamic_info_json);

    ret = duer_system_info_free_dynamic_system_info(&dynamic_system_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free dynamic system info failed");
    }

    return report_data_json;

free_task_info_array:
    baidu_json_release(task_array_json);

free_system_dynamic_info_json:
    baidu_json_release(system_dynamic_info_json);

free_report_data_json:
    baidu_json_release(report_data_json);

free_dynamic_info:
    ret = duer_system_info_free_dynamic_system_info(&dynamic_system_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free dynamic system info failed");
    }

out:
    return NULL;
}

static baidu_json *report_memory_info(void)
{
    int ret = DUER_OK;
    duer_memory_info_t memory_info;
    baidu_json *report_data_json = NULL;
    baidu_json *memory_info_json = NULL;

    ret = duer_system_info_get_mem_info(&memory_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get system memory info failed\n");

        goto out;
    }

    report_data_json = baidu_json_CreateObject();
    if (report_data_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto out;
    }

    memory_info_json = baidu_json_CreateObject();
    if (memory_info_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_report_data_json;
    }

    switch (memory_info.memory_type) {
    case RAM:
        baidu_json_AddStringToObjectCS(memory_info_json, "Type", "RAM");
        break;
    case PSRAM:
        baidu_json_AddStringToObjectCS(memory_info_json, "Type", "PSRAM");
        break;
    default:
        DUER_LOGW("Undefined memory type, %d", memory_info.memory_type);
        break;
    }

    if (memory_info.total_memory_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Total",
                memory_info.total_memory_size);
    }
    if (memory_info.available_memory_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Available",
                memory_info.available_memory_size);
    }
    if (memory_info.shared_memory_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Shared",
                memory_info.shared_memory_size);
    }
    if (memory_info.buffer_memory_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Buffer",
                memory_info.buffer_memory_size);
    }
    if (memory_info.swap_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Swap",
                memory_info.swap_size);
    }
    if (memory_info.free_swap_size > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Free_Swap",
                memory_info.free_swap_size);
    }
    if (memory_info.peak > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Peak",
                memory_info.peak);
    }
    if (memory_info.trough < ~0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Trough",
                memory_info.trough);
    }
    if (memory_info.average > 0) {
        baidu_json_AddNumberToObjectCS(
                memory_info_json,
                "Average",
                memory_info.average);
    }

    baidu_json_AddItemToObjectCS(
            report_data_json,
            "Memory",
            memory_info_json);

    return report_data_json;

free_report_data_json:
    baidu_json_release(report_data_json);
out:
    return NULL;
}

static baidu_json *report_disk_info(void)
{
    int ret = DUER_OK;
    duer_disk_info_t *disk_info = NULL;
    duer_disk_info_t *disk_info_head = NULL;
    baidu_json *disk_info_json = NULL;
    baidu_json *report_data_json = NULL;
    baidu_json *disk_info_array_json = NULL;

    ret = duer_system_info_get_disk_info(&disk_info);
    if (ret != DUER_OK || disk_info == NULL) {
        DUER_LOGE("Sys Info: Get disk info failed");

        goto out;
    }
    disk_info_head = disk_info;

    report_data_json = baidu_json_CreateObject();
    if (report_data_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_disk_info;
    }

    disk_info_array_json = baidu_json_CreateArray();
    if (disk_info_array_json == NULL) {
        DUER_LOGE("Dev Info: Creater Array json failed");

        goto free_report_data_json;
    }

    while (disk_info) {
        disk_info_json = baidu_json_CreateObject();
        if (disk_info_json == NULL) {
            DUER_LOGE("Create json object failed");

            goto free_disk_info_array_json;
        }
        baidu_json_AddStringToObjectCS(disk_info_json, "Name", disk_info->partition_name);
        if (DUER_STRLEN(disk_info->mount_info) != 0) {
            baidu_json_AddStringToObjectCS(disk_info_json, "Mount_Info", disk_info->mount_info);
        }
        baidu_json_AddNumberToObjectCS(disk_info_json, "Size", disk_info->total_size);
        baidu_json_AddNumberToObjectCS(disk_info_json, "Used", disk_info->used_size);
        baidu_json_AddNumberToObjectCS(disk_info_json, "Available", disk_info->available_size);
        baidu_json_AddNumberToObjectCS(disk_info_json, "Unilization", disk_info->unilization);

        baidu_json_AddItemToArray(disk_info_array_json, disk_info_json);

        disk_info = disk_info->next;
    }

    baidu_json_AddItemToObjectCS(report_data_json, "Disk", disk_info_array_json);

    ret = duer_system_info_free_disk_info(disk_info_head);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free disk info failed");
    }

    return report_data_json;

free_disk_info_array_json:
    baidu_json_release(disk_info_array_json);

free_report_data_json:
    baidu_json_release(report_data_json);

free_disk_info:
    ret = duer_system_info_free_disk_info(disk_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free disk info failed");
    }

out:
    return NULL;
}

static baidu_json *report_network_info(void)
{
    int ret = DUER_OK;
    duer_network_info_t network_info;
    baidu_json *report_data_json = NULL;
    baidu_json *network_info_json = NULL;
    baidu_json *wireless_info_json = NULL;

    ret = duer_system_info_get_network_info(&network_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get network info failed");

        goto out;
    }

    report_data_json = baidu_json_CreateObject();
    if (report_data_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_network_info;
    }

    network_info_json = baidu_json_CreateObject();
    if (network_info_json == NULL) {
        DUER_LOGE("Create json object failed");

        goto free_report_data_json;
    }

    switch (network_info.network_type) {
    case ETHERNET_TYPE:
        baidu_json_AddStringToObject(
                network_info_json,
                "Link_Encap",
                "ETHERNET");
        break;
    case WIFI_TYPE:
        baidu_json_AddStringToObject(
                network_info_json,
                "Link_Encap",
                "WIFI");
        break;
    default:
        DUER_LOGW("There is no network type defined");
        break;
    }

    if (network_info.network_type == WIFI_TYPE) {
        wireless_info_json = baidu_json_CreateObject();
        if (wireless_info_json == NULL) {
            DUER_LOGE("Create json object failed");

            goto free_network_info_json;
        }

        if (network_info.wireless.link != INVALID_RSSI) {
            baidu_json_AddNumberToObjectCS(
                    wireless_info_json,
                    "Link",
                    network_info.wireless.link);
        }
        if (network_info.wireless.level != INVALID_LEVEL) {
            baidu_json_AddNumberToObjectCS(
                    wireless_info_json,
                    "Level",
                    network_info.wireless.level);
        }
        if (network_info.wireless.noise != INVALID_SNR) {
            baidu_json_AddNumberToObjectCS(
                    wireless_info_json,
                    "Noise",
                    network_info.wireless.noise);
        }
        if (DUER_STRLEN(network_info.wireless.bssid) != 0) {
            baidu_json_AddStringToObjectCS(
                    wireless_info_json,
                    "BSSID",
                    network_info.wireless.bssid);
        }
        if (DUER_STRLEN(network_info.wireless.ssid) != 0) {
            baidu_json_AddStringToObjectCS(
                    wireless_info_json,
                    "SSID",
                    network_info.wireless.ssid);
        }

        baidu_json_AddItemToObjectCS(
                network_info_json,
                "Detail",
                wireless_info_json);
    }

    if (DUER_STRLEN(network_info.hw_address) != 0) {
        baidu_json_AddStringToObjectCS(
                network_info_json,
                "HW_Addr",
                network_info.hw_address);
    }
    if (DUER_STRLEN(network_info.ipv4_address) != 0) {
        baidu_json_AddStringToObjectCS(
                network_info_json,
                "IPV4_Addr",
                network_info.ipv4_address);
    }
    if (DUER_STRLEN(network_info.ipv6_address) != 0) {
        baidu_json_AddStringToObjectCS(
                network_info_json,
                "IPV6_Addr",
                network_info.ipv6_address);
    }
    if (network_info.mtu > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "MTU",
                network_info.mtu);
    }
    if (network_info.transmitted_packets > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Transmitted",
                network_info.transmitted_packets);
    }
    if (network_info.received_packets > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Received",
                network_info.received_packets);
    }
    if (network_info.forwarded_packets > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Forwarded",
                network_info.forwarded_packets);
    }
    if (network_info.dropped_packets > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Dropped",
                network_info.dropped_packets);
    }
    if (network_info.checksum_error > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Checksum",
                network_info.checksum_error);
    }
    if (network_info.invalid_length_error > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Invalid_Length",
                network_info.invalid_length_error);
    }
    if (network_info.routing_error > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Routing",
                network_info.routing_error);
    }
    if (network_info.protocol_error > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "Protocol",
                network_info.protocol_error);
    }
    if (network_info.rx_average_speed > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "RX_Aver_Speed",
                network_info.rx_average_speed);
    }
    if (network_info.tx_average_speed > 0) {
        baidu_json_AddNumberToObjectCS(
                network_info_json,
                "TX_Aver_Speed",
                network_info.tx_average_speed);
    }
    baidu_json_AddItemToObjectCS(
                report_data_json,
                "Network",
                network_info_json);

    ret = duer_system_info_free_network_info(&network_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free network info failed");
    }

    return report_data_json;

free_network_info_json:
    baidu_json_release(network_info_json);

free_report_data_json:
    baidu_json_release(report_data_json);

free_network_info:
    ret = duer_system_info_free_network_info(&network_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Free network info failed");
    }

out:
    return NULL;
}

static int duer_report_system_static_info(void)
{
    int ret = DUER_OK;

    static duer_bool is_reported = DUER_FALSE;

    if (is_reported) {
        return DUER_OK;
    }

    is_reported = DUER_TRUE;

    baidu_json *report_static_system_info_json = NULL;

    report_static_system_info_json = report_system_static_info();

    ret = duer_ds_log(
            DUER_DS_LOG_LEVEL_INFO,
            DUER_DS_LOG_MODULE_SYSTEM,
            0,
            report_static_system_info_json);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Report system static info failed");
    }

    return ret;
}

static int duer_system_info_init(void)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (s_sys_info_ops == NULL || s_sys_info_ops->init == NULL) {
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

int duer_init_system_info(void)
{
    int ret = DUER_OK;

    ret = duer_system_info_init();
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: init failed ret: %d", ret);
    }

    ret = duer_ds_register_report_function(report_system_dynamic_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Register report system dynamic function failed ret: %d", ret);
    }

    ret = duer_ds_register_report_function(report_memory_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Register report memory function failed ret: %d", ret);
    }

    ret = duer_ds_register_report_function(report_network_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Register report network function failed ret: %d", ret);
    }

    ret = duer_ds_register_report_function(report_disk_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Register report disk function failed ret: %d", ret);
    }

    return ret;
}

int duer_system_info_register_system_info_ops(duer_system_info_ops_t *info_ops)
{
    if (info_ops == NULL
            || info_ops->get_system_static_info == NULL
            || info_ops->get_system_dynamic_info == NULL
            || info_ops->free_system_dynamic_info == NULL
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

int duer_system_info_get_static_system_info(duer_system_static_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL || s_sys_info_ops->get_system_static_info == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    DUER_MEMSET(info, 0, sizeof(*info));

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

    if (s_sys_info_ops == NULL || s_sys_info_ops->get_system_dynamic_info == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    DUER_MEMSET(info, 0, sizeof(*info));

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

int duer_system_info_free_dynamic_system_info(duer_system_dynamic_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL || s_sys_info_ops->free_system_dynamic_info == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Lock sys info failed");

        return DUER_ERR_FAILED;
    }

    status = s_sys_info_ops->free_system_dynamic_info(info);
    if (status != DUER_OK) {
        DUER_LOGE("Sys Info: Free system dynamic info failed");
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

    if (s_sys_info_ops == NULL || s_sys_info_ops->get_memory_info == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    DUER_MEMSET(info, 0, sizeof(*info));
    info->trough = ~0;

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

    if (s_sys_info_ops == NULL || s_sys_info_ops->get_disk_info == NULL) {
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

    if (s_sys_info_ops == NULL || s_sys_info_ops->free_disk_info == NULL) {
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

int duer_system_info_get_network_info(duer_network_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL || s_sys_info_ops->get_network_info == NULL) {
        DUER_LOGE("Sys Info: Uninit system info");

        return DUER_ERR_FAILED;
    }

    DUER_MEMSET(info, 0, sizeof(*info));

    info->wireless.link = INVALID_RSSI;
    info->wireless.level = INVALID_LEVEL;
    info->wireless.noise = INVALID_SNR;

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

int duer_system_info_free_network_info(duer_network_info_t *info)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_sys_info_ops == NULL || s_sys_info_ops->free_network_info == NULL) {
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
    if (DUER_STRLEN(info->os_version) > 0) {
        DUER_LOGI("OS Version: %s", info->os_version);
    }
    DUER_LOGI("Duer OS Version: %s", DUER_OS_VERSION);
    if (DUER_STRLEN(info->sw_version) > 0) {
        DUER_LOGI("OS Version: %s", info->sw_version);
    }
    if (DUER_STRLEN(info->brand) > 0) {
        DUER_LOGI("Brand :%s", info->brand);
    }
    if (DUER_STRLEN(info->equipment_type) > 0) {
        DUER_LOGI("Equipment Type :%s", info->equipment_type);
    }
    if (DUER_STRLEN(info->hardware_version) > 0) {
        DUER_LOGI("Hardware Version :%s", info->hardware_version);
    }
    DUER_LOGI("RAM :%d", info->ram_size);
    DUER_LOGI("ROM :%d", info->rom_size);
    DUER_LOGI("\n");
}

void duer_system_info_show_system_dynamic_info(duer_system_dynamic_info_t const *info)
{
    char *sys_load = NULL;

    if (info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Dynamic Info");
    DUER_LOGI("System Up Time: %d", info->system_up_time);
    DUER_LOGI("Total Task: %d", info->total_task );
    DUER_LOGI("Running Task: %d", info->running_task);

    if (info->system_load != NULL) {
        sys_load = DUER_MALLOC(info->system_load_len + 1);
        if (sys_load != NULL) {
            DUER_MEMSET(sys_load, 0, info->system_load_len + 1);

            DUER_STRNCPY(sys_load, info->system_load, info->system_load_len);

            DUER_LOGI("System load: %s", sys_load);

            DUER_FREE(sys_load);
        }
    }
}

void duer_system_info_show_memory_info(duer_memory_info_t const *memory_info)
{
    if (memory_info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Memory Info");
    switch (memory_info->memory_type) {
    case RAM:
        DUER_LOGI("Memory Type: RAM");
        break;
    case PSRAM:
        DUER_LOGI("Memory Type: PSRAM");
        break;
    default:
        DUER_LOGI("Undefined Memory Type");
    }

    DUER_LOGI("Total Memory: %d", memory_info->total_memory_size);
    DUER_LOGI("Available Memory: %d", memory_info->available_memory_size);

    if (memory_info->shared_memory_size >= 0) {
        DUER_LOGI("Shared memory: %d", memory_info->shared_memory_size);
    }
    if (memory_info->buffer_memory_size >= 0) {
        DUER_LOGI("Buffer memory: %d", memory_info->buffer_memory_size);
    }
    if (memory_info->swap_size >= 0) {
        DUER_LOGI("Swap size: %d", memory_info->swap_size);
    }
    if (memory_info->free_swap_size >= 0) {
        DUER_LOGI("Free swap size: %d", memory_info->free_swap_size);
    }
    DUER_LOGI("Memory peak size: %d", memory_info->peak);
    DUER_LOGI("Memory trough size: %d", memory_info->trough);
    DUER_LOGI("Memory average size: %d", memory_info->average);
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
        if (DUER_STRLEN(partition->partition_name) > 0) {
            DUER_LOGI("Partition Name: %s", partition->partition_name);
        }
        DUER_LOGI("Total Size: %d", partition->total_size);
        DUER_LOGI("Used Size: %d", partition->used_size);
        DUER_LOGI("Available Size: %d", partition->available_size);
        DUER_LOGI("Unilization: %d", partition->unilization);
        if (DUER_STRLEN(partition->mount_info) > 0) {
            DUER_LOGI("Mount Info: %s", partition->mount_info);
        }
        DUER_LOGI("\n");

        partition = partition->next;
    }

    DUER_LOGI("\n");
}

void duer_system_info_show_networkcard_info(duer_network_info_t const *network_info)
{
    if (network_info == NULL) {
        DUER_LOGE("Argument Error");

        return;
    }

    DUER_LOGI("System Network Info");
    if (DUER_STRLEN(network_info->hw_address) > 0) {
        DUER_LOGI("HW Address: %s", network_info->hw_address);
    }
    if (DUER_STRLEN(network_info->wireless.bssid) > 0) {
        DUER_LOGI("BSSID: %s", network_info->wireless.bssid);
    }
    if (DUER_STRLEN(network_info->wireless.ssid) > 0) {
        DUER_LOGI("SSID: %s", network_info->wireless.ssid);
    }
    if (DUER_STRLEN(network_info->ipv4_address) > 0) {
        DUER_LOGI("IPV4 Address: %s", network_info->ipv4_address);
    }
    if (DUER_STRLEN(network_info->ipv6_address) > 0) {
        DUER_LOGI("IPV6 Address: %s", network_info->ipv6_address);
    }
    if (network_info->network_type == WIFI_TYPE) {
        DUER_LOGI("Wireless link: %d", network_info->wireless.link);
        DUER_LOGI("Wireless level: %d", network_info->wireless.level);
        DUER_LOGI("Wireless noise: %d", network_info->wireless.noise);
    }
    DUER_LOGI("MTU: %d", network_info->mtu);
    DUER_LOGI("Transmitted: %d", network_info->transmitted_packets);
    DUER_LOGI("Received: %d", network_info->received_packets);
    DUER_LOGI("Forwarded: %d", network_info->forwarded_packets);
    DUER_LOGI("Dropped: %d", network_info->dropped_packets);
    DUER_LOGI("Checksum: %d", network_info->checksum_error);
    DUER_LOGI("Invalid Length: %d", network_info->invalid_length_error);
    DUER_LOGI("Routing: %d", network_info->routing_error);
    DUER_LOGI("Protocol: %d", network_info->protocol_error);
    DUER_LOGI("Transmitted Average Speed: %d", network_info->tx_average_speed);
    DUER_LOGI("Received Average Speed: %d", network_info->rx_average_speed);
}

int duer_uninit_system_info(void)
{
    int ret = DUER_OK;
    if (s_lock_sys_info != NULL) {
        ret = duer_mutex_destroy(s_lock_sys_info);
        s_lock_sys_info = NULL;
    }

    s_sys_info_ops = NULL;

    return ret;
}
