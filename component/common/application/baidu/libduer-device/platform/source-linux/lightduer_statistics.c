/* Copyright (2017) Baidu Inc. All rights reserved.
 *
 * File: duerapp_system_info.c
 * Auth: Zhong Shuai(zhongshuai@baidu.com)
 * Desc: Provide the information about the system
 */

#include "lightduer_statistics.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "lightduer_system_info.h"
#include "lightduer_mutex.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"

#ifdef ANDROID
// ensure android app has permission to read and write sdcard
#define TEMP_FILE "/sdcard/temp_log"
#else
#define TEMP_FILE "/tmp/temp_log"
#endif // ANDROID

#define DELAY_TIEM (5 * 1000)

extern int duer_system_info_init(void);

volatile static uint32_t s_peak = 0;
volatile static uint32_t s_trough = (~0x0);
volatile static uint32_t s_sum = 0;
volatile static uint32_t s_counts = 0;
volatile static duer_mutex_t s_lock_sys_info = NULL;

extern duer_system_static_info_t g_system_static_info;

static void calculate_system_info(void *arg)
{
    int fd = 0;
    int ret = DUER_OK;
    static int count = 1;
    const int BUF_LEN = 300;
    char buf[BUF_LEN + 1];
    char used_memory_str[10];
    uint64_t used_memory = 0;

    memset(used_memory_str, 0, sizeof(used_memory_str));

    ret = system("free > "TEMP_FILE);
    if (ret < 0) {
        DUER_LOGE("Get Memory info failed ret: %d", ret);
        return;
    }

    fd = open(TEMP_FILE, O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open "TEMP_FILE" failed, ret = %d", ret);
        return;
    }

    do {
        count = read(fd, buf, BUF_LEN);
        if (count < 0) {
            DUER_LOGE("Read "TEMP_FILE" failed ret: %d", count);
            break;
        }

        buf[BUF_LEN] = '\0';

        sscanf(buf, "%*[^:]:%*s %s %*s %*s %*s", used_memory_str);

        used_memory = atoll(used_memory_str);

        ret = duer_mutex_lock(s_lock_sys_info);
        if (ret != DUER_OK) {
            DUER_LOGE("Lock sys info failed");
            break;
        }

        if (used_memory > s_peak) {
            s_peak = used_memory;
        }

        if (used_memory < s_trough) {
            s_trough = used_memory;
        }

        s_sum += used_memory;
        s_counts++;

        ret = duer_mutex_unlock(s_lock_sys_info);
        if (ret != DUER_OK) {
            DUER_LOGE("Unlock sys info failed");
        }
    } while (0);

    close(fd);

    return;
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

#ifndef ANDROID
    timer = duer_timer_acquire(calculate_system_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(timer, DELAY_TIEM);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        ret = DUER_ERR_FAILED;
    }
#endif

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

static int get_sys_time(uint64_t *time)
{
    int i = 0;
    int fd = -1;
    int count = 0;
    int ret = DUER_OK;
    const int BUF_LEN = 15;
    char buf[BUF_LEN];
    char sys_time[BUF_LEN];

    memset(buf, 0, sizeof(buf));

    do {
        fd = open("/proc/uptime", O_RDONLY);
        if (fd == -1) {
            DUER_LOGE("Open /proc/uptime failed");
            ret = DUER_ERR_FAILED;
            break;
        }

        count = read(fd, sys_time, BUF_LEN - 1);
        if (count < 0) {
            DUER_LOGE("Read /proc/uptime failed ret: %d", count);
            ret = DUER_ERR_FAILED;
            break;
        }

        sys_time[BUF_LEN - 1] = '\0';

        for (i = 0; i < BUF_LEN; i++) {
            if (sys_time[i] != '.') {
                buf[i] = sys_time[i];
            } else {
                break;
            }
        }

        *time = atoll(buf);
    } while (0);

    if (fd != -1) {
        close(fd);
    }

    return ret;
}

static int duer_get_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    int j = 0;
    int i = 0;
    int k = 0;
    int fd = -1;
    int count = 0;
    int progress[2] = {0, 0};
    int ret = DUER_OK;
    const int BUF_LEN = 50;
    char buf[BUF_LEN];
    char num[BUF_LEN];

    memset(buf, 0, BUF_LEN);
    memset(num, 0, BUF_LEN);

    do {
        ret = get_sys_time(&info->system_up_time);
        if (ret < 0) {
            DUER_LOGE("Get system up time failed");
            ret = DUER_ERR_FAILED;
            break;
        }

        fd = open("/proc/loadavg", O_RDONLY);
        if (fd == -1) {
            DUER_LOGE("Open /proc/loadavg failed");
            ret = DUER_ERR_FAILED;
            break;
        }

        count = read(fd, buf, BUF_LEN);
        if (count <= 0) {
            DUER_LOGE("Read /proc/loadavg failed ret: %d", count);
            ret = DUER_ERR_FAILED;
            break;
        }

        info->system_load = (char *)DUER_MALLOC(count);
        if (info->system_load == NULL) {
            DUER_LOGE("Malloc failed");
            break;
        }

        memcpy(info->system_load, buf, count - 1);
        info->system_load_len = count - 1;

        for (i = 0; i < BUF_LEN; i++) {
            if (buf[i] == ' ') {
                j++;
                if (j >= 3) {
                    break;
                }
            }
        }

        j = 0;
        i++;
        while (i < BUF_LEN) {
            if (buf[i] != '/' && buf[i] != ' ') {
                num[k++] = buf[i++];
            } else {
                progress[j++] = atoi(num);
                memset(num, 0, BUF_LEN);
                i++;
                k = 0;
                if (j >= 2) {
                    break;
                }
            }
        }

        info->running_task = progress[0];
        info->total_task = progress[1];
    } while (0);

    if (fd != -1) {
        close(fd);
    }

    return ret;
}

