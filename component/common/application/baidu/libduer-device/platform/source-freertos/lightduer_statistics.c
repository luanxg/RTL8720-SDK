/* Copyright (2017) Baidu Inc. All rights reserved.
 *
 * File: duerapp_system_info.c
 * Auth: Zhong Shuai(zhongshuai@baidu.com)
 * Desc: Provide the information about the system
 */

#include "lightduer_system_info.h"
#include "lightduer_statistics.h"
#include <string.h>
//Edit by Realtek
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lightduer_mutex.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_timestamp.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_timestamp.h"

#ifdef DUER_PLATFORM_ESPRESSIF
#include "esp_system.h"
#include "esp_wifi.h"

#define DUER_GET_FREE_HEAP_SIZE(...)    esp_get_free_heap_size()

#else

#define DUER_GET_FREE_HEAP_SIZE(...)    xPortGetFreeHeapSize()

#endif

#define DELAY_TIEM (5 * 1000)

extern int duer_system_info_init(void);

volatile static uint32_t s_peak = 0;
volatile static uint32_t s_trough = (~0x0);
volatile static uint32_t s_sum = 0;
volatile static uint32_t s_counts = 0;
volatile static duer_mutex_t s_lock_sys_info = NULL;
static duer_timer_handler s_timer;

extern const duer_system_static_info_t g_system_static_info;

static void calculate_system_info(void *arg)
{
    int ret = DUER_OK;
    uint32_t free_heap_size = 0;

    free_heap_size = DUER_GET_FREE_HEAP_SIZE();

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");
        return;
    }

    if (free_heap_size > s_peak) {
        s_peak = free_heap_size;
    }

    if (free_heap_size < s_trough) {
        s_trough = free_heap_size;
    }

    s_sum += free_heap_size;
    s_counts++;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");
    }

    return;
}

static int duer_init_system_info_module(void)
{
    int ret = 0;

    if (s_lock_sys_info != NULL) {
        return DUER_OK;
    }

    s_lock_sys_info = duer_mutex_create();
    if (s_lock_sys_info == NULL) {
        DUER_LOGE("Create lock failed");

        return DUER_ERR_FAILED;
    }

    s_timer = duer_timer_acquire(calculate_system_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(s_timer, DELAY_TIEM);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        ret = DUER_ERR_FAILED;
    }

    return DUER_OK;
}

static int duer_get_system_static_info(duer_system_static_info_t *info)
{
    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");
        return DUER_ERR_INVALID_PARAMETER;
    }

    memcpy(info, &g_system_static_info, sizeof(*info));

    return DUER_OK;
}

static int duer_get_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    unsigned portBASE_TYPE num_of_tasks = 0;

    num_of_tasks = uxTaskGetNumberOfTasks();
    info->total_task = num_of_tasks;

#if (configUSE_TRACE_FACILITY == 1)
    uint32_t run_time = 0;
    UBaseType_t task_array_size = 0;
    TaskStatus_t *task_status_array = NULL;

    task_status_array = DUER_MALLOC(num_of_tasks * sizeof(TaskStatus_t));

    task_array_size = uxTaskGetSystemState(task_status_array, num_of_tasks, &run_time);

    info->system_up_time = run_time;
    info->running_task   = task_array_size;

    DUER_FREE(task_status_array);
#else

    info->system_up_time = duer_timestamp();

#endif

    return DUER_OK;
}

static int duer_free_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    return DUER_OK;
}

static int duer_get_memory_info(duer_memory_info_t *mem_info)
{
    int ret = DUER_OK;

    mem_info->total_memory_size = g_system_static_info.ram_size;

    calculate_system_info(NULL);

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");
        return ret;
    }

    mem_info->peak = s_peak;
    mem_info->trough = s_trough;
    if (s_counts > 0) {
        mem_info->average = 1.0f * s_sum / s_counts;
    } else {
        DUER_LOGE("No memory info was measured!!!");
    }

    s_peak = 0;
    s_trough = ~0;
    s_sum = 0;
    s_counts = 0;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");
    }

    return ret;
}

