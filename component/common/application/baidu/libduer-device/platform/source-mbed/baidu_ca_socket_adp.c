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
/**
 * File: baidu_ca_socket_adp.c
 * Auth: Zhang Leliang (zhangleliang@baidu.com)
 * Desc: Adapt the socket function to esp32.
 */

#include "baidu_ca_adapter_internal.h"

#include "lightduer_connagent.h"
#ifdef DUER_HTTP_DNS_SUPPORT
#include "lightduer_http_dns_client_ops.h"
#endif // DUER_HTTP_DNS_SUPPORT
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_events.h"
#include "lightduer_random.h"
#include "lightduer_timers.h"
#include "lightduer_timestamp.h"
#include "lightduer_priority_conf.h"

#include <sockets.h>
#include <netdb.h>

#define TASK_STACK_SIZE (1024 * 4)

#ifndef NON_BLOCKING
#define NON_BLOCKING    (0)
#endif

extern duer_handler my_handler;

#if defined(NON_BLOCKING) && (NON_BLOCKING == 1)
#define MSG_FLAG        (MSG_DONTWAIT)
#else
#define MSG_FLAG        (0)
#endif

#ifndef DUER_SENDTIMEOUT
#define DUER_SENDTIMEOUT    6000 // 6s
#endif

typedef struct _bcasoc_s
{
    volatile int fd;
    volatile int is_send_block;
    volatile int destroy;
    volatile int ref_count;
    duer_u32_t           send_data_block_begin_time; // unit is ms
    duer_timer_handler  _timer;
    duer_transevt_func  _callback;
} bcasoc_t;

static duer_events_handler  g_lightduer_events = NULL;
static duer_mutex_t         g_mutex = NULL;
static duer_timer_handler   s_finalize_timer = NULL;

static void bcasoc_lock() {
    duer_mutex_lock(g_mutex);
}

static void bcasoc_unlock() {
    duer_mutex_unlock(g_mutex);
}

static unsigned int bcasoc_parse_ip(const duer_addr_t *addr, duer_bool *use_http_dns)
{
    unsigned int rs = 0;
    if (addr && addr->host) {
        DUER_LOGV("bcasoc_parse_ip: ip = %s", addr->host);
#if 1
        struct hostent *hp = gethostbyname(addr->host);
#else
        //debug purpose
        struct hostent *hp = NULL;
        struct sockaddr_in addr_in;
        addr_in.sin_addr.s_addr = inet_addr(addr->host);
        if(addr_in.sin_addr.s_addr == INADDR_NONE) {
            DUER_LOGI("invalid ip");
            hp = NULL;
        } else {
            hp = gethostbyname(addr->host);
        }
#endif
#ifdef DUER_HTTP_DNS_SUPPORT
        if (!hp) {
            DUER_LOGW("retry HTTP DNS, %s!", addr->host);
            duer_http_dns_task_init();
            const char *ip = duer_http_dns_query(addr->host, 2000); // timeout is 2s
            duer_http_dns_task_destroy();
            DUER_LOGD("http dns got ip:%s", ip);
            if (ip) {
                if (use_http_dns) {
                    *use_http_dns = DUER_TRUE;
                }
                hp = gethostbyname(ip);
            }
        }
#endif // DUER_HTTP_DNS_SUPPORT
        if (!hp) {
            DUER_LOGE("DNS failed!!!");
        } else {
            struct in_addr* ip4_addr;
            if (hp->h_addrtype == AF_INET) {
                ip4_addr = (struct in_addr*)hp->h_addr_list[0];
                DUER_LOGI("DNS lookup succeeded. IP=%s", inet_ntoa(*ip4_addr));
            } else if (hp->h_addrtype == AF_INET6) {
                DUER_LOGE("ipv6 address got");
            } else {
                DUER_LOGE("other type(%d) address got", hp->h_addrtype);
            }
            if (ip4_addr) {
                rs = htonl(ip4_addr->s_addr);
            }
        }
    }

    DUER_LOGI("bcasoc_parse_ip: rs = 0x%08x", rs);

    return rs;
}

