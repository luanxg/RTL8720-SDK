[TOC]

#系统通用信息采集模块介绍

系统通用信息模块主要用来采集系统的任务，内存，磁盘，网络信息。

##系统通用信息结构体信息介绍
1.  系统静态信息

```
typedef struct _system_static_info_s {
    char os_version[OS_VERSION_LEN + 1];              // 操作系统版本号
    char sw_version[SW_VERSION_LEN + 1];              // 用户软件开发版本号
    char brand[BRAND_LEN + 1];                        // 品牌
    char equipment_type[EQUIPMENT_TYPE_LEN + 1];      // 设备型号
    char hardware_version[HARDWARE_VERSION_LEN + 1];  // 硬件版本号
    uint32_t ram_size;                                // RAM大小 (KB)
    uint32_t rom_size;                                // ROM大小 (KB)
} duer_system_static_info_t;
```

2.  系统动态信息

```
typedef struct _task_info_s {
    char task_name[TASK_NAME_LEN];          // 任务名称
    int8_t cpu_occupancy_rate;              // CPU占用率（如果是负数，不上报云端）
    uint32_t stack_size;                    // 任务栈大小 (KB)
    uint32_t stack_free_size;               // 任务可用栈大小 (KB)
    int32_t stack_max_used_size;            // 任务使用的最大栈值（如果是负数不上报）
    struct _task_info_s *next;              // 下一个任务指针
} duer_task_info_t;

typedef struct _system_dynamic_info_s {
    uint64_t system_up_time;                // 系统启动时长 (单位 系统心跳)
    uint32_t running_task;                  // 正在运行的任务数量
    uint32_t total_task;                    // 系统总共的任务数量
    duer_task_info_t *task_info;            // 任务信息头指针
    char *system_load;                      // 当前系统负荷
    size_t system_load_len;                 // 系统负荷字符串长度
    char *system_crash_info;                // 系统Crash信息
    size_t system_crash_info_len;           // 系统Crash信息长度
} duer_system_dynamic_info_t;
```

3. 系统内存信息

```
typedef enum _memory_type {
    RAM   = 1,                  // 内存类型
    PSRAM = 2,
} duer_memory_type_t;

typedef struct _memory_info {
    duer_memory_type_t memory_type;          // 内存类型
    uint64_t total_memory_size;              // 内存总大小   (KB)
    uint64_t available_memory_size;          // 可用内存大小 (KB)
    int64_t shared_memory_size;              // 共享内存大小 (KB)
    int64_t buffer_memory_size;              // 缓冲内存大小 (KB)
    int64_t swap_size;                       // 交换分区大小 (KB)
    int64_t free_swap_size;                  // 可用交换分区大小 （KB）
    uint64_t peak;                           // 5分钟内的内存使用峰值 (KB)
    uint64_t trough;                         // 5分钟内的内存使用谷值 (KB)
    uint64_t average;                        // 5分钟内的内存使用平均值 (KB)
} duer_memory_info_t;
```

4. 系统磁盘信息

```
typedef struct _disk_info_t {
    char partition_name[PARTITION_NAME_LEN + 1];    // 分区名称
    char mount_info[MOUNT_INFO_LEN + 1];            // 挂载信息
    uint8_t unilization;                            // 使用率 (50%)
    uint64_t total_size;                            // 分区总大小 (KB)
    uint64_t used_size;                             // 分区已使用大小 (KB)
    uint64_t available_size;                        // 分区可使用大小 (KB)
    struct _disk_info_t *next;                      // 下一个分区信息指针
} duer_disk_info_t;
```

5. 系统网络信息

