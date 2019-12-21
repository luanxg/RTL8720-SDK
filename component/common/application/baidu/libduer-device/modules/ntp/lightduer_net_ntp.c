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
// Author: Chang Li (changli@baidu.com)
//
// Description: NTP client API implementation.

/**
 * NTP timestamp format,
 *      coarse: integer part  (uint32)
 *      fine  : fraction part (uint32)
 *
 * NTP packet format,
 *  |0 1|2  4|5  7|8     15|16   23|24     31|
 *  |LI |VN  |Mode|Stratum |Poll   |Precision|
 *  |    Root Delay          (32 bits)       |
 *  |    Root Dispersion     (32 bits)       |
 *  |    Reference Identifier(32 bits)       |
 *  |    Reference Timestamp (64 bits)       |
 *  |    Originate Timestamp (64 bits)       |
 *  |    Receive   Timestamp (64 bits)       |
 *  |    Transmit  Timestamp (54 bits)       |
 *  |    (opt) Auth KeyID    (32 bits)       |
 *  |    (opt) MAC           (128 bits)      |
 */

#include "lightduer_net_ntp.h"
#include "lightduer_lib.h"
#include "lightduer_timestamp.h"
#include "lightduer_log.h"
#include "lightduer_sleep.h"
#include "lightduer_net_transport_wrapper.h"
#include "lightduer_net_util.h"
#include "mbedtls/md5.h"

//#define DUER_DEBUG_LEVEL 4

#define NTP_SERVER "s2c.time.edu.cn"	// 202.112.10.36
#define NTP_PORT (123)

#define JAN_1970    (0x83aa7e80)    // 2208988800 1970 - 1900 in seconds

#define LI          (0)             // Leap Indicator, normal
#define VN          (3)             // Version, v3
#define MODE        (3)             // Mode, client mode
#define STRATUM     (0)             // Stratum, 0 for unspecified.
#define POLL        (4)             // Poll Interval, log2 interval(32 sec)
#define PREC        (-6)            // Precision, nearest power of two(64 ms)

/* Compute the micro second part to NTP fraction.
 * i.e. ( usec / 1e6 * UINT_MAX), UINT_MAX is 4294967296
 * Multiply by 4294.967296 quickly while not quite exactly.
 */
#define NTPFRAC(x) ( 4294 * (x) + ( (1981 * (x)) >> 11 ) )

/* Compute the NTP fraction part to micro seconds.
 * The reverse of NTPFRAC
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

const int RECV_TIMEOUT = 2000;  // 2000 ms by default.

typedef struct _NtpTime {
    uint32_t coarse;
    uint32_t fine;
} NtpTime;

static void ntp_get_current_time(NtpTime *time);
static void ntp_set_time(NtpTime *new_time, uint32_t adjust_time,
                         DuerTime *result_time);
static int ntp_send_packet(duer_trans_handler hdlr, const duer_addr_t *addr, Md5Key *key);
static int ntp_rfc1305_calc_set(uint32_t *data, NtpTime *arrival,
                                DuerTime *result_time);

/*
 * @Brief: generate message digest and append it to packet.
 * @Param key: the key string.
 * @Param pkt: packet which contains key ID and digest.
 * @Return: .
 */
static int md5_authencrypt(
    const char     *key,    /* key pointer */
    unsigned int   *pkt,    /* packet pointer */
    int             length  /* packet length 48B*/
    )
{
    unsigned char digest[MD5_DIGEST_LEN] = {0};
    mbedtls_md5_context ctx;

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (const duer_u8_t *)key, DUER_STRLEN(key));
    mbedtls_md5_update(&ctx, (duer_u8_t *)pkt, length);
    mbedtls_md5_finish(&ctx, digest);
    mbedtls_md5_free(&ctx);

#ifdef NTP_DEBUG
    DUER_LOGD("###digest=\n");
    int i = 0;
    for(i = 0; i < MD5_DIGEST_LEN; i++) {
        DUER_LOGD("%02x ", digest[i]);
    }
    DUER_LOGD("###\n");
#endif

    DUER_MEMMOVE((duer_u8_t *)pkt + length + MD5_KEYID_LEN, digest, sizeof(digest));

    return 0;
}

