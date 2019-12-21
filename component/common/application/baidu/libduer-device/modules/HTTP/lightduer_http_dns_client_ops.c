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
 * File: lightduer_http_dns_client_ops.c
 * Auth: CHEN YUN (chenyun08@baidu.com)
 * Desc: HTTP DNS Client Socket OPS
 */

#if defined(DUER_HTTP_DNS_SUPPORT)
#include "lightduer_http_dns_client_ops.h"
#include "lightduer_log.h"
#include "lightduer_http_client.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_events.h"
#include "lightduer_mutex.h"
#include "baidu_json.h"

#define HTTP_DNS_REQ_TIMEOUT    (300) //ms
#define HTTP_DNS_RSP_IP_SIZE    (32)
#define MD5_SUM_MAX_LENGTH  (16)
#define SIGN_MAX_LENGTH (32)
#define STRING_CONCATENATION_MAX_LENGHT (100)

static duer_events_handler s_duer_http_dns_task_handle = NULL;
static duer_mutex_t s_http_dns_task_mutex;
static duer_mutex_t s_http_dns_query_mutex = NULL;
static duer_u32_t s_http_dns_query_time_start;
static duer_http_client_t *s_http_dns_client = NULL;
// int64_t g_acc_tick;
// int64_t g_time_offset;
// const char *g_http_dns_host_name = "http://httpdns.baidubce.com";
static const int HTTP_DNS_REQ_BUFFER_SIZE = 512;

//Please apply for the account password on Baidu Cloud.
//Refer to the following URL address
//https://cloud.baidu.com/product/httpdns.html
static const char *s_http_account_id = NULL;
static const char *s_http_dns_secret = NULL;
static const char *s_http_dns_protocol = "http://";
static const char *s_http_dns_host_name = "180.76.76.200";
static const char *s_http_dns_account_id_base = "/v3/resolve?account_id=";
static const char *s_http_dns_dn_base = "&dn=";
static const char *s_http_dns_time_base = "&t=";
static const char *s_http_dns_sign_base = "&sign=";
static const char *s_http_dns_data_key = "data";
static const char *s_http_dns_client_ip_key = "clientip";
static const char *s_http_dns_time_stamp_key = "timestamp";
static const char *s_http_dns_ip_key = "ip";
static const char *s_http_dns_ttl_key = "ttl";
static duer_http_dns_query_state_t s_http_dns_status = HTTP_DNS_QUERY_UNKNOWN;
static char s_http_dns_rsp_ip[HTTP_DNS_RSP_IP_SIZE] = {0};

