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
// Desc: Report System infomation to Duer Cloud

#include "duer_system_info.h"
#include "duer_report_system_info.h"
#include <unistd.h>
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_timers.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"

#define NETCARD_NUM      (5)
#define SLEEP_TIME       (2)
#define DELAY_TIME       (5 * 60 * 1000)

#define FILE_LINE_LEN    (1000)
#define SYSTEM_LOG_FILE  "/data/syslog.log"

int duer_report_static_system_info(void)
{
    int ret = DUER_OK;
    baidu_json *data = NULL;
    baidu_json *system_info = NULL;
    duer_system_static_info_t info;

    ret = duer_system_info_get_static_system_info(&info);
    if (ret != DUER_OK) {
        DUER_LOGE("Reporter: Get static system info failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    data = baidu_json_CreateObject();
    if(data == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    system_info = baidu_json_CreateObject();
    if(system_info == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto free_data_obj;
    }

    baidu_json_AddStringToObjectCS(system_info, "OS_Version", (char *)info.os_version);
    baidu_json_AddStringToObjectCS(system_info, "Duer_OS_Version", (char *)info.duer_os_version);
    baidu_json_AddStringToObjectCS(system_info, "Software_Version", (char *)info.sdk_version);
    baidu_json_AddStringToObjectCS(system_info, "Brand", (char *)info.brand);
    baidu_json_AddStringToObjectCS(system_info, "Equipment_type", (char *)info.equipment_type);
    baidu_json_AddStringToObjectCS(system_info, "Hardware_Version", (char *)info.hardware_version);
    baidu_json_AddNumberToObjectCS(system_info, "RAM_Size", info.ram_size);
    baidu_json_AddNumberToObjectCS(system_info, "ROM_Size", info.rom_size);
    baidu_json_AddItemToObjectCS(data, "System_Static_Info", system_info);

    ret = duer_data_report(data);
    if (ret != DUER_OK) {
        DUER_LOGE("Report: Report system static info failed ret: %d", ret);
    }

    baidu_json_Delete(data);

    return ret;

free_data_obj:
    baidu_json_Delete(data);
out:
    return ret;
}

int duer_report_dynamic_system_info(void)
{
    int i = 0;
    int ret = DUER_OK;
    baidu_json *data = NULL;
    baidu_json *array = NULL;
    baidu_json *system_info = NULL;
    duer_system_dynamic_info_t info;

    ret = duer_system_info_get_dynamic_system_info(&info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get system dynamic info failed");

        return DUER_ERR_FAILED;
    }

    data = baidu_json_CreateObject();
    if(data == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    system_info = baidu_json_CreateObject();
    if(system_info == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto free_data_info;
    }

    array = baidu_json_CreateArray();
    if(array == NULL) {
        DUER_LOGE("Reproter: Create json array object failed");

        ret = DUER_ERR_FAILED;

        goto free_system_info;
    }

    baidu_json_AddNumberToObject(system_info, "System_Up_time", info.system_up_time);
    baidu_json_AddNumberToObject(system_info, "Running_Progresses", info.running_progresses);
    baidu_json_AddNumberToObject(system_info, "Total_Progresses", info.total_progresses);
    for (i = 0; i < 3; i++) {
        baidu_json_AddItemToArray(array, baidu_json_CreateNumber(info.system_load_average[i]));
    }
    baidu_json_AddItemToObject(system_info, "System_Load_Average", array);
    baidu_json_AddItemToObject(data, "System_Dynamic_Info", system_info);

    ret = duer_data_report(data);
    if (ret != DUER_OK) {
        DUER_LOGE("Report: Report system dynamic info failed ret: %d", ret);
    }

    baidu_json_Delete(data);

    return ret;

free_system_info:
    baidu_json_Delete(system_info);
free_data_info:
    baidu_json_Delete(data);

out:
    return ret;
}

int duer_report_memory_info(void)
{
    int ret = DUER_OK;
    baidu_json *data = NULL;
    baidu_json *mem_info = NULL;
    duer_memory_info_t info;

    ret = duer_system_info_get_mem_info(&info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get system memory info failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    data = baidu_json_CreateObject();
    if(data == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    mem_info = baidu_json_CreateObject();
    if(mem_info == NULL) {
        DUER_LOGE("Reproter: Create json object failed");

        ret = DUER_ERR_FAILED;

        goto free_data_info;
    }

    baidu_json_AddNumberToObject(mem_info, "Total_Memory_Size", info.total_memory_size);
    baidu_json_AddNumberToObject(mem_info, "Available_Memory_Size", info.available_memory_size);
    baidu_json_AddNumberToObject(mem_info, "Shared_Memory_Size", info.shared_memory_size);
    baidu_json_AddNumberToObject(mem_info, "Buffer_Memory_Size", info.buffer_memory_size);
    baidu_json_AddNumberToObject(mem_info, "Swap_Size", info.swap_size);
    baidu_json_AddNumberToObject(mem_info, "Free_Swap_Size", info.free_swap_size);
    baidu_json_AddNumberToObject(mem_info, "Peak", info.peak);
    baidu_json_AddNumberToObject(mem_info, "Trough", info.trough);
    baidu_json_AddNumberToObject(mem_info, "Average", info.average);
    baidu_json_AddItemToObject(data, "Memory", mem_info);

    ret = duer_data_report(data);
    if (ret != DUER_OK) {
        DUER_LOGE("Report: Report memory info failed ret: %d", ret);
    }

free_data_info:
    baidu_json_Delete(data);
out:
    return ret;
}

int duer_report_disk_info(void)
{
    int ret = DUER_OK;
    baidu_json *data = NULL;
    baidu_json *array = NULL;
    baidu_json *disk_info = NULL;
    duer_disk_info_t *info = NULL;
    duer_disk_info_t *head = NULL;

    ret = duer_system_info_get_disk_info(&info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get disk info failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
    head = info;

    while (info) {
        data = baidu_json_CreateObject();
        if(data == NULL) {
            DUER_LOGE("Reproter: Create json object failed");

            ret = DUER_ERR_FAILED;

            goto free_disk_info;
        }

        disk_info = baidu_json_CreateObject();
        if (disk_info == NULL) {
            DUER_LOGE("Reproter: Create json object failed");

            ret = DUER_ERR_FAILED;

            goto free_data_info;
        }

        baidu_json_AddItemToObject(data, "Disk", disk_info);

        baidu_json_AddStringToObject(disk_info, "Name", (char *)info->partition_name);
        baidu_json_AddNumberToObject(disk_info, "Size", info->total_size);
        baidu_json_AddNumberToObject(disk_info, "Used", info->used_size);
        baidu_json_AddNumberToObject(disk_info, "Available", info->available_size);
        baidu_json_AddStringToObject(disk_info, "Unilization", (char *)info->unilization);
        baidu_json_AddStringToObject(disk_info, "Mount_Info", (char *)info->mount_info);

        ret = duer_data_report(data);
        if (ret != DUER_OK) {
            DUER_LOGE("Report: Report Disk info failed ret: %d", ret);
        }

        baidu_json_Delete(data);

        info = info->next;

        /*
         * The Duer Cloud will ignore the same reporte data point within 1s
         * so we delay 2s
         */
        sleep(SLEEP_TIME);
    }

    duer_system_info_free_disk_info(head);

    return ret;

free_data_info:
    baidu_json_Delete(data);
free_disk_info:
    duer_system_info_free_disk_info(head);
out:
    return ret;
}

int duer_report_network_info(void)
{
    int i = 0;
    int ret = DUER_OK;
    static uint64_t old_rx_packet[NETCARD_NUM];
    static uint64_t old_tx_packet[NETCARD_NUM];
    baidu_json *data = NULL;
    baidu_json *array = NULL;
    baidu_json *quality = NULL;
    baidu_json *network_info = NULL;
    duer_network_card_info_t *info = NULL;
    duer_network_card_info_t *head = NULL;

    ret = duer_system_info_get_network_info(&info);
    if (ret != DUER_OK) {
        DUER_LOGE("Get network info failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }
    head = info;

    while (info) {
        data = baidu_json_CreateObject();
        if(data == NULL) {
            DUER_LOGE("Reproter: Create json object failed");

            ret = DUER_ERR_FAILED;

            goto free_network_info;
        }

        network_info = baidu_json_CreateObject();
        if (network_info == NULL) {
            DUER_LOGE("Reproter: Create json object failed");

            ret = DUER_ERR_FAILED;

            goto free_data_info;
        }
        baidu_json_AddItemToObject(data, "Network", network_info);

        baidu_json_AddStringToObject(network_info, "Name", (char *)info->network_card_name);
        baidu_json_AddStringToObject(network_info, "Link_Encap", (char *)info->link_encap);
        baidu_json_AddStringToObject(network_info, "HW_Address", (char *)info->hw_address);
        baidu_json_AddStringToObject(network_info, "IPV4_Address", (char *)info->ipv4_address);
        baidu_json_AddStringToObject(network_info, "Bcast", (char *)info->bcast);
        baidu_json_AddStringToObject(network_info, "Mask", (char *)info->mask);
        baidu_json_AddStringToObject(network_info, "IPV6_Adress", (char *)info->ipv6_address);
        if (info->quality != NULL) {
            quality = baidu_json_CreateObject();
            if (quality == NULL) {
                DUER_LOGE("Reproter: Create json object failed");

                ret = DUER_ERR_FAILED;

                goto free_data_info;
            }
            baidu_json_AddItemToObject(network_info, "Quality", quality);

            baidu_json_AddNumberToObject(quality, "Link", info->quality->link);
            baidu_json_AddNumberToObject(quality, "Level", info->quality->level);
            baidu_json_AddNumberToObject(quality, "Noise", info->quality->noise);
        }
        baidu_json_AddNumberToObject(network_info, "MTU", info->mtu);
        baidu_json_AddNumberToObject(network_info, "RX_Packet", info->rx_packet);
        baidu_json_AddNumberToObject(network_info, "RX_Error_Packet", info->rx_error_packet);
        baidu_json_AddNumberToObject(network_info, "RX_Dropped_Packet", info->rx_dropped_packet);
        baidu_json_AddNumberToObject(network_info, "RX_Overruns", info->rx_overruns);
        baidu_json_AddNumberToObject(network_info, "RX_Frame", info->rx_frame);
        baidu_json_AddNumberToObject(network_info, "TX_Packet", info->tx_packet);
        baidu_json_AddNumberToObject(network_info, "TX_Error_Packet", info->tx_error_packet);
        baidu_json_AddNumberToObject(network_info, "TX_Dropped_Packet", info->tx_dropped_packet);
        baidu_json_AddNumberToObject(network_info, "TX_Overruns", info->tx_overruns);

        if (i < NETCARD_NUM) {
            baidu_json_AddNumberToObject(network_info, "RX_Average_Speed",
                    (info->rx_packet - old_rx_packet[i]) / (DELAY_TIME / 1000));
            old_rx_packet[i] = info->rx_packet;
            baidu_json_AddNumberToObject(network_info, "TX_Average_Speed",
                    (info->tx_packet - old_tx_packet[i]) / (DELAY_TIME / 1000));
            old_tx_packet[i] = info->tx_packet;
            i++;
        }

        ret = duer_data_report(data);
        if (ret != DUER_OK) {
            DUER_LOGE("Report: Report network info failed ret: %d", ret);
        }

        baidu_json_Delete(data);

        info = info->next;

        /*
         * The Duer Cloud will ignore the same reporte data point within 1s
         * so we delay 2s
         */
        sleep(SLEEP_TIME);
    }

    duer_system_info_free_network_info(head);

    return ret;

free_data_info:
    baidu_json_Delete(data);
free_network_info:
    duer_system_info_free_network_info(head);
out:
    return ret;
}

int duer_report_system_log(enum LogLevel level)
{
    uint32_t len;
    int ret = DUER_OK;
    FILE *fp = NULL;
    char *text = NULL;
    char *search_ret = NULL;
    baidu_json *data = NULL;
    static uint64_t g_curr_offset = 0;

    fp = fopen(SYSTEM_LOG_FILE, "r");
    if (fp == NULL) {
        DUER_LOGE("Can not open file: %s", SYSTEM_LOG_FILE);

        return DUER_ERR_FAILED;
    }

    fseek(fp, g_curr_offset, SEEK_SET);

    while(!feof(fp)) {
        text = DUER_MALLOC(FILE_LINE_LEN + 1);
        if (text == NULL) {
            DUER_LOGE("Malloc failed");

            continue;
        }
        memset(text, 0x0, FILE_LINE_LEN + 1);

        fgets(text, FILE_LINE_LEN, fp);

        len = strlen(text);
        if (len == 0) {
            goto free_text;
        }

        switch (level) {
        case LOG_DEBUG :
            goto report;
        case LOG_INFO :
            if (strstr(text, "info") ) {
                goto report;
            }
        case LOG_NOTICE:
            if (strstr(text, "notice") ) {
                goto report;
            }
        case LOG_WARNING :
            if (strstr(text, "warn") ) {
                goto report;
            }
        case LOG_ERR :
            if (strstr(text, "err") ) {
                goto report;
            }
        case LOG_CRIT :
            if (strstr(text, "crit") ) {
                goto report;
            }
        case LOG_ALERT :
            if (strstr(text, "alert") ) {
                goto report;
            }
        case LOG_EMERG :
            if (strstr(text, "emerg")) {
                goto report;
            }
            break;
        default:
            DUER_LOGI("Can not found any log tag");
        };

        g_curr_offset += len;

        goto free_text;
report:
        data = baidu_json_CreateObject();
        if (data == NULL) {
            DUER_LOGE("Create json object failed");

            goto free_text;
        }

        baidu_json_AddStringToObject(data, "syslog", text);

        ret = duer_data_report(data);
        if (ret != DUER_OK) {
            DUER_LOGE("Report syslog failed");

            goto free_data_json;
        }

        g_curr_offset += len;

free_data_json:
        baidu_json_Delete(data);
free_text:
        DUER_FREE(text);
    }

    fclose(fp);

    return DUER_OK;
}

static void report_system_dynamic_info(void *arg)
{
    duer_report_dynamic_system_info();

    duer_report_memory_info();

    duer_report_disk_info();

    duer_report_network_info();

    duer_report_system_log(LOG_WARNING);
}

int duer_init_system_info_reporter(void)
{
    int ret = 0;

    duer_timer_handler report_timer;

    report_timer = duer_timer_acquire(report_system_dynamic_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(report_timer, DELAY_TIME);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

out:
    return ret;
}
