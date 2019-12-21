
#include "lightduer_statistics.h"
#include "lightduer_system_info.h"
#include <stdint.h>
#include <mbed_stats.h>
#include <string.h>
#include "lightduer_mutex.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_dev_info.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_timestamp.h"
#include "lightduer_system_info.h"
#include "WiFiStackInterface.h"

static const uint32_t HEAP_SIZE = 160; // kB

#ifdef MBED_HEAP_STATS_ENABLED
static const uint32_t TIMER_DELAY  = 5 * 1000;
volatile static uint32_t s_peak = 0;
volatile static uint32_t s_trough = (~0x0);
volatile static uint32_t s_sum = 0;
volatile static uint32_t s_counts = 0;
volatile static duer_mutex_t s_lock_sys_info = NULL;
#endif

extern void *baidu_get_netstack_instance(void);

#ifndef CUSTOM_OS_VERSION
#warning "You must specify the OS version"
#define CUSTOM_OS_VERSION           "mbed-OS 5.1.5"
#endif

#ifndef CUSTOM_SW_VERSION
#warning "You must specify the Software version"
#define CUSTOM_SW_VERSION           "demo 2.0.0"
#endif

#ifndef CUSTOM_BRAND
#warning "You must specify the Brand"
#define CUSTOM_BRAND                "Baidu"
#endif

#ifndef CUSTOM_MODEL
#warning "You must specify the Device model"
#define CUSTOM_MODEL                "RDA5981"
#endif

#ifndef CUSTOM_HW_VERSION
#warning "You must specify the Hardware version"
#define CUSTOM_HW_VERSION           "U04"
#endif

#ifndef CUSTOM_RAM_SIZE
#warning "You must specify the RAM size"
#define CUSTOM_RAM_SIZE             288
#endif

#ifndef CUSTOM_ROM_SIZE
#warning "You must specify the ROM size"
#define CUSTOM_ROM_SIZE             (1 * 1024)
#endif

const static duer_system_static_info_t s_system_static_info = {
    .os_version       = CUSTOM_OS_VERSION,
    .sw_version       = CUSTOM_SW_VERSION,
    .brand            = CUSTOM_BRAND,
    .equipment_type   = CUSTOM_MODEL,
    .hardware_version = CUSTOM_HW_VERSION,
    .ram_size         = CUSTOM_RAM_SIZE,
    .rom_size         = CUSTOM_ROM_SIZE,
};

#ifdef MBED_HEAP_STATS_ENABLED
static void calculate_system_info(void *arg)
{
    uint32_t current_size = 0;
    mbed_stats_heap_t stats;

    mbed_stats_heap_get(&stats);
    current_size = HEAP_SIZE - stats.current_size / 1024;

    duer_mutex_lock(s_lock_sys_info);

    if (current_size > s_peak) {
        s_peak = current_size;
    }

    if (current_size < s_trough) {
        s_trough = current_size;
    }

    s_sum += current_size;
    s_counts++;

    duer_mutex_unlock(s_lock_sys_info);
}
#endif // MBED_HEAP_STATS_ENABLED

static int duer_init_system_info_module(void)
{
#ifdef MBED_HEAP_STATS_ENABLED
    int ret = 0;
    duer_timer_handler timer;

    if (s_lock_sys_info == NULL) {
        s_lock_sys_info = duer_mutex_create();
        if (s_lock_sys_info == NULL) {
            DUER_LOGE("Create lock failed");

            return DUER_ERR_FAILED;
        }
    }

    calculate_system_info(NULL);

    timer = duer_timer_acquire(calculate_system_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(timer, TIMER_DELAY);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        return DUER_ERR_FAILED;
    }
#endif // MBED_HEAP_STATS_ENABLED

    return DUER_OK;
}

static int duer_get_system_static_info(duer_system_static_info_t *info)
{
    int ret = DUER_OK;

    if (info == NULL) {
        DUER_LOGE("Sys Info: Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;

        goto out;
    }

    memcpy(info, &s_system_static_info, sizeof(*info));
out:
    return ret;
}

static int duer_get_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    info->system_up_time = duer_timestamp();

    return DUER_OK;
}

static int duer_free_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    return DUER_OK;
}

static int duer_get_memory_info(duer_memory_info_t *mem_info)
{
#ifdef MBED_HEAP_STATS_ENABLED
    //mem_info->memory_type = RAM;
    //mem_info->total_memory_size = HEAP_SIZE;

    duer_mutex_lock(s_lock_sys_info);
    mem_info->peak = s_peak;
    mem_info->trough = s_trough;
    if (s_counts > 0) {
        mem_info->average = s_sum / s_counts;
    }

    s_peak = 0;
    s_trough = (~0x0);
    s_sum = 0;
    s_counts = 0;

    duer_mutex_unlock(s_lock_sys_info);

    return DUER_OK;
#else

    return DUER_ERR_FAILED;
#endif // MBED_HEAP_STATS_ENABLED
}

static int duer_get_disk_info(duer_disk_info_t **disk_info)
{
    duer_disk_info_t *info = NULL;

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
    if (disk_info != NULL) {
        DUER_FREE(disk_info);
    }

    return DUER_OK;
}

static int get_network_info(duer_network_info_t *network_info)
{
    WiFiStackInterface *net_stack = NULL;
    const char *bssid = NULL;

    net_stack = (WiFiStackInterface *)baidu_get_netstack_instance();
    if (net_stack == NULL) {
        DUER_LOGE("Get netstack instance failed");

        return DUER_ERR_FAILED;
    }

    net_stack->update_rssi();
    network_info->wireless.link = net_stack->get_rssi();

    network_info->network_type = WIFI_TYPE;
    strncpy(network_info->hw_address, net_stack->get_mac_address(), HW_ADDRESS_LEN);
    strncpy(network_info->ipv4_address, net_stack->get_ip_address(), IPV4_ADDRESS_LEN);

    bssid = net_stack->get_BSSID();
    if (bssid == NULL) {
        DUER_LOGE("bssid is NULL");
        return DUER_ERR_FAILED;
    }

    snprintf(network_info->wireless.bssid, BSSID_LEN + 1, "%02x:%02x:%02x:%02x:%02x:%02x",
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    snprintf(network_info->wireless.ssid, SSID_LEN + 1, net_stack->get_ssid());

    return DUER_OK;
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

        goto out;
    }

    ret = duer_init_system_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Sys Info: Init system info failed ret: %d", ret);

        goto out;
    }

out:
    return ret;
}