static int http_dns_rsp_data_parse(const char *buffer, size_t len)
{
    baidu_json *json_data = NULL;
    baidu_json *json_client_ip = NULL;
    baidu_json *json_time_stamp = NULL;
    baidu_json *json_host = NULL;
    baidu_json *json_ip_ttl = NULL;
    baidu_json *json_ip_array = NULL;
    baidu_json *ip_item = NULL;
    baidu_json *json_ttl = NULL;
    char current_host_key[100] = {0};
    int array_count = 0;
    int size = 0;
    int ret = 0;

    do {
        if (buffer == NULL || len == 0) {
            DUER_LOGE("%s, GET http dns response len:%d", __func__, len);
            ret = -1;
            break;
        }

        json_data = baidu_json_Parse(buffer);

        if (json_data == NULL) {
            DUER_LOGE("%s, GET http dns response parse fail", __func__);
            ret = -1;
            break;
        }

        json_client_ip = baidu_json_GetObjectItem(json_data, s_http_dns_client_ip_key);

        if (json_client_ip == NULL) {
            DUER_LOGE("%s, GET http client ip parse fail", __func__);
            ret = -1;
            break;
        }

        //DUER_LOGE("client ip:%s", json_client_ip->valuestring);

        json_time_stamp = baidu_json_GetObjectItem(json_data, s_http_dns_time_stamp_key);

        if (json_time_stamp == NULL) {
            DUER_LOGE("%s, GET time stamp parse fail", __func__);
            ret = -1;
            break;
        }

        // DUER_LOGI("json_time_stamp :%d", json_time_stamp->valueint);

        /*data json*/
        json_host = baidu_json_GetObjectItem(json_data, s_http_dns_data_key);

        if (json_host == NULL) {
            DUER_LOGE("%s, GET http dns host parse fail", __func__);
            ret = -1;
            break;
        }

        if (DUER_SSCANF(buffer, "%*[^,],%*[^:{]:{\"%[^\"]", current_host_key) != 1) {
            DUER_LOGI("return error, no host name");
            ret = -1;
            break;
        }

        json_ip_ttl = baidu_json_GetObjectItem(json_host, current_host_key);

        if (json_ip_ttl == NULL) {
            DUER_LOGI("%s, GET http dns ip and ttl parse fail", __func__);
            ret = -1;
            break;
        }

        json_ip_array = baidu_json_GetObjectItem(json_ip_ttl, s_http_dns_ip_key);

        if (json_ip_array == NULL) {
            DUER_LOGI("%s, GET http dns ip array parse fail", __func__);
            ret = -1;
            break;
        }

        array_count = baidu_json_GetArraySize(json_ip_array);

        for (int i = 0; i < array_count; i++) {
            ip_item = baidu_json_GetArrayItem(json_ip_array, i);

            if (ip_item == NULL) {
                DUER_LOGI("%s, GET http dns ip parse fail", __func__);
                ret = -1;
                break;
            }

            if (ip_item->valuestring == NULL) {
                DUER_LOGI("%s, GET http dns ip parse fail", __func__);
                ret = -1;
                break;
            } else {
                //Note:temporarily handle the first set of IP.         
                DUER_SNPRINTF(s_http_dns_rsp_ip,
                              (HTTP_DNS_RSP_IP_SIZE - 1),
                              "%s",
                              ip_item->valuestring);
                DUER_LOGI("parse ip is:%s", s_http_dns_rsp_ip);
                break;
            }
        }

        if (ret == -1) {
            break;
        }

        json_ttl = baidu_json_GetObjectItem(json_ip_ttl, s_http_dns_ttl_key);

        if (json_ttl == NULL) {
            DUER_LOGI("%s, GET http dns ttl parse fail", __func__);
            ret = -1;
            break;
        }

        // DUER_LOGI ("ttl:%d", json_ttl->valueint);
        ret = 0;
    } while (0);

    if (json_data) {
        baidu_json_Delete(json_data);
    }

    return ret;
}

static void set_http_dns_query_state(duer_http_dns_query_state_t state)
{
    s_http_dns_status = state;
    // switch(state)
    // {
    // case HTTP_DNS_QUERY_UNKNOWN:
    //     s_http_dns_status = HTTP_DNS_QUERY_UNKNOWN;
    // break;
    // case HTTP_DNS_QUERY_START:
    //     s_http_dns_status = HTTP_DNS_QUERY_START;
    // break;
    // case HTTP_DNS_QUERYING:
    //      s_http_dns_status = HTTP_DNS_QUERYING;
    // break;
    // case HTTP_DNS_QUERY_SUCC:
    //      s_http_dns_status = HTTP_DNS_QUERY_SUCC;
    // break;
    // case HTTP_DNS_QUERY_FAIL:
    //      s_http_dns_status = HTTP_DNS_QUERY_FAIL;
    // break;
    // default:
    //     // do nothing
    // break;
    // }
}
/**
 * {"clientip":"58.33.165.244","data":{"res.iot.baidu.com":{"ip":["180.97.104.162"],"ttl":300}},"msg":"ok","timestamp":1531407249}
 * [http_dns_request_callback description]
 * @param  p_user_ctx [description]
 * @param  pos        [description]
 * @param  buf        [description]
 * @param  len        [description]
 * @param  type       [description]
 * @return            [description]
 */
static int http_dns_request_callback(void *p_user_ctx, duer_http_data_pos_t pos,
                                     const char *buffer, size_t len, const char *type)
{
    if (buffer == NULL || len == 0) {
        return 0;
    }

    DUER_LOGI("%s,data:%s,len:%d", __func__, buffer, len);

    if (!http_dns_rsp_data_parse(buffer, len)) {
        set_http_dns_query_state(HTTP_DNS_QUERY_SUCC);
        // g_time_offset = (esp_timer_get_time() - g_acc_tick) / 1000;
        // g_acc_tick = esp_timer_get_time();
        // DUER_LOGE("%s,consume total time:%lld \r\n", __func__, g_time_offset);
    } else {
        set_http_dns_query_state(HTTP_DNS_QUERY_FAIL);
        DUER_MEMSET(s_http_dns_rsp_ip, 0x0, HTTP_DNS_RSP_IP_SIZE);
    }

    return 0;
}