static int duer_free_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    DUER_FREE(info->system_load);
    info->system_load = NULL;

    return DUER_OK;
}

static int duer_get_memory_info(duer_memory_info_t *mem_info)
{
    int fd = 0;
    int count = 0;
    int ret = DUER_OK;
    const int BUF_LEN = 300;
    char buf[BUF_LEN + 1];
    char *swap_info = NULL;
    char swap_size_str[10];
    char total_memory_str[10];
    char shared_memory_str[10];
    char buffer_memory_str[10];
    char free_swap_size_str[10];
    char available_memory_str[10];

    memset(total_memory_str, 0, sizeof(total_memory_str));
    memset(available_memory_str, 0, sizeof(available_memory_str));
    memset(shared_memory_str, 0, sizeof(shared_memory_str));
    memset(buffer_memory_str, 0, sizeof(buffer_memory_str));
    memset(swap_size_str, 0, sizeof(swap_size_str));
    memset(free_swap_size_str, 0, sizeof(free_swap_size_str));

    ret = system("free > "TEMP_FILE);
    if (ret < 0) {
        DUER_LOGE("Get Memory info failed ret: %d", ret);
        return DUER_ERR_FAILED;
    }

    fd = open(TEMP_FILE, O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open "TEMP_FILE" failed");
        return DUER_ERR_FAILED;
    }

    count = read(fd, buf, BUF_LEN);
    close(fd);
    if (count < 0) {
        DUER_LOGE("Read "TEMP_FILE" failed ret: %d", count);
        return DUER_ERR_FAILED;
    }

    buf[BUF_LEN] = '\0';

    sscanf(buf, "%*[^:]:%s %*s %s %s %s",
            total_memory_str,
            available_memory_str,
            shared_memory_str,
            buffer_memory_str);

    mem_info->memory_type = RAM;
    mem_info->total_memory_size = atoll(total_memory_str);
    mem_info->available_memory_size = atoll(available_memory_str);
    mem_info->shared_memory_size = atoll(shared_memory_str);
    mem_info->buffer_memory_size = atoll(buffer_memory_str);

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

    swap_info = strstr(buf, "Swap");
    if (swap_info == NULL) {
        DUER_LOGE("Can not find swap info");
        return DUER_ERR_FAILED;
    }

    sscanf(swap_info, "%*[^:]:%s %*s %s", swap_size_str, free_swap_size_str);

    mem_info->swap_size = atoll(swap_size_str);
    mem_info->free_swap_size = atoll(free_swap_size_str);

    return DUER_OK;
}