```
typedef struct _wireless_network_quality_s {
    int8_t link;                                      // 无线网卡链接质量
    int32_t level;                                    // 无线网卡接收强度
    int32_t noise;                                    // 无线网卡静默质量
} duer_wireless_network_quality_t;

typedef enum _network_type {
    ETHERNET_TYPE = 1,                                // 网络类型
} duer_network_type;

// 仅上报用于Duer OS 的网络信息
typedef struct _network_info_s {
    duer_network_type network_type;                   // 网络链接类型
    char hw_address[HW_ADDRESS_LEN + 1];              // MAC地址
    char ipv4_address[IPV4_ADDRESS_LEN + 1];          // IPV4地址
    char ipv6_address[IPV6_ADDRESS_LEN + 1];          // IPV6地址
    duer_wireless_network_quality_t *quality;         // 无线网络质量信息指针
    uint32_t mtu;                                     // 网络最大传输单元
    uint64_t transmitted_packets;                     // 网络发送的数据包数量
    uint64_t received_packets;                        // 网络接收的数据包数量
    uint64_t forwarded_packets;                       // 网络转发的数据包数量
    uint64_t dropped_packets;                         // 网络丢弃的数据包数量
    uint64_t checksum_error;                          // 网络教研失败的数据包数量
    uint64_t invalid_length_error;                    // 网络无效长度数据包数量
    uint64_t routing_error;                           // 网络路由错误数据包数量
    uint64_t protocol_error;                          // 网络协议错误数据包数量
    uint64_t tx_average_speed;                        // 5分钟内的网络发送速度(bytes/s)
    uint64_t rx_average_speed;                        // 5分钟内的网络接收速度(bytes/s)
} duer_network_info_t;

typedef struct _system_info_ops_t {
    int (*init)(void);                                                 // 初始化函数，用户在这个函数实现全局变量的锁和定时器，用户每隔五分钟采集和计算内存和网络信息
    int (*get_system_static_info)(duer_system_static_info_t *info);    // 用户实现系统静态信息
    int (*get_system_dynamic_info)(duer_system_dynamic_info_t *info);  // 用户实现系统动态信息的采集工作
    int (*free_system_dynamic_info)(duer_system_dynamic_info_t *info); // 释放动态信息采集工作申请的资源
    int (*get_memory_info)(duer_memory_info_t *mem_info);              // 用户获取内存信息
    int (*get_disk_info)(duer_disk_info_t **disk_info);                // 用户获取磁盘信息
    int (*free_disk_info)(duer_disk_info_t *disk_info);                // 用户释放获取磁盘信息所申请的资源
    int (*get_network_info)(duer_network_info_t *network_info);        // 用户获取网络信息
    int (*free_network_info)(duer_network_info_t *network_info);       // 用户释放获取网络资源时申请的资源
} duer_system_info_ops_t;
```

##系统通用信息模块API介绍
函数功能：    初始化系统信息模块
函数参数：    无
函数返回值：  成功：DUER_OK
              失败：Other
int duer_system_info_init(void);

函数功能：   向通用信息模块注册获取信息函数
函数参数：   duer_system_info_ops_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_register_system_info_ops(duer_system_info_ops_t *info_ops);

函数功能：   获取系统静态信息
函数参数：   duer_system_static_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_get_static_system_info(duer_system_static_info_t *info);

函数功能：   获取系统动态信息
函数参数：   duer_system_dynamic_info_t *
函数返回值:  成功：DUER_OK
             失败：Other
int duer_system_info_get_dynamic_system_info(duer_system_dynamic_info_t *info);

函数功能：   释放动态信息资源
函数参数：   duer_system_dynamic_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_free_dynamic_system_info(duer_system_dynamic_info_t *info);

函数功能：   获取系统内存信息
函数参数：   duer_memory_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_get_mem_info(duer_memory_info_t *info);

函数功能：   获取系统磁盘信息
函数参数：   duer_disk_info_t **
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_get_disk_info(duer_disk_info_t **info);

函数功能：   释放磁盘信息资源
函数参数：   duer_disk_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_free_disk_info(duer_disk_info_t *info);

函数功能：   获取系统网络信息
函数参数：   duer_network_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_get_network_info(duer_network_info_t *info);

函数功能：   释放系统网络信息资源
函数参数：   duer_network_info_t *
函数返回值： 成功：DUER_OK
             失败：Other
int duer_system_info_free_network_info(duer_network_info_t *info);

函数功能：   打印系统静态信息
函数参数：   duer_system_static_info_t const *
函数返回值： 无
void duer_system_info_show_system_static_info(duer_system_static_info_t const *info);

函数功能：   打印系统动态信息
函数参数：   duer_system_dynamic_info_t const *
函数返回值： 无
void duer_system_info_show_system_dynamic_info(duer_system_dynamic_info_t const *info);

函数功能：   打印系统内存信息
函数参数：   duer_memory_info_t const *
函数返回值： 无
void duer_system_info_show_memory_info(duer_memory_info_t const *memory_info);

函数功能：   打印系统磁盘信息
函数参数：   duer_disk_info_t const *
函数返回值： 无
void duer_system_info_show_disk_info(duer_disk_info_t const *disk_info);

函数功能：   打印系统网络信息
函数参数：   duer_network_info_t const *
函数返回值： 无
void duer_system_info_show_networkcard_info(duer_network_info_t const *network_info);

##Demo Code