static char *construct_http_dns_req_url(char *host_name)
{
    char *url = NULL;
    char *ptr = NULL;
    unsigned char in_buf[STRING_CONCATENATION_MAX_LENGHT];
    unsigned char md5sum[MD5_SUM_MAX_LENGTH];
    unsigned char sign[SIGN_MAX_LENGTH] = {0};
    // DuerTime cur_time;
    duer_u32_t valid_timestamp = 1866666666;
    int ret = 0;
    int i = 0;

    if (s_http_account_id ==  NULL || s_http_dns_secret == NULL)  {
        DUER_LOGW("Warning, account id or http dns secret is null.");
        return NULL;
    }

    // if(duer_ntp_client(NULL, 3000, &cur_time, NULL) < 0){
    //     DUER_LOGW("Warning, get ntp time fail.");
    //     return NULL;
    // }
    //set sign invalid timeout 300s
    // cur_time.sec += 300;
    if (host_name == NULL) {
        DUER_LOGW("Warning, host name is null.");
        return NULL;
    }

    if (DUER_STRLEN(host_name) <= 0) {
        DUER_LOGW("Warning, get current host fail.");
        return NULL;
    }

    if (DUER_SNPRINTF(in_buf, STRING_CONCATENATION_MAX_LENGHT, "%s-%s-%d", host_name,
                      s_http_dns_secret, valid_timestamp) < 0) {
        DUER_LOGW("Warning, get input buffer fail.");
        return NULL;
    }

    mbedtls_md5(in_buf, DUER_STRLEN(in_buf), md5sum);
    ptr = sign;

    for (i = 0; i < MD5_SUM_MAX_LENGTH; i++) {
        if (DUER_SNPRINTF(ptr, SIGN_MAX_LENGTH, "%02x", md5sum[i]) < 0) {
            DUER_LOGW("Warning, get sign fail.");
            return NULL;
        }

        ptr += 2;
    }

    url = (char *)DUER_MALLOC(HTTP_DNS_REQ_BUFFER_SIZE);

    if (url == NULL) {
        DUER_LOGW("Warning, malloc fail, can not construct http dns url.");
        return NULL;
    }

    ret = DUER_SNPRINTF(url,
                        HTTP_DNS_REQ_BUFFER_SIZE - 1,
                        "%s%s%s%s%s%s%s%d%s%s", s_http_dns_protocol,
                        s_http_dns_host_name,
                        s_http_dns_account_id_base,
                        s_http_account_id,
                        s_http_dns_dn_base,
                        host_name,
                        s_http_dns_time_base,
                        valid_timestamp,
                        s_http_dns_sign_base,
                        sign);

    if (ret < 0) {
        DUER_LOGW("Warning,construct http dsn url fail.");

        if (url) {
            DUER_FREE(url);
            url = NULL;
        }

        return NULL;
    }

    return url;
}

/**
 * [duer_http_dns_check_stop description]
 * @return [1: to stop; 0: no stop]
 */
int duer_http_dns_check_stop()
{
    duer_u32_t delta_time = (duer_timestamp() - s_http_dns_query_time_start);

    if (delta_time > HTTP_DNS_REQ_TIMEOUT) {
        DUER_LOGW("Warning, delta_time:%d timeout, need stop!", delta_time);
        return 1;
    }

    if (s_http_dns_status == HTTP_DNS_QUERY_SUCC
            || s_http_dns_status == HTTP_DNS_QUERY_FAIL) {
        DUER_LOGI("http dns request result stop.");
        return 1;
    }

    // DUER_LOGI("delta_time:%d , go on...", delta_time);
    return 0;
}