static void bcasoc_check_data(int what, void *object)
{
    bcasoc_lock();
    bcasoc_t *soc = (bcasoc_t *)object;
    int rs = 0;
    int fd = -1;
    duer_transevt_func callback = NULL;
    fd_set fdread, fdwrite, fdex;
    struct timeval time = {0, 0};

    if (!soc) {
        goto out;
    } else {
        DUER_LOGV("soc:%p, destroy:%d, fd:%d, ref_count:%d, timer:%p",
                soc, soc->destroy, soc->fd, soc->ref_count, soc->_timer);
        soc->ref_count--;
        if (soc->fd == -1) {
            DUER_LOGI("soc:%p, destroy:%d, fd:%d, ref_count:%d, timer:%p",
                    soc, soc->destroy, soc->fd, soc->ref_count, soc->_timer);
            if (soc->destroy == 1) {
                if (soc->_timer) {
                    duer_timer_release(soc->_timer);
                    soc->_timer = NULL;
                }
                if (!soc->ref_count) {
                    DUER_FREE(soc);
                }
            }
            goto out;
        }
    }

    callback = soc->_callback;

    if (soc->is_send_block == 1) {
        duer_u32_t current_ts = duer_timestamp();
        DUER_LOGD("current timestamp:%u, begin_time:%u",
                    current_ts, soc->send_data_block_begin_time);
        if (soc->send_data_block_begin_time) {
            if (current_ts < soc->send_data_block_begin_time) {
                soc->send_data_block_begin_time = current_ts;
            } else if (current_ts - soc->send_data_block_begin_time > DUER_SENDTIMEOUT) {
                DUER_LOGI("send timeout current timestamp:%u, begin_time:%u",
                        current_ts, soc->send_data_block_begin_time);
                soc->send_data_block_begin_time = duer_timestamp();// reset the timestamp
                if (callback) {
                    callback(DUER_TEVT_SEND_TIMEOUT);
                }
            }
        }
    }

    fd = soc->fd;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdex);

    FD_SET(fd, &fdread);
    FD_SET(fd, &fdwrite);
    FD_SET(fd, &fdex);

    rs = select(fd + 1, &fdread, &fdwrite, &fdex, &time);
    DUER_LOGD("select rs:%d, block:%d", rs, soc->is_send_block);

    if (rs > 0) {
        if (FD_ISSET(fd, &fdread)) {
            if (callback) {
                callback(DUER_TEVT_RECV_RDY);
            }
        } else if (FD_ISSET(fd, &fdex)) {
            DUER_LOGE("exception occurs %d:%s", rs, strerror(rs));
            rs = -1;
        }
        if (soc->is_send_block == 1) {
            if (FD_ISSET(fd, &fdwrite)) {
                if (callback) {
                    callback(DUER_TEVT_SEND_RDY);
                }
                soc->is_send_block = 0;
                soc->send_data_block_begin_time = 0;
            }
        }

        if (!FD_ISSET(fd, &fdread)) {
            if (soc->_timer) {
                duer_timer_start(soc->_timer, 100);
            }
        }
    } else if (rs == 0) {
        DUER_LOGD("run: %p", soc);
        if (soc->_timer) {
            duer_timer_start(soc->_timer, 100);
        }
    }

out:
    bcasoc_unlock();
}

static int bcasoc_events_call(duer_events_func func, int what, void *object)
{
    if (g_lightduer_events) {
        return duer_events_call(g_lightduer_events, func, what, object);
    }
    return DUER_ERR_FAILED;
}

static void bcasoc_timer_expired(void *param)
{
    bcasoc_lock();
    bcasoc_t *soc = (bcasoc_t *)param;
    if (!soc->ref_count) {
        soc->ref_count++;
        bcasoc_events_call(bcasoc_check_data, 0, param);
    }
    bcasoc_unlock();
}

void bcasoc_initialize(void)
{
    if (g_lightduer_events == NULL) {
        g_lightduer_events = duer_events_create("lightduer_socket", TASK_STACK_SIZE, 10);
    }

    if (g_mutex == NULL) {
        g_mutex = duer_mutex_create();
    }
}

duer_socket_t bcasoc_create(duer_transevt_func func)
{
    bcasoc_t *soc = DUER_MALLOC(sizeof(bcasoc_t));
    if (soc) {
        DUER_MEMSET(soc, 0, sizeof(bcasoc_t));
        soc->fd = -1;
        soc->is_send_block = 0;
        soc->destroy = 0;
        soc->ref_count = 0;
        soc->send_data_block_begin_time = 0;
        soc->_callback = func;
    }
    return soc;
}