```
#include "lightduer_system_info.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "lightduer_mutex.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_dev_info.h"
#include "lightduer_audio_log.h"
#include "lightduer_audio_memory.h"

#define DELAY_TIEM (5 * 1000)

volatile static uint32_t peak = 0;
volatile static uint32_t trough = (~0x0);
volatile static uint32_t average = 0;
volatile static duer_mutex_t s_lock_sys_info = NULL;

static duer_system_static_info_t s_system_static_info = {
    .os_version       = "FreeRTOS V8.2.0",
    .duer_os_version  = "Lightduer OS x.x.x",
    .brand            = "Baidu",
    .equipment_type   = "X-Man",
    .hardware_version = "x.x.x",
    .ram_size         = 4431,
    .rom_size         = (4 * 1024),
    .ota_info = {
        .developer_name   = "Allen",
        .firmware_version = "1.1.1.1"
    },
};

static void calculate_system_info(void *arg)
{
    int ret = DUER_OK;
    static int count = 1;
    uint32_t free_heap_size = 0;
    uint32_t used_heap_size = 0;

    free_heap_size = system_get_free_heap_size();
    used_heap_size = (s_system_static_info.ram_size) - (free_heap_size / 1024);

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    if ((count++ % 100) == 0) {
        peak = 0;
        trough = (~0x0);
    }

    if (used_heap_size > peak) {
        peak = used_heap_size;
    }

    if (used_heap_size < trough) {
        trough = used_heap_size;
    }

    average = (peak + trough) / 2;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");
    }

out:
    return ret;
}

static int duer_init_system_info_module(void)
{
    int ret = 0;
    duer_timer_handler timer;

    if (s_lock_sys_info == NULL) {
        s_lock_sys_info = duer_mutex_create();
        if (s_lock_sys_info == NULL) {
            DUER_LOGE("Create lock failed");

            return DUER_ERR_FAILED;
        }
    }

    timer = duer_timer_acquire(calculate_system_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(timer, DELAY_TIEM);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        ret = DUER_ERR_FAILED;
    }

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
    uint64_t run_time = 0;
    UBaseType_t num_of_tasks = 0;
    UBaseType_t task_array_size = 0;
    TaskStatus_t *task_status_array = NULL;

    num_of_tasks = uxTaskGetNumberOfTasks();

    task_status_array = DUER_MALLOC(num_of_tasks * sizeof(TaskStatus_t));

    task_array_size = uxTaskGetSystemState(task_status_array, num_of_tasks, &run_time);

    info->system_up_time = run_time;
    info->total_task     = num_of_tasks;
    info->running_task   = task_array_size;

    DUER_FREE(task_status_array);

    return DUER_OK;
}

static int duer_free_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    return DUER_OK;
}

static int duer_get_memory_info(duer_memory_info_t *mem_info)
{
    int ret = DUER_OK;
    uint32_t free_heap_size = 0;

    free_heap_size = system_get_free_heap_size();

    mem_info->memory_type = RAM;
    mem_info->total_memory_size = s_system_static_info.ram_size;
    mem_info->available_memory_size = free_heap_size / 1024;

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");

        goto out;
    }

    mem_info->peak = peak;
    mem_info->trough = trough;
    mem_info->average = average;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");
    }

out:
    return ret;
}

static int duer_get_disk_info(duer_disk_info_t **disk_info)
{
    duer_disk_info_t *info = NULL;

    info = DUER_MALLOC(sizeof(*info));
    if (info == NULL) {
        DUER_LOGE("Malloc failed");

        return DUER_ERR_FAILED;
    }

    memset(info, 0, sizeof(*info));

    strncpy(info->partition_name, "Flash", 5);

    info->unilization    = 50;
    info->total_size     = 1024 * 4;
    info->used_size      = 1024 * 2;
    info->available_size = 1024 * 2;
    info->next           = NULL;

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
    network_info->quality = DUER_MALLOC(sizeof(duer_wireless_network_quality_t));
    if (network_info->quality == NULL) {
        DUER_LOGE("Malloc failed");

        return DUER_ERR_FAILED;
    }

    network_info->quality->link  = 33;
    network_info->quality->level = 0;
    network_info->quality->noise = 0;

    network_info->network_type = ETHERNET_TYPE;
    strncpy(network_info->hw_address, "00:0c:29:f6:c8:24", 17);
    strncpy(network_info->ipv4_address, "192.168.111.128", 15);
    network_info->mtu = 1500;
    network_info->transmitted_packets = 1000;
    network_info->received_packets    = 20000;

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

int duer_init_system_info(void)
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
    }
out:
    return ret;
}

```