/* return the value in micro seconds */
static uint32_t ntp_diff(NtpTime *start, NtpTime *stop) {
    int a = 0;
    uint32_t b = 0;
    uint32_t offset = 0;
    a = stop->coarse - start->coarse;

    if (stop->fine >= start->fine) {
        b = stop->fine - start->fine;
    } else {
        b = start->fine - stop->fine;
        b = ~b;
        a -= 1;
    }

    offset = a * 1000000 + USEC(b);
    return offset;
}

void ntp_get_current_time(NtpTime *time) {
    uint32_t ts = duer_timestamp();
    DUER_LOGD("%s: Timestamp=%ld\n", __FUNCTION__, ts);

    time->coarse = ts / 1000;
    time->fine = NTPFRAC((ts % 1000) * 1000);
    DUER_LOGD("%s: current_time=%u, %u.\n", __FUNCTION__, time->coarse, time->fine);
}

void ntp_set_time(NtpTime *new_time, uint32_t adjust_time,
                  DuerTime *result_time) {
    result_time->sec = new_time->coarse - JAN_1970 + adjust_time / 1000000;
    result_time->usec = USEC(new_time->fine);
    result_time->usec += adjust_time % 1000000;

    if (result_time->usec > 999999) {
        result_time->usec -= 1000000;
        result_time->sec += 1;
    }
}

int ntp_rfc1305_calc_set(uint32_t *data, NtpTime *arrival,
                         DuerTime *result_time) {
    int li                  = 0;
    int vn                  = 0;
    int mode                = 0;
    int stratum             = 0;
    int poll                = 0;
    int prec                = 0;
    int delay               = 0;
    int disp                = 0;
    int refid               = 0;
    uint32_t el_time        = 0;
    uint32_t st_time        = 0;
    uint32_t adjust_time    = 0;
    int ret                 = -1;
    NtpTime reftime;
    NtpTime orgtime;
    NtpTime rectime;
    NtpTime xmttime;

    ALLOW_UNUSED_LOCAL(reftime);
    ALLOW_UNUSED_LOCAL(ret);
    ALLOW_UNUSED_LOCAL(refid);
    ALLOW_UNUSED_LOCAL(disp);
    ALLOW_UNUSED_LOCAL(delay);
    ALLOW_UNUSED_LOCAL(poll);
    ALLOW_UNUSED_LOCAL(stratum);
    ALLOW_UNUSED_LOCAL(mode);
    ALLOW_UNUSED_LOCAL(vn);
    ALLOW_UNUSED_LOCAL(li);

    li = duer_ntohl(((uint32_t *)data)[0]) >> 30 & 0x03;
    vn = duer_ntohl(((uint32_t *)data)[0]) >> 27 & 0x07;
    mode = duer_ntohl(((uint32_t *)data)[0]) >> 24 & 0x07;
    stratum = duer_ntohl(((uint32_t *)data)[0]) >> 16 & 0xff;
    poll = duer_ntohl(((uint32_t *)data)[0]) >> 8 & 0xff;
    prec = duer_ntohl(((uint32_t *)data)[0]) & 0xff;

    if (prec & 0x80) {
        prec |= 0xffffff00;
    }

    delay = duer_ntohl(((uint32_t *)data)[1]);
    disp = duer_ntohl(((uint32_t *)data)[2]);
    refid = duer_ntohl(((uint32_t *)data)[3]);
    reftime.coarse = duer_ntohl(((uint32_t *)data)[4]);
    reftime.fine   = duer_ntohl(((uint32_t *)data)[5]);
    orgtime.coarse = duer_ntohl(((uint32_t *)data)[6]);
    orgtime.fine   = duer_ntohl(((uint32_t *)data)[7]);
    rectime.coarse = duer_ntohl(((uint32_t *)data)[8]);
    rectime.fine   = duer_ntohl(((uint32_t *)data)[9]);
    xmttime.coarse = duer_ntohl(((uint32_t *)data)[10]);
    xmttime.fine   = duer_ntohl(((uint32_t *)data)[11]);

    DUER_LOGD("%s: li=%d, vn=%d, mode=%d, stratum=%d, poll=%d, prec=%d;\n"
              "delay=%d, disp=%d, refid=%d;\n"
              "xmttime=%u,%u\n", __FUNCTION__, li, vn, mode, stratum, poll, prec, delay, disp,
              refid, xmttime.coarse, xmttime.fine);

    el_time = ntp_diff(&orgtime, arrival);
    st_time = ntp_diff(&rectime, &xmttime);
    adjust_time = (el_time - st_time) / 2;

    ntp_set_time(&xmttime, adjust_time, result_time);

    return 0;
}

