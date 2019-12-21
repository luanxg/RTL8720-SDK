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

#include <stdint.h>

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_SYSTEM_INFO_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_SYSTEM_INFO_H

#define OS_VERSION_LEN          20
#define DUER_OS_VERSION_LEN     5
#define SDK_VERSION_LEN         5
#define BRAND_LEN               20
#define EQUIPMENT_TYPE_LEN      20
#define HARDWARE_VERSION_LEN    5

#define PARTITION_NAME_LEN      10
#define MOUNT_INFO_LEN          20
#define UNILIZATION_LEN         5

#define NETWORK_CARD_NAME_LEN   10
#define HW_ADDRESS_LEN          17
#define IPV4_ADDRESS_LEN        32
#define BCAST_LEN               32
#define MASK_LEN                32
#define IPV6_ADDRESS_LEN        128
#define LINK_ENACP_LEN          10


#ifdef __cplusplus
extern "C" {
#endif

typedef struct SystemStaticInfo {
    char os_version[OS_VERSION_LEN + 1];
    char duer_os_version[DUER_OS_VERSION_LEN + 1];
    char sdk_version[SDK_VERSION_LEN + 1];
    char brand[BRAND_LEN + 1];
    char equipment_type[EQUIPMENT_TYPE_LEN + 1];
    char hardware_version[HARDWARE_VERSION_LEN + 1];
    uint32_t ram_size;                                 // Unit (KB)
    uint32_t rom_size;                                 // Unit (KB)
} duer_system_static_info_t;

typedef struct SystemDynamicInfo {
    uint32_t running_progresses;
    uint32_t total_progresses;
    uint64_t system_up_time;                           // Unit (Second)
    float system_load_average[3];
} duer_system_dynamic_info_t;

typedef struct MemoryInfo {
    uint64_t total_memory_size;                        // Unit (KB)
    uint64_t available_memory_size;                    // Unit (KB)
    uint64_t shared_memory_size;                       // Unit (KB)
    uint64_t buffer_memory_size;                       // Unit (KB)
    uint64_t swap_size;                                // Unit (KB)
    uint64_t free_swap_size;                           // Unit (KB)
    uint64_t peak;                                     // Within 5 minutes Unit (KB)
    uint64_t trough;                                   // Within 5 minutes Unit (KB)
    uint64_t average;                                  // Within 5 minutes Unit (KB)
} duer_memory_info_t;

typedef struct DiskInfo {
    char partition_name[PARTITION_NAME_LEN + 1];
    char unilization[UNILIZATION_LEN + 1];             // percentage (50%)
    char mount_info[MOUNT_INFO_LEN + 1];
    uint64_t total_size;                               // Unit (KB)
    uint64_t used_size;                                // Unit (KB)
    uint64_t available_size;                           // Unit (KB)
    struct DiskInfo *next;
} duer_disk_info_t;

typedef struct WirelessNetworkQuality {
    int8_t link;                                       // General quality of the reception
    int32_t level;                                     // Signal strength at the receiver
    int32_t noise;                                     // Silence level (no packet) at the receiver
} duer_wireless_network_quality_t;

typedef struct NetworkCardInfo {
    char network_card_name[NETWORK_CARD_NAME_LEN + 1];
    char link_encap[LINK_ENACP_LEN + 1];
    char hw_address[HW_ADDRESS_LEN + 1];
    char ipv4_address[IPV4_ADDRESS_LEN + 1];
    char bcast[BCAST_LEN + 1];
    char mask[MASK_LEN + 1];
    char ipv6_address[IPV6_ADDRESS_LEN + 1];
    duer_wireless_network_quality_t *quality;
    uint32_t mtu;
    uint64_t rx_packet;
    uint64_t rx_error_packet;
    uint64_t rx_dropped_packet;
    uint64_t rx_overruns;
    uint64_t rx_frame;
    uint64_t rx_average_speed;                          // Within 5 minutes Unit (bytes/s)
    uint64_t tx_packet;
    uint64_t tx_error_packet;
    uint64_t tx_dropped_packet;
    uint64_t tx_overruns;
    uint64_t tx_average_speed;                          // Within 5 minutes Unit (bytes/s)
    struct NetworkCardInfo *next;
} duer_network_card_info_t;

typedef struct SystemInfoOPS {
    int (*init)(void);
    int (*get_system_static_info)(duer_system_static_info_t *info);
    int (*get_system_dynamic_info)(duer_system_dynamic_info_t *info);
    int (*get_memory_info)(duer_memory_info_t *mem_info);
    int (*get_disk_info)(duer_disk_info_t **disk_info);
    int (*free_disk_info)(duer_disk_info_t *disk_info);
    int (*get_network_info)(duer_network_card_info_t **network_info);
    int (*free_network_info)(duer_network_card_info_t *network_info);
} duer_system_info_ops_t;

extern int duer_system_info_init(void);

extern int duer_system_info_register_system_info_ops(duer_system_info_ops_t *info_ops);

extern int duer_system_info_get_static_system_info(duer_system_static_info_t *info);

extern int duer_system_info_get_dynamic_system_info(duer_system_dynamic_info_t *info);

extern int duer_system_info_get_mem_info(duer_memory_info_t *info);

extern int duer_system_info_get_disk_info(duer_disk_info_t **info);

extern int duer_system_info_free_disk_info(duer_disk_info_t *info);

extern int duer_system_info_get_network_info(duer_network_card_info_t **info);

extern int duer_system_info_free_network_info(duer_network_card_info_t *info);

extern void duer_system_info_show_system_static_info(duer_system_static_info_t const *info);

extern void duer_system_info_show_system_dynamic_info(duer_system_dynamic_info_t const *info);

extern void duer_system_info_show_memory_info(duer_memory_info_t const *memory_info);

extern void duer_system_info_show_disk_info(duer_disk_info_t const *disk_info);

extern void duer_system_info_show_networkcard_info(duer_network_card_info_t const *network_info);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_SYSTEM_INFO_H