static int duer_get_disk_info(duer_disk_info_t **disk_info)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int fd = 0;
    int count = 0;
    int skip_line = 0;
    int ret = DUER_OK;
    const int BUF_LEN = 2000;
    const int LINE_LEN = 201;
    char *buf = NULL;
    char line_str[LINE_LEN];
    char used_size_str[10];
    char total_size_str[10];
    char unilization_str[10];
    char available_size_str[10];
    char mount_info_str[MOUNT_INFO_LEN + 1];
    char partition_name[100];
    duer_disk_info_t *item = NULL;
    duer_disk_info_t *next = NULL;
    duer_disk_info_t *head = NULL;
    duer_disk_info_t *previous = NULL;
    duer_disk_info_t *partition = NULL;

    memset(line_str, 0, sizeof(line_str));
    memset(partition_name, 0, sizeof(partition_name));
    memset(total_size_str, 0, sizeof(total_size_str));
    memset(used_size_str, 0, sizeof(used_size_str));
    memset(available_size_str, 0, sizeof(available_size_str));
    memset(unilization_str, 0, sizeof(unilization_str));
    memset(mount_info_str, 0, MOUNT_INFO_LEN + 1);

    ret = system("df > "TEMP_FILE);
    if (ret < 0) {
        DUER_LOGE("Get disk info failed ret: %d", ret);
        return DUER_ERR_FAILED;
    }

    buf = DUER_MALLOC(BUF_LEN + 1);
    if (buf == NULL) {
        DUER_LOGE("Malloc buf failed");
        return DUER_ERR_FAILED;
    }

    memset(buf, 0, BUF_LEN + 1);

    ret = DUER_OK;

    do {
        fd = open(TEMP_FILE, O_RDONLY);
        if (fd < 0) {
            DUER_LOGE("Open "TEMP_FILE" failed ret: %d", ret);
            ret = DUER_ERR_FAILED;
            break;
        }

        count = read(fd, buf, BUF_LEN);
        close(fd);
        if (count < 0) {
            DUER_LOGE("Read "TEMP_FILE" failed ret: %d", ret);
            ret = DUER_ERR_FAILED;
            break;
        }

        for (i = 0, j = 0, k = 0; buf[i] != '\0'; i++) {
            if (buf[i] != '\n') {
                if (j < LINE_LEN) {
                    line_str[j++] = buf[i];
                } else {
                    skip_line = 1;
                    DUER_LOGW("line %d is too long", k);
                }
            } else {
                j = 0;

                if (k++ < 1 || skip_line) {
                    skip_line = 0;
                    continue;
                }

                partition = DUER_MALLOC(sizeof(*partition));
                if (partition == NULL) {
                    DUER_LOGE("Malloc disk fialed");

                    item = head;
                    while (item) {
                        next = item->next;
                        DUER_FREE(item);
                        item = next;
                    }

                    ret = DUER_ERR_FAILED;
                    break;
                }

                memset(partition, 0, sizeof(*partition));

                if (k == 2) {
                    head = partition;
                    previous = partition;
                } else {
                    previous->next = partition;
                    previous = partition;
                }

                sscanf(line_str, "%s %s %s %s %s%% %s",
                       partition_name,
                       total_size_str,
                       used_size_str,
                       available_size_str,
                       unilization_str,
                       mount_info_str);

                DUER_LOGI("partition_name = %s", partition_name);
                memcpy(partition->partition_name, partition_name, PARTITION_NAME_LEN);
                memcpy(partition->mount_info, mount_info_str, MOUNT_INFO_LEN);
                partition->unilization = atoi(unilization_str);
                partition->total_size = atoll(total_size_str);
                partition->used_size = atoll(used_size_str);
                partition->available_size = atoll(available_size_str);
                partition->next = NULL;

                memset(line_str, 0, sizeof(line_str));
                memset(partition_name, 0, PARTITION_NAME_LEN + 1);
                memset(total_size_str, 0, sizeof(total_size_str));
                memset(used_size_str, 0, sizeof(used_size_str));
                memset(available_size_str, 0, sizeof(available_size_str));
                memset(unilization_str, 0, sizeof(unilization_str));
                memset(mount_info_str, 0, MOUNT_INFO_LEN + 1);
            }
        }

        *disk_info = head;
    } while (0);

    if (buf != NULL) {
        DUER_FREE(buf);
        buf = NULL;
    }

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