duer_status_t bcasoc_connect(duer_socket_t ctx, const duer_addr_t *addr)
{
    int rs = DUER_ERR_FAILED;
    duer_bool use_http_dns = DUER_FALSE;
    bcasoc_t *soc = (bcasoc_t *)ctx;

    DUER_LOGV("Entry bcasoc_connect ctx = %p", ctx);

    if (soc && addr) {
        soc->fd = socket(AF_INET, addr->type == DUER_PROTO_TCP ? SOCK_STREAM : SOCK_DGRAM,
                         addr->type == DUER_PROTO_TCP ? IPPROTO_TCP : IPPROTO_UDP);
        DUER_LOGV("Result bcasoc_connect fd = %d", soc->fd);
        if (soc->fd < 0) {
            DUER_LOGE("socket create failed: %d", soc->fd);
            return soc->fd;
        }

        //NOTE:
        //update the port, because on mbed platform the port is not random
        //every start with the same initial value which may cause connect fail
        struct sockaddr_in src_addr;
        DUER_MEMSET(&src_addr, 0, sizeof(src_addr));
        socklen_t src_addr_len = sizeof(src_addr);
        duer_u16_t port = (duer_u32_t)duer_random() % (0x10000 - 5000) + 5000;
        src_addr.sin_port = htons(port);
        src_addr.sin_family = AF_INET;
        rs = bind(soc->fd, (struct sockaddr*)&src_addr, src_addr_len);
        if (rs >= 0) {
            DUER_LOGI("bind new port is: %d!", port);
        } else {
            DUER_LOGI("bind failed!");
        }

        struct sockaddr_in addr_in;
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(addr->port);
        DUER_LOGV("Result bcasoc_connect: addr = %s", (char *)addr->host);
        unsigned int ip_data = bcasoc_parse_ip(addr, &use_http_dns);
        if (ip_data > 0) {
            addr_in.sin_addr.s_addr = htonl(ip_data);
        } else {
            DUER_LOGI("got ip failed!");
            return DUER_ERR_TRANS_DNS_FAIL;
        }
        rs = connect(soc->fd, (struct sockaddr *)&addr_in, sizeof(addr_in));

        if (rs >= 0) {
#if defined(NON_BLOCKING) && (NON_BLOCKING == 1)
            int flags = fcntl(soc->fd, F_GETFL, 0);
            fcntl(soc->fd, F_SETFL, flags | O_NONBLOCK);
#endif
            if (soc->_callback != NULL) {
                if (soc->_timer == NULL) {
                    soc->_timer = duer_timer_acquire(bcasoc_timer_expired, soc, DUER_TIMER_ONCE);
                }
                //DUER_LOGI("before call bcasoc_timer_expired, soc:%p, fd:%d", soc, soc->fd);
                bcasoc_timer_expired((void*)soc);
            }
            if (use_http_dns) {
                rs = DUER_INF_TRANS_IP_BY_HTTP_DNS;
            }
            // debug purpose
            // int tmp = getsockname(soc->fd, (struct sockaddr*)&src_addr, &src_addr_len);
            // if (tmp >= 0) {
            //     DUER_LOGI(" src ip:%s, port:%d \n",
            //         inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
            // } else {
            //     DUER_LOGI(" getsockname failed!");
            // }
        }
    }

    DUER_LOGD("Result bcasoc_connect rs = %d", rs);

    return rs;
}

duer_status_t bcasoc_send(duer_socket_t ctx, const void *data, duer_size_t size, const duer_addr_t *addr)
{
    bcasoc_t *soc = (bcasoc_t *)ctx;
    int rs = DUER_ERR_FAILED;
    fd_set fdw;
    struct timeval tv;

    if (!soc || soc->fd == -1) {
        return rs;
    }

    FD_ZERO(&fdw);
    FD_SET(soc->fd, &fdw);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    rs = select(soc->fd + 1, NULL, &fdw, NULL, &tv);
    DUER_LOGD("bcasoc_send select rs:%d, EINTR:%d, size:%d", rs, EINTR, size);
    if (FD_ISSET(soc->fd, &fdw)) {
        rs = send(soc->fd, data, size, MSG_FLAG);
        DUER_LOGD("try send rs:%d!", rs);
    } else if (rs == 0) {
        DUER_LOGD("send timeout -> WOULD_BLOCK!!!");
        rs = DUER_ERR_TRANS_WOULD_BLOCK;
        bcasoc_lock();
        soc->is_send_block = 1;
        soc->send_data_block_begin_time = duer_timestamp();
        bcasoc_unlock();
    } else if (rs != EINTR) {
        DUER_LOGE("write socket error %d:%s", rs, strerror(rs));
        rs = DUER_ERR_TRANS_INTERNAL_ERROR;
    }

    DUER_LOGD("Result bcasoc_send rs = %d", rs);

    return rs > 0 ? size : rs;
}