int ntp_send_packet(duer_trans_handler hdlr, const duer_addr_t* addr, Md5Key* key) {
    uint32_t data[NTP_PKG_LEN / sizeof(uint32_t) + NTP_MAC_LEN / sizeof(uint32_t)];
    NtpTime time_sent;
    int ret = -1;
    duer_size_t size = NTP_PKG_LEN;

    DUER_MEMSET(data, 0, sizeof(data));
    data[0] = duer_htonl((LI << 30) | (VN << 27) | (MODE << 24)
                      | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
    data[1] = duer_htonl(1 << 16); // Root Delay (seconds)
    data[2] = duer_htonl(1 << 16); // Root Dispersion (seconds)
    ntp_get_current_time(&time_sent);
    data[10] = duer_htonl(time_sent.coarse); // Transmit Timestamp coarse
    data[11] = duer_htonl(time_sent.fine); // Transmit Timestamp fine

    if (key != NULL) {
        data[12] = duer_htonl(key->key_id); /* KeyID */
        md5_authencrypt(key->key_str, data, NTP_PKG_LEN);
        size = NTP_PKG_LEN + NTP_MAC_LEN;
    }

    ret = duer_trans_send(hdlr, data, size, addr);

    return ret;
}

int duer_ntp_client(char *host, int timeout, DuerTime *result_time, Md5Key *key) {
    duer_trans_handler hdlr = NULL;
    duer_addr_t svr_addr;
    uint32_t incoming_data[255] = {0};
    NtpTime time_recv;
    int ret = -1;

    if (host == NULL) {
        svr_addr.host = NTP_SERVER;
        svr_addr.host_size = DUER_STRLEN(NTP_SERVER);
    } else {
        svr_addr.host = host;
        svr_addr.host_size = DUER_STRLEN(host);
    }
    svr_addr.port = NTP_PORT;
    svr_addr.type = DUER_PROTO_UDP;

    hdlr = duer_trans_acquire(NULL, NULL);
    duer_trans_connect(hdlr, &svr_addr);
    ret = ntp_send_packet(hdlr, &svr_addr, key);    //svr_addr is useless?
    DUER_LOGD("%s: send return=%d.\n", __FUNCTION__, ret);

    if ((ret != NTP_PKG_LEN) && (ret != NTP_PKG_LEN + NTP_MAC_LEN)) {
        DUER_LOGW("send nok, ret=%d, should resend.", ret);
        duer_trans_close(hdlr);
        return -1;
    }

    int wait_time = 0;  // ms
    if (timeout == 0) {
        timeout = RECV_TIMEOUT;
    }

    while (1) {
        ret = duer_trans_recv(hdlr, incoming_data, sizeof(incoming_data),
                             &svr_addr);    //svr_addr is useless?
        DUER_LOGD("%s: receive return=%d.\n", __FUNCTION__, ret);

        if (ret < 0) {
            DUER_LOGD("recv nok, wait for 500 ms.");
            wait_time += 500;
            if (wait_time > timeout) {
                break;
            }

            duer_sleep(500);

            continue;
        }

        ntp_get_current_time(&time_recv);
        ret = ntp_rfc1305_calc_set(incoming_data, &time_recv, result_time);
        DUER_LOGD("%s: calc_set return=%d.\n", __FUNCTION__, ret);

        if (ret == 0) {
            DUER_LOGD("NTP: success.\n");
            DUER_LOGI("%s: result time=%u, %u\n", __FUNCTION__, result_time->sec,
                      result_time->usec);
            break;
        } else {
            DUER_LOGE("%s: NTP received data, but not ok.\n", __FUNCTION__);
            break;
        }
    }

    duer_trans_close(hdlr);

    return ret;
}