static int get_str_num(char const *str, char const *needle, uint64_t *num)
{
    char num_str[20];
    char *search_ret = NULL;

    memset(num_str, 0, sizeof(num_str));
    search_ret = strstr(str, needle);
    if (search_ret == NULL) {

        return DUER_ERR_FAILED;
    }

    sscanf(search_ret + strlen(needle), "%s", num_str);
    *num = atoll(num_str);

    return DUER_OK;
}

static int get_network_info(duer_network_info_t *network_info)
{
    int i = 0;
    int j = 0;
    int fd_net_info = -1;
    int fd_wireless_info = 0;
    int count = 0;
    int ret = DUER_OK;
    int skip = -1;
    int len = 0;
    char num_str[20];
    char link_str[10];
    char line_str[121];
    char level_str[10];
    char noise_str[10];
    char w_netdev_buf[301];
    const int BUF_LEN = 2000;
    char *buf = NULL;
    char *search_ret = NULL;

    memset(line_str, 0, sizeof(line_str));
    memset(network_info, 0, sizeof(*network_info));

    ret = system("ifconfig > "TEMP_FILE);
    if (ret < 0) {
        DUER_LOGE("Get network info failed ret: %d", ret);
        return DUER_ERR_FAILED;
    }

    buf = DUER_MALLOC(BUF_LEN + 1);
    if (buf == NULL) {
        DUER_LOGE("Malloc buf failed");
        return DUER_ERR_FAILED;
    }

    memset(buf, 0, BUF_LEN + 1);

    do {
        fd_net_info = open(TEMP_FILE, O_RDONLY);
        if (fd_net_info == -1) {
            DUER_LOGE("Open "TEMP_FILE" failed ret: %d", ret);
            ret = DUER_ERR_FAILED;
            break;
        }

        count = read(fd_net_info, buf, BUF_LEN);
        if (count < 0) {
            DUER_LOGE("Read "TEMP_FILE" failed ret: %d", count);
            ret = DUER_ERR_FAILED;
            break;
        }

        for (i = 0, j = 0; buf[i] != '\0'; i++) {
            if (buf[i] != '\n') {
                line_str[j++] = buf[i];
            } else {
                j = 0;

                search_ret = strstr(line_str, "Link encap");
                if (search_ret != NULL) {
                    if (skip == 0) {
                        // if skip == 0, one networkinfo has been parsed
                        break;
                    }

                    search_ret = strstr(line_str, "lo");
                    if (search_ret != NULL) {
                        skip = 1;
                        memset(line_str, 0, 121);
                        continue;
                    } else {
                        skip = 0;
                    }

                    search_ret = strstr(line_str, "wlan");
                    if (search_ret != NULL) {
                        network_info->network_type = WIFI_TYPE;
                        fd_wireless_info = open("/proc/net/wireless", O_RDONLY);
                        if (fd_wireless_info < 0) {
                            DUER_LOGE("Open /proc/net/wireless failed");
                            ret = DUER_ERR_FAILED;
                            break;
                        }

                        memset(w_netdev_buf, 0, sizeof(w_netdev_buf));

                        count = read(fd_wireless_info, w_netdev_buf, sizeof(w_netdev_buf) - 1);
                        close(fd_wireless_info);
                        if (count < 0) {
                            DUER_LOGE("Read /proc/net/wireless failed");
                            ret = DUER_ERR_FAILED;
                            break;
                        }

                        search_ret = strstr(w_netdev_buf, "wlan");
                        if (search_ret != NULL) {
                            memset(link_str, 0, sizeof(link_str));
                            memset(level_str, 0, sizeof(level_str));
                            memset(noise_str, 0, sizeof(noise_str));

                            sscanf(search_ret, "%*[^:]:%*s %s %s %s",
                                   link_str,
                                   level_str,
                                   noise_str);
                            len = strlen(link_str);
                            if (len > 0 && link_str[len - 1] == '.') {
                                link_str[len - 1] = 0;
                            }
                            len = strlen(level_str);
                            if (len > 0 && level_str[len - 1] == '.') {
                                level_str[len - 1] = 0;
                            }
                            len = strlen(noise_str);
                            if (len > 0 && noise_str[len - 1] == '.') {
                                noise_str[len - 1] = 0;
                            }

                            network_info->wireless.link = atoi(link_str);
                            network_info->wireless.level = atoi(level_str);
                            network_info->wireless.noise = atoi(noise_str);
                        }
                    } else {
                        // TODO: maybe other type
                        network_info->network_type = ETHERNET_TYPE;
                    }

                    search_ret = strstr(line_str, "HWaddr");
                    if (search_ret != NULL) {
                        sscanf(search_ret, "%*s %[a-zA-Z0-9:]", network_info->hw_address);
                    }
                }

                if (skip) {
                    memset(line_str, 0, 121);
                    continue;
                }

                search_ret = strstr(line_str, "inet addr");
                if (search_ret != NULL) {
                    search_ret = strstr(line_str, ":");
                    if (search_ret != NULL) {
                        sscanf(search_ret + 1, "%s", network_info->ipv4_address);
                    }
                }

                search_ret = strstr(line_str, "inet6 addr");
                if (search_ret != NULL) {
                    sscanf(search_ret + 12, "%s", network_info->ipv6_address);
                }

                search_ret = strstr(line_str, "MTU");
                if (search_ret != NULL) {
                    memset(num_str, 0, 20);
                    sscanf(search_ret + 4, "%s", num_str);
                    network_info->mtu = atoi(num_str);
                }

                search_ret = strstr(line_str, "RX packets:");
                if (search_ret != NULL) {
                    get_str_num(line_str, "RX packets:", &network_info->received_packets);
                    // get_str_num(line_str, "errors:", &network_info->rx_error_packet);
                    // get_str_num(line_str, "dropped:", &network_info->rx_dropped_packet);
                    // get_str_num(line_str, "overruns:", &network_info->rx_overruns);
                    // get_str_num(line_str, "frame:", &network_info->rx_frame);
                }

                search_ret = strstr(line_str, "TX packets:");
                if (search_ret != NULL) {
                    get_str_num(line_str, "TX packets:", &network_info->transmitted_packets);
                    // get_str_num(line_str, "errors:", &network_info->tx_error_packet);
                    // get_str_num(line_str, "dropped:", &network_info->tx_dropped_packet);
                    // get_str_num(line_str, "overruns:", &network_info->tx_overruns);
                }

                memset(line_str, 0, sizeof(line_str));
            }
        }
    } while (0);

    if (buf != NULL) {
        DUER_FREE(buf);
        buf = NULL;
    }

    if (fd_net_info != -1) {
        close(fd_net_info);
    }

    return ret;
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