static void duer_http_dns_task_run(int what, void *object)
{
    int ret = 0;
    duer_http_result_t status = DUER_HTTP_ERR_FAILED;
    // duer_http_client_t* client = NULL;
    char *host = (char *)object;
    char *url = NULL;
    DUER_LOGE("%s", __func__);

    if (s_http_dns_task_mutex == NULL) {
        DUER_LOGW("http dns has finalized,s_http_dns_task_mutex:%p", s_http_dns_task_mutex);
        return;
    }

    duer_mutex_lock(s_http_dns_task_mutex);
    set_http_dns_query_state(HTTP_DNS_QUERYING);
    DUER_MEMSET(s_http_dns_rsp_ip, 0x0, HTTP_DNS_RSP_IP_SIZE);
    s_http_dns_query_time_start = duer_timestamp();

    if (s_http_dns_client == NULL) {
        s_http_dns_client = duer_create_http_client();
    }

    if (s_http_dns_client == NULL) {
        DUER_LOGE("Warning, create http dns client fail.");
        set_http_dns_query_state(HTTP_DNS_QUERY_FAIL);
    } else {
        duer_http_reg_data_hdlr(s_http_dns_client, http_dns_request_callback, NULL);
        url = construct_http_dns_req_url(host);

        if (url == NULL) {
            DUER_LOGE("Warning, construct http dns req fail.");
            set_http_dns_query_state(HTTP_DNS_QUERY_FAIL);
        } else {
            DUER_LOGI("get url:%s", url);
            duer_http_reg_stop_notify_cb(s_http_dns_client, duer_http_dns_check_stop);
            status = duer_http_get(s_http_dns_client, url, 0, 0);

            if (status != DUER_HTTP_OK) {
                DUER_LOGE("Warning, http dns request failed:%d", status);
                DUER_MEMSET(s_http_dns_rsp_ip, 0x0, HTTP_DNS_RSP_IP_SIZE);
                set_http_dns_query_state(HTTP_DNS_QUERY_FAIL);
            } else {
                DUER_LOGE("http dns check stop, request result:%d", status);
            }
        }
    }

    if (s_http_dns_client != NULL) {
        duer_http_destroy(s_http_dns_client);
        DUER_FREE(s_http_dns_client);
        s_http_dns_client = NULL;
    }

    if (url != NULL) {
        DUER_FREE(url);
        url = NULL;
    }

    duer_mutex_unlock(s_http_dns_task_mutex);
}

char *duer_http_dns_query(char *host, int timeout)
{
    DUER_LOGD("%s", __func__);

    if (host == NULL) {
        return NULL;
    }

    if (DUER_STRLEN(host) == 0) {
        DUER_LOGW("host size is zero.");
        return NULL;
    }

    if (s_duer_http_dns_task_handle == NULL) {
        DUER_LOGW("s_duer_http_dns_task_handle is null");
        return NULL;
    }

    if (s_http_dns_query_mutex == NULL) {
        DUER_LOGW("s_http_dns_query_mutex:%p", s_http_dns_query_mutex);
        return NULL;
    }

    // xSemaphoreGive(s_http_dns_task_sem);
    // xSemaphoreTake(s_http_dns_query_sem, timeout / portTICK_PERIOD_MS);
    duer_mutex_lock(s_http_dns_query_mutex);
    set_http_dns_query_state(HTTP_DNS_QUERY_START);
    duer_events_call(s_duer_http_dns_task_handle, duer_http_dns_task_run, timeout, (void *)host);
    int period = 0;
    int count = (timeout / 50);

    do {
        duer_sleep(50);

        if (s_http_dns_status == HTTP_DNS_QUERY_SUCC) {
            if (DUER_STRLEN(s_http_dns_rsp_ip) != 0) {
                DUER_LOGI("http dns query succss, ip is %s", s_http_dns_rsp_ip);
                duer_mutex_unlock(s_http_dns_query_mutex);
                return s_http_dns_rsp_ip;
            } else {
                DUER_LOGW("http dns query not find.");
                duer_mutex_unlock(s_http_dns_query_mutex);
                return NULL;
            }
        }

        if (s_http_dns_status == HTTP_DNS_QUERY_FAIL) {
            DUER_LOGW("http dns query fail.");
            duer_mutex_unlock(s_http_dns_query_mutex);
            return NULL;
        }

        if (++period > count) {
            DUER_LOGW("http dns query timeout.");
            duer_mutex_unlock(s_http_dns_query_mutex);
            return NULL;
        }
    } while (1);
}

void duer_http_dns_task_init()
{
    if (s_duer_http_dns_task_handle == NULL) {
        s_duer_http_dns_task_handle = duer_events_create("http_dns_task", 1024 * 3, 5);
    }

    if (s_http_dns_task_mutex == NULL) {
        s_http_dns_task_mutex = duer_mutex_create();
    }

    if (s_http_dns_query_mutex == NULL) {
        s_http_dns_query_mutex = duer_mutex_create();
    }
}

void duer_http_dns_task_destroy()
{
    if (s_duer_http_dns_task_handle != NULL) {
        duer_events_destroy(s_duer_http_dns_task_handle);
        s_duer_http_dns_task_handle = NULL;
    }

    if (s_http_dns_task_mutex != NULL) {
        duer_mutex_destroy(s_http_dns_task_mutex);
        s_http_dns_task_mutex = NULL;
    }

    if (s_http_dns_query_mutex != NULL) {
        duer_mutex_destroy(s_http_dns_query_mutex);
        s_http_dns_query_mutex = NULL;
    }
}
#endif