duer_status_t bcasoc_recv(duer_socket_t ctx, void *data, duer_size_t size, duer_addr_t *addr)
{
    bcasoc_t *soc = (bcasoc_t *)ctx;
    int rs = DUER_ERR_FAILED;
    fd_set fdr;
    struct timeval tv;

    if (!soc || soc->fd == -1) {
        return rs;
    }

    FD_ZERO(&fdr);
    FD_SET(soc->fd, &fdr);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    rs = select(soc->fd + 1, &fdr, NULL, NULL, &tv);
    DUER_LOGD("recv select result:%d, size:%d", rs, size);
    if (FD_ISSET(soc->fd, &fdr)) {
        rs = recv(soc->fd, data, size, MSG_FLAG);
        if (rs <= 0) {
            DUER_LOGE("Can't reach remote host!!! rs = %d", rs);
            rs = DUER_ERR_TRANS_INTERNAL_ERROR;
        }
    } else if (rs == 0) {
        rs = DUER_ERR_TRANS_WOULD_BLOCK;
    } else if (rs != EINTR) {
        DUER_LOGE("read socket error %d:%s", rs, strerror(rs));
        rs = DUER_ERR_TRANS_INTERNAL_ERROR;
    }

    if (rs > 0 ) {
        if (soc->fd != -1) {
            //DUER_LOGI("before call bcasoc_timer_expired, soc:%p, fd:%d", soc, soc->fd);
            bcasoc_timer_expired(soc);
        }
    }

    DUER_LOGD("Result bcasoc_recv rs = %d", rs);

    return rs;
}

duer_status_t bcasoc_recv_timeout(duer_socket_t ctx, void *data, duer_size_t size, duer_u32_t timeout, duer_addr_t *addr)
{
    DUER_LOGV("bcasoc_recv_timeout: ctx = %p, data = %p, size = %d, timeout = %d, addr = %p", ctx, data, size, timeout,
              addr);
    int rs = DUER_ERR_TRANS_INTERNAL_ERROR;
    bcasoc_t *soc = (bcasoc_t *)ctx;
    if (soc && soc->fd != -1) {
        fd_set fdr, fde;
        struct timeval time = {timeout / 1000, (timeout % 1000) * 1000};

        FD_ZERO(&fdr);
        FD_SET(soc->fd, &fdr);

        FD_ZERO(&fde);
        FD_SET(soc->fd, &fde);

        rs = select(soc->fd + 1, &fdr, NULL, &fde, &time);
        DUER_LOGV("bcasoc_recv_timeout select: rs = %d", rs);
        if (rs == 0) {
            rs = DUER_ERR_TRANS_TIMEOUT;
            DUER_LOGW("recv timeout!!!");
        } else if (rs < 0 || FD_ISSET(soc->fd, &fde)) {
            rs = DUER_ERR_TRANS_INTERNAL_ERROR;
            DUER_LOGW("recv failed: rs = %d", rs);
        } else if (FD_ISSET(soc->fd, &fdr)) {
            rs = recv(soc->fd, data, size, 0);
        }
    }

    DUER_LOGV("bcasoc_recv_timeout: rs = %d, fd = %d", rs, soc ? soc->fd : -99);
    return rs;
}

duer_status_t bcasoc_close(duer_socket_t ctx)
{
    bcasoc_t *soc = (bcasoc_t *)ctx;
    if (soc) {
        if (soc->_timer) {
            duer_timer_stop(soc->_timer);
            DUER_LOGI("stop timer:%p", soc->_timer);
        }
        bcasoc_lock();
        if (soc->fd != -1) {
            close(soc->fd);
            soc->fd = -1;
        }

        bcasoc_unlock();
    }
    return DUER_OK;
}

duer_status_t bcasoc_destroy(duer_socket_t ctx)
{
    bcasoc_t *soc = (bcasoc_t *)ctx;
    bcasoc_lock();
    soc->destroy = 1;
    DUER_LOGI("start last timer:%p, soc:%p", soc->_timer, soc);
    if (soc->_timer) {
        duer_timer_start(soc->_timer, 200);
    } else {
        DUER_FREE(soc);
    }
    bcasoc_unlock();
    return DUER_OK;
}

static void bcasoc_destroy_events(int what, void *object)
{
    DUER_LOGI("bcasoc_destroy_events");
    duer_events_destroy(g_lightduer_events);
    g_lightduer_events = NULL;
    duer_mutex_destroy(g_mutex);
    g_mutex = NULL;
    duer_timer_release(s_finalize_timer);
    s_finalize_timer = NULL;
}

static void bcasoc_finalize_callback(void *param)
{
    bcasoc_events_call(bcasoc_destroy_events, 0, NULL);
}

void bcasoc_finalize(void)
{
    s_finalize_timer = duer_timer_acquire(bcasoc_finalize_callback, NULL, DUER_TIMER_ONCE);
    if (s_finalize_timer) {
        duer_timer_start(s_finalize_timer, 200);
    }
}