static int duer_get_disk_info(duer_disk_info_t **disk_info)
{
    duer_disk_info_t *info = NULL;

    if (disk_info == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    /**
     * TODU: You SHOULD impliment this function for obtain the partition remain size.
     *
     * Follow is a demo
     *
     * =====================================
     * info = DUER_MALLOC(sizeof(*info));
     * if (info == NULL) {
     *     DUER_LOGE("Malloc failed");
     *
     *     return DUER_ERR_FAILED;
     * }
     *
     * memset(info, 0, sizeof(*info));
     *
     * strncpy(info->partition_name, "Flash", PARTITION_NAME_LEN);
     *
     * info->total_size     = xxx_get_flash_total_size();
     * info->used_size      = xxx_get_flash_used_size();
     * info->available_size = xxx_get_flash_available();
     * info->next           = NULL; // If you have another patitions, linked to the next.
     */

    *disk_info = info;

    return DUER_OK;
}

static int duer_free_disk_info(duer_disk_info_t *disk_info)
{
    duer_disk_info_t *temp = NULL;
    while (disk_info != NULL) {
        temp = disk_info;
        disk_info = temp->next;
        DUER_FREE(temp);
    }

    return DUER_OK;
}

static int get_network_info(duer_network_info_t *network_info)
{
    if (network_info == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    /**
     * TODU: You SHOULD impliment this function for obtain the network information.
     *
     * Follow is a demo
     *
     * =====================================
     *
     * network_info->wireless->link  = xxx_get_rssi();
     * network_info->wireless->level = xxx_get_signal_strength;
     * network_info->wireless->noise = xxx_get_noise();
     *
     * network_info->network_type = WIFI_TYPE;
     * strncpy(network_info->hw_address, xxx_get_mac_address(), HW_ADDRESS_LEN);
     * strncpy(network_info->ipv4_address, xxx_get_ipaddr(), IPV4_ADDRESS_LEN);
     * strncpy(network_info->wireless.bssid, xxx_get_ipaddr(), BSSID_LEN);
     * network_info->mtu = xxx_get_mtu();
     * network_info->transmitted_packets = xxx_get_tx_packets();
     * network_info->received_packets    = xxx_get_rx_packets();
     *
     * return DUER_OK;
     */

#ifdef DUER_PLATFORM_ESPRESSIF
    {
        wifi_ap_record_t record;

        esp_wifi_sta_get_ap_info(&record);

        network_info->network_type = WIFI_TYPE;
        DUER_SNPRINTF(network_info->wireless.ssid, SSID_LEN + 1, record.ssid);
        DUER_SNPRINTF(network_info->wireless.bssid, BSSID_LEN + 1, "%02x:%02x:%02x:%02x:%02x:%02x",
                record.bssid[0], record.bssid[1], record.bssid[2],
                record.bssid[3], record.bssid[4], record.bssid[5]);
        network_info->wireless.link = record.rssi;

        return DUER_OK;
    }
#endif

    // If you want to report this message change this value to DUER_OK
    return DUER_ERR_FAILED;
}

static int duer_free_network_info(duer_network_info_t *network_info)
{
    return DUER_OK;
}

static duer_system_info_ops_t s_system_info_ops = {
    .init                     = duer_init_system_info_module,
    .get_system_static_info   = duer_get_system_static_info,
    .get_system_dynamic_info  = duer_get_system_dynamic_info,
    .free_system_dynamic_info = duer_free_system_dynamic_info,
    .get_memory_info          = duer_get_memory_info,
    .get_disk_info            = duer_get_disk_info,
    .free_disk_info           = duer_free_disk_info,
    .get_network_info         = get_network_info,
    .free_network_info        = duer_free_network_info,
};

int duer_statistics_initialize(void)
{
    int ret = DUER_OK;

    ret = duer_system_info_register_system_info_ops(&s_system_info_ops);
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Register system info ops failed ret: %d", ret);
        return ret;
    }

    ret = duer_init_system_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Init system info failed ret: %d", ret);
    }

    return ret;
}

int duer_statistics_finalize(void)
{
    int ret = DUER_OK;

    if (s_timer != NULL) {
        duer_timer_stop(s_timer);
        duer_timer_release(s_timer);
        s_timer = NULL;
    }

    if (s_lock_sys_info != NULL) {
        duer_mutex_destroy(s_lock_sys_info);
        s_lock_sys_info = NULL;
    }

    ret = duer_uninit_system_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: uninit system info failed ret: %d", ret);
    }

    return ret;
}
