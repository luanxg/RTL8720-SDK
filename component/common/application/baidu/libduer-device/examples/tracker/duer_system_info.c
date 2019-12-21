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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_timers.h"

#define DELAY_TIEM (5 * 1000)

volatile static uint64_t peak;
volatile static uint64_t trough = (~0x0);
volatile static uint64_t average;
volatile static duer_mutex_t s_lock_sys_info = NULL;

static duer_system_static_info_t s_sys_static_info = {
    .os_version       = "Linux 4.4.22",
    .duer_os_version  = "1.1.1",
    .sdk_version      = "1.0.0",
    .brand            = "Baidu",
    .equipment_type   = "Golen Eye",
    .hardware_version = "1.1.0",
    .ram_size         = (4096 * 1024),
    .rom_size         = (2028 * 1024),
};

static int get_system_static_info(duer_system_static_info_t *info)
{
    DUER_MEMCPY((void *)info, (void *)&s_sys_static_info, sizeof(*info));

    return DUER_OK;
}

static int get_sys_time(uint64_t *time)
{
    int i = 0;
    int fd = 0;
    int num = 0;
    int count = 0;
    int ret = DUER_OK;
    char buf[15];
    char sys_time[15];

    memset(buf, 0 , 15);

    fd = open("/proc/uptime", O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open /tmp/uptime failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    count = read(fd, sys_time, 14);
    if (count < 0) {
        DUER_LOGE("Read /proc/uptime failed ret: %d", count);

        ret = DUER_ERR_FAILED;

        goto close_fd;
    }

    sys_time[14] = '\0';

    for (i = 0; i < 15; i++) {
        if (sys_time[i] != '.') {
            buf[i] = sys_time[i];
        } else {
            break;
        }
    }

    *time = atoi(buf);

close_fd:
    close(fd);
out:
    return ret;
}

static int get_system_dynamic_info(duer_system_dynamic_info_t *info)
{
    int j = 0;
    int i = 0;
    int k = 0;
    int fd = 0;
    int count = 0;
    int progress[2];
    int ret = DUER_OK;
    char buf[50];
    char num[50];
    double load[3];

    memset(buf, 0 , 50);
    memset(num, 0 , 50);

    ret = get_sys_time(&info->system_up_time);
    if (ret < 0) {
        DUER_LOGE("Get system up time failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    fd = open("/proc/loadavg", O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open /proc/loadavg failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    count = read(fd, buf, 50);
    if (count < 0) {
        DUER_LOGE("Read /proc/loadavg failed ret: %d", count);

        ret = DUER_ERR_FAILED;

        goto close_fd;
    }

    for (i = 0, k = 0; i < 50; i++) {
        if (buf[i] != ' ') {
            num[k++] = buf[i];
        } else {
            info->system_load_average[j] = atof(num);
            k = 0;
            j++;
            if (j >= 3) {
                break;
            }
        }
    }

    memset(num, 0 , 50);

    j = 0;
    k = 0;
    i++;
    while (i < 50) {
        if (buf[i] != '/' && buf[i] != ' ') {
            num[k++] = buf[i++];
        } else {
            progress[j++] = atoi(num);
            memset(num, 0 , 50);
            i++;
            k = 0;
            if (j >= 2) {
                break;
            }
        }
    }

    info->running_progresses = progress[0];
    info->total_progresses   = progress[1];

close_fd:
    close(fd);
out:
    return ret;
}

static int get_memory_info(duer_memory_info_t *mem_info)
{
    int fd = 0;
    int count = 0;
    int ret = DUER_OK;
    char buf[301];
    char *swap_info = NULL;
    char swap_size_str[10];
    char total_memory_str[10];
    char shared_memory_str[10];
    char buffer_memory_str[10];
    char free_swap_size_str[10];
    char available_memory_str[10];

    memset(total_memory_str, 0, 10);
    memset(available_memory_str, 0, 10);
    memset(shared_memory_str, 0, 10);
    memset(buffer_memory_str, 0, 10);
    memset(swap_size_str, 0, 10);
    memset(free_swap_size_str, 0, 10);

    ret = system("free > /tmp/free_log");
    if (ret < 0) {
        DUER_LOGE("Get Memory info failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto out;
    }

    fd = open("/tmp/free_log", O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open /tmp/free_log failed");

        ret = DUER_ERR_FAILED;

        goto out;
    }

    count = read(fd, buf, 300);
    if (count < 0) {
        DUER_LOGE("Read /tmp/free_log failed ret: %d", count);

        ret = DUER_ERR_FAILED;

        goto close_fd;
    }

    buf[300] = '\0';

    sscanf(buf, "%*[^:]:%s %*s %s %s %s",
            total_memory_str,
            available_memory_str,
            shared_memory_str,
            buffer_memory_str);

    mem_info->total_memory_size = atoi(total_memory_str);
    mem_info->available_memory_size = atoi(available_memory_str);
    mem_info->shared_memory_size = atoi(shared_memory_str);
    mem_info->buffer_memory_size = atoi(buffer_memory_str);

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");

        goto close_fd;
    }

    mem_info->peak = peak;
    mem_info->trough = trough;
    mem_info->average = average;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");

        goto close_fd;
    }

    swap_info = strstr(buf, "Swap");
    if (swap_info == NULL) {
        DUER_LOGE("Can not find swap info");

        goto close_fd;
    }

    sscanf(swap_info, "%*[^:]:%s %*s %s",
            swap_size_str,
            free_swap_size_str);

    mem_info->swap_size = atoi(swap_size_str);
    mem_info->free_swap_size = atoi(free_swap_size_str);

close_fd:
    close(fd);
out:
    return ret;
}

static int free_disk_info(duer_disk_info_t *disk_info)
{
    duer_disk_info_t *item = NULL;

    while (disk_info) {
        item = disk_info->next;
        DUER_FREE(disk_info);
        disk_info = item;
    }

    return DUER_OK;
}

static int get_disk_info(duer_disk_info_t **disk_info)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int fd = 0;
    int num = 0;
    int count = 0;
    int ret = DUER_OK;
    char buf[601];
    char line_str[101];
    char used_size_str[10];
    char total_size_str[10];
    char unilization_str[10];
    char available_size_str[10];
    char mount_info_str[MOUNT_INFO_LEN + 1];
    char partition_name[PARTITION_NAME_LEN + 1];
    duer_disk_info_t *item = NULL;
    duer_disk_info_t *next = NULL;
    duer_disk_info_t *head = NULL;
    duer_disk_info_t *previous = NULL;
    duer_disk_info_t *partition = NULL;

    memset(buf, 0, 601);
    memset(line_str, 0, 101);
    memset(partition_name, 0, PARTITION_NAME_LEN + 1);
    memset(total_size_str, 0, 10);
    memset(used_size_str, 0, 10);
    memset(available_size_str, 0, 10);
    memset(unilization_str, 0, 10);
    memset(mount_info_str, 0, MOUNT_INFO_LEN + 1);

    ret = system("df > /tmp/disk_info_log");
    if (ret < 0) {
        DUER_LOGE("Get disk info failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto err;
    }

    fd = open("/tmp/disk_info_log", O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open /tmp/disk_info_log failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto err;
    }

    count = read(fd, buf, 600);
    if (count < 0) {
        DUER_LOGE("Read /tmp/disk_info_log failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto close_fd;
    }

    for (i = 0, j = 0, k = 1; buf[i] != '\0'; i++) {
        if (buf[i] != '\n') {
            line_str[j++] = buf[i];
        } else {
            j = 0;

            if (k++ < 4) {
                continue;
            }

            partition = DUER_MALLOC(sizeof(*partition));
            if (partition == NULL) {
                DUER_LOGE("Malloc disk fialed");

                ret = DUER_ERR_FAILED;

                goto free_memory;
            }

            memset(partition, 0, sizeof(*partition));

            if (k == 5) {
                head = partition;
                previous = partition;
            } else {
                previous->next = partition;
                previous = partition;
            }

            sscanf(line_str, "%s %s %s %s %s %s",
                   partition_name,
                   total_size_str,
                   used_size_str,
                   available_size_str,
                   unilization_str,
                   mount_info_str);

            strncpy(partition->partition_name, partition_name, PARTITION_NAME_LEN);
            strncpy(partition->mount_info , mount_info_str, MOUNT_INFO_LEN);
            strncpy(partition->unilization, unilization_str, UNILIZATION_LEN);
            partition->total_size = atoi(total_size_str);
            partition->used_size = atoi(used_size_str);
            partition->available_size = atoi(available_size_str);
            partition->next = NULL;

            memset(line_str, 0 , 101);
            memset(partition_name, 0, PARTITION_NAME_LEN + 1);
            memset(total_size_str, 0, 10);
            memset(used_size_str, 0, 10);
            memset(available_size_str, 0, 10);
            memset(unilization_str, 0, 10);
            memset(mount_info_str, 0, MOUNT_INFO_LEN + 1);
        }
    }

    *disk_info = head;

    close(fd);

    return ret;

free_memory:
    item = head;
    while (item) {
        next = item->next;
        DUER_FREE(item);
        item = next;
    }
close_fd:
    close(fd);
err:
    return ret;
}

static int get_str_num(char const *str, char const *needle, uint64_t *num)
{
    char num_str[20];
    char *search_ret = NULL;

    memset(num_str, 0, 20);
    search_ret = strstr(str, needle);
    if (search_ret == NULL) {

        return DUER_ERR_FAILED;
    }

    sscanf(search_ret + strlen(needle), "%s", num_str);
    *num = atoi(num_str);

    return DUER_OK;
}

static int free_network_info(duer_network_card_info_t *network_info)
{
    duer_network_card_info_t *item = NULL;

    while (network_info) {
        item = network_info->next;
        if (network_info->quality != NULL) {
            DUER_FREE(network_info->quality);
        }
        DUER_FREE(network_info);
        network_info = item;
    }

    return DUER_OK;
}

static int get_network_info(duer_network_card_info_t **network_info)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int fd_net_info = 0;
    int fd_wireless_info = 0;
    int count = 0;
    int ret = DUER_OK;
    char num_str[20];
    char link_str[10];
    char line_str[121];
    char level_str[10];
    char noise_str[10];
    char w_netdev_buf[301];
    char *buf = NULL;
    char *search_ret = NULL;
    duer_network_card_info_t *item = NULL;
    duer_network_card_info_t *head = NULL;
    duer_network_card_info_t *next = NULL;
    duer_network_card_info_t *net_dev = NULL;
    duer_wireless_network_quality_t *wnet_dev_info = NULL;

    memset(line_str, 0, 121);

    ret = system("ifconfig > /tmp/network_info_log");
    if (ret < 0) {
        DUER_LOGE("Get network info failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto err;
    }

    buf = DUER_MALLOC(2001);
    if (buf == NULL) {
        DUER_LOGE("Malloc buf failed");

        ret = DUER_ERR_FAILED;

        goto err;
    }

    memset(buf, 0, 2001);

    fd_net_info = open("/tmp/network_info_log", O_RDONLY);
    if (fd_net_info < 0) {
        DUER_LOGE("Open /tmp/network_info_log failed ret: %d", ret);

        ret = DUER_ERR_FAILED;

        goto free_buf;
    }

    count = read(fd_net_info, buf, 2000);
    if (count < 0) {
        DUER_LOGE("Read /tmp/network_info_log failed ret: %d", count);

        ret = DUER_ERR_FAILED;

        goto close_fd_net_info;
    }

    for (i = 0, j = 0, k = 0; buf[i] != '\0'; i++) {
        if (buf[i] != '\n') {
            line_str[j++] = buf[i];
        } else {
            k++;
            j = 0;

            search_ret = strstr(line_str, "Link encap");
            if (search_ret != NULL) {
                net_dev = DUER_MALLOC(sizeof(*net_dev));
                if (net_dev == NULL) {
                    DUER_LOGE("Malloc net dev failed");

                    ret = DUER_ERR_FAILED;

                    goto close_fd_net_info;
                }

                memset(net_dev, 0, sizeof(*net_dev));

                if (k == 1) {
                    head = net_dev;
                    item = net_dev;
                } else {
                    item->next = net_dev;
                    item = net_dev;
                }

                sscanf(line_str, "%s", net_dev->network_card_name);

                search_ret = strstr(line_str, ":");
                if (search_ret != NULL) {
                    sscanf(search_ret + 1, "%s", net_dev->link_encap);
                }

                search_ret = strstr(line_str, "wlan");
                if (search_ret != NULL) {
                    wnet_dev_info = DUER_MALLOC(sizeof(*wnet_dev_info));
                    if (wnet_dev_info == NULL) {
                        DUER_LOGE("Malloc wireless net dev failed");

                        ret = DUER_ERR_FAILED;

                        goto free_net_dev;
                    } else {
                        fd_wireless_info = open("/proc/net/wireless", O_RDONLY);
                        if (fd_wireless_info < 0) {
                            DUER_LOGE("Open /proc/net/wireless failed");

                            ret = DUER_ERR_FAILED;

                            goto free_net_dev;
                        }

                        memset(w_netdev_buf, 0, 301);

                        count = read(fd_wireless_info, w_netdev_buf, 300);
                        if (count < 0) {
                            DUER_LOGE("Read /proc/net/wireless failed");

                            ret = DUER_ERR_FAILED;

                            close(fd_wireless_info);

                            goto free_net_dev;
                        }

                        close(fd_wireless_info);

                        search_ret = strstr(w_netdev_buf, net_dev->network_card_name);
                        if (search_ret != NULL) {
                            memset(link_str, 0, 10);
                            memset(level_str, 0, 10);
                            memset(noise_str, 0, 10);

                            sscanf(search_ret, "%*[^:]:%*s %[^.]. %[^.]. %[^.].",
                                   link_str,
                                   level_str,
                                   noise_str);

                            wnet_dev_info->link = atoi(link_str);
                            wnet_dev_info->level = atoi(level_str);
                            wnet_dev_info->noise = atoi(noise_str);

                            net_dev->quality = wnet_dev_info;
                        }
                    }
                }

                search_ret = strstr(line_str, "HWaddr");
                if (search_ret != NULL) {
                    sscanf(search_ret, "%*s %[a-zA-Z0-9:]", net_dev->hw_address);
                }
            }

            search_ret = strstr(line_str, "inet addr");
            if (search_ret != NULL) {
                search_ret = strstr(line_str, ":");
                if (search_ret != NULL) {
                    sscanf(search_ret + 1, "%s", net_dev->ipv4_address);
                }

                search_ret = strstr(line_str, "Bcast");
                if (search_ret != NULL) {
                    sscanf(search_ret + 6, "%s", net_dev->bcast);
                }

                search_ret = strstr(line_str, "Mask");
                if (search_ret != NULL) {
                    sscanf(search_ret + 5, "%s", net_dev->mask);
                }
            }

            search_ret = strstr(line_str, "inet6 addr");
            if (search_ret != NULL) {
                sscanf(search_ret + 11, "%[^%]", net_dev->ipv6_address);
            }

            search_ret = strstr(line_str, "MTU");
            if (search_ret != NULL) {
                memset(num_str, 0, 20);
                sscanf(search_ret + 4, "%s", num_str);
                net_dev->mtu = atoi(num_str);
            }

            search_ret = strstr(line_str, "RX packets:");
            if (search_ret != NULL) {
                get_str_num(line_str, "RX packets:", &net_dev->rx_packet);
                get_str_num(line_str, "errors:", &net_dev->rx_error_packet);
                get_str_num(line_str, "dropped:", &net_dev->rx_dropped_packet);
                get_str_num(line_str, "overruns:", &net_dev->rx_overruns);
                get_str_num(line_str, "frame:", &net_dev->rx_frame);
            }

            search_ret = strstr(line_str, "TX packets:");
            if (search_ret != NULL) {
                get_str_num(line_str, "TX packets:", &net_dev->tx_packet);
                get_str_num(line_str, "errors:", &net_dev->tx_error_packet);
                get_str_num(line_str, "dropped:", &net_dev->tx_dropped_packet);
                get_str_num(line_str, "overruns:", &net_dev->tx_overruns);
            }

            memset(line_str, 0, 121);
        }
    }

    *network_info = head;

    close(fd_net_info);

    return ret;

free_net_dev:
    item = head;
    while (item) {
        next = item->next;
        if (item->quality != NULL) {
            DUER_FREE(item->quality);
        }
        DUER_FREE(item);
        item = next;
    }
close_fd_net_info:
    close(fd_net_info);
free_buf:
    DUER_FREE(buf);
err:
    return ret;
}

static void calculate_system_info(void *arg)
{
    int fd = 0;
    int ret = DUER_OK;
    static int count = 1;
    char buf[301];
    char used_memory_str[10];
    uint64_t used_memory = 0;

    memset(used_memory_str, 0, 10);

    ret = system("free > /tmp/free_log");
    if (ret < 0) {
        DUER_LOGE("Get Memory info failed ret: %d", ret);

        return;
    }

    fd = open("/tmp/free_log", O_RDONLY);
    if (fd < 0) {
        DUER_LOGE("Open /tmp/free_log failed");

        return;
    }

    count = read(fd, buf, 300);
    if (count < 0) {
        DUER_LOGE("Read /tmp/free_log failed ret: %d", count);

        goto close_fd;
    }

    buf[300] = '\0';

    sscanf(buf, "%*[^:]:%*s %s %*s %*s %*s",
            used_memory_str);

    used_memory = atoi(used_memory_str);

    ret = duer_mutex_lock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Lock sys info failed");

        goto close_fd;
    }

    if ((count++ % 100) == 0) {
        peak = 0;
        trough = (~0x0);
    }

    if (used_memory > peak) {
        peak = used_memory;
    }

    if (used_memory < trough) {
        trough = used_memory;
    }

    average = (peak + trough) / 2;

    ret = duer_mutex_unlock(s_lock_sys_info);
    if (ret != DUER_OK) {
        DUER_LOGE("Unlock sys info failed");
    }

close_fd:
    close(fd);
}

static int init_timer(void)
{
    int ret = DUER_OK;
    duer_timer_handler timer;

    timer = duer_timer_acquire(calculate_system_info, NULL, DUER_TIMER_PERIODIC);

    ret = duer_timer_start(timer, DELAY_TIEM);
    if (ret != DUER_OK) {
        DUER_LOGE("Start timer failed");

        ret = DUER_ERR_FAILED;
    }

    if (s_lock_sys_info == NULL) {
        s_lock_sys_info = duer_mutex_create();
        if (s_lock_sys_info == NULL) {
            DUER_LOGE("Create lock failed");

            return DUER_ERR_FAILED;
        }
    }

    return ret;
}

static duer_system_info_ops_t system_info_ops = {
    .init                    = init_timer,
    .get_system_static_info  = get_system_static_info,
    .get_system_dynamic_info = get_system_dynamic_info,
    .get_memory_info         = get_memory_info,
    .get_disk_info           = get_disk_info,
    .free_disk_info          = free_disk_info,
    .get_network_info        = get_network_info,
    .free_network_info       = free_network_info,
};

int duer_init_system_info(void)
{
    return duer_system_info_register_system_info_ops(&system_info_ops);
}
