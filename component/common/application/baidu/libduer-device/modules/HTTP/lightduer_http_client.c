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
 * File: lightduer_http_client.c
 * Author: Pan Haijun, Gang Chen(chengang12@baidu.com)
 * Desc: HTTP Client
 */

#include "lightduer_http_client.h"
#include <string.h>
#include "lightduer_log.h"
#include "lightduer_ds_log_http.h"
#include "lightduer_timestamp.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_ds_log.h"

const char *duer_engine_get_rsa_cacrt(void);

#define HC_CHECK_ERR(p_client, ret, log_str, http_res) \
    do{ \
        if(ret) { \
            DUER_LOGE(log_str "socket error (%d)", ret); \
            return http_res; \
        } \
    } while(0)

static const int DUER_HTTP_CR_LF_LEN               = 2;          //DUER_STRLEN("\r\n")
static const int DUER_HTTP_SCHEME_LEN_MAX          = 8;
static const int DUER_HTTP_HOST_LEN_MAX            = 255;
static const int DUER_HTTP_REDIRECTION_MAX         = 30;
static const int DUER_HTTP_TIME_OUT                = (2 * 1000); //TIME OUT IN MILI-SECOND
static const int DUER_HTTP_TIME_OUT_TRIES          = 30;
static const int DUER_HTTP_REQUEST_CMD_LEN_MAX     = 6;
static const int DUER_HTTP_CHUNK_SIZE              = 1025;
static const int DUER_HTTP_RSP_CHUNK_SIZE          = 100;
static const int DUER_HTTP_RESUME_COUNT_MAX        = 10;
static const int DUER_HTTP_RANGE_LEN_MAX           = 10;

static const int DUER_SCHEME_HTTP = 0;
static const int DUER_SCHEME_HTTPS = 1;
static const int DUER_DEFAULT_HTTP_PORT = 80;
static const int DUER_DEFAULT_HTTPS_PORT = 443;

const char *s_hc_request_cmd[] = {
    "GET",      //DUER_HTTP_GET
    "POST",     //DUER_HTTP_POST
    "PUT",      //DUER_HTTP_PUT
    "DELETE",   //DUER_HTTP_DELETE
    "HEAD",     //DUER_HTTP_HEAD
};

enum duer_http_response_code {
    DUER_HTTP_MIN_VALID_RSP_CODE       = 200,
    DUER_HTTP_PARTIAL_CONTENT_RSP_CODE = 206,
    DUER_HTTP_MIN_INVALID_RSP_CODE     = 400,
};

duer_http_result_t duer_http_init_socket_ops(duer_http_client_t *p_client,
        duer_http_socket_ops_t *p_ops,
        void *socket_args)
{
    DUER_LOGV("entry\n");

    if (!p_client || !p_ops) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Args err!\n");
        return DUER_HTTP_ERR_FAILED;
    }

    DUER_MEMMOVE(&p_client->ops, p_ops, sizeof(duer_http_socket_ops_t));

    if (!p_client->ops.init) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("http sokcet ops.init is null!\n");
        return DUER_HTTP_ERR_FAILED;
    }

    p_client->ops.n_handle = p_client->ops.init(socket_args);

    if (p_client->ops.n_handle == 0) {
        duer_ds_log_http(DUER_DS_LOG_HTTP_SOCKET_INIT_FAILED, NULL);
        DUER_LOGE("http sokcet ops.init initialize failed!\n");
        return DUER_HTTP_ERR_FAILED;
    }

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

static void http_client_close_socket_connect(duer_http_client_t *p_client)
{
    if (p_client->last_host) {
        DUER_FREE(p_client->last_host);
        p_client->last_host = NULL;
        p_client->ops.close(p_client->ops.n_handle);
    }
}

duer_http_result_t duer_http_init(duer_http_client_t *p_client)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Args err: p_client is null!\n");
        return DUER_HTTP_ERR_FAILED;
    }

    DUER_MEMSET(p_client, 0, sizeof(duer_http_client_t));

    duer_ds_log_http(DUER_DS_LOG_HTTP_INIT, NULL);

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

void duer_http_destroy(duer_http_client_t *p_client)
{
    DUER_LOGD("entry,%p\n", p_client);

    if (!p_client) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Args err: p_client is null!\n");
        return;
    }

    if (p_client->connect_keep_timer) {
        duer_timer_stop(p_client->connect_keep_timer);
        duer_timer_release(p_client->connect_keep_timer);
        p_client->connect_keep_timer = NULL;
    }

    http_client_close_socket_connect(p_client);

    DUER_LOGI("duer_http_destroy");
    p_client->ops.destroy(p_client->ops.n_handle);
    p_client->ops.n_handle = 0;

    duer_ds_log_http(DUER_DS_LOG_HTTP_DESTROYED, NULL);
    DUER_LOGV("leave\n");
    return;
}

void duer_http_set_cust_headers(duer_http_client_t *p_client, const char **headers, size_t pairs)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_LOGE("Args err: p_client is null!\n");
        return;
    }

    p_client->pps_custom_headers = headers;
    p_client->sz_headers_count = pairs;

    DUER_LOGV("leave\n");
}

void duer_http_reg_data_hdlr(duer_http_client_t *p_client,
                             duer_http_data_handler data_hdlr_cb,
                             void *p_usr_ctx)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_LOGE("Args err: p_client is null!\n");
        return;
    }

    p_client->data_hdlr_cb = data_hdlr_cb;
    p_client->p_data_hdlr_ctx = p_usr_ctx;

    DUER_LOGV("leave\n");
}

void duer_http_reg_url_get_cb(duer_http_client_t *p_client, duer_http_get_url_cb_t cb)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_LOGE("Args err: p_client is null!\n");
        return;
    }

    p_client->get_url_cb = cb;

    DUER_LOGV("leave\n");
}

void duer_http_reg_stop_notify_cb(duer_http_client_t *p_client,
                                  duer_http_stop_notify_cb_t chk_stp_cb)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_LOGE("Args err: p_client is null!\n");
        return;
    }

    p_client->check_stop_notify_cb = chk_stp_cb;

    DUER_LOGV("leave\n");
}

int duer_http_get_rsp_code(duer_http_client_t *p_client)
{
    DUER_LOGV("entry\n");

    if (!p_client) {
        DUER_LOGE("Args err: p_client is null!\n");
        return -1;
    }

    DUER_LOGV("leave\n");
    return p_client->n_http_response_code;
}

/**
 * DESC:
 * parse http url
 *
 * PARAM:
 * @param[in]  url:             url to be parsed
 * @param[out] scheme:          protocal scheme buffer
 * @param[in]  scheme_len:      length of arg "scheme"
 * @param[out] host:            http host name buffer
 * @param[in]  host_len:        length of arg "host"
 * @param[out] port:            http port number
 * @param[out] path:            http path buffer
 * @param[in]  path_len:        length of arg "path"
 * @param[in]  is_relative_url: the url is a relative url or not
 *                              a relative url do not have host/port,
 *                              the previous url's host/port should be used
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_parse_url(const char *url,
        char *scheme,
        size_t scheme_len,
        char *host,
        unsigned short *port,
        char *path,
        size_t path_len,
        duer_bool is_relative_url)
{
    size_t r_host_len = 0;
    size_t r_path_len = 0;
    char  *p_path     = NULL;
    char  *p_fragment = NULL;
    char  *p_host     = NULL;
    char  *p_scheme   = (char *) url;

    DUER_LOGV("entry\n");

    if (is_relative_url) {
        p_path = (char *)url;
    } else {
        p_host = (char *)DUER_STRSTR(url, "://");

        if (p_host == NULL) {
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Could not find host");
            return DUER_HTTP_ERR_PARSE; //URL is invalid
        }

        //to find http scheme
        if ((int)scheme_len < p_host - p_scheme + 1) { //including NULL-terminating char
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Scheme str is too small (%d >= %d)", scheme_len, p_host - p_scheme + 1);
            return DUER_HTTP_ERR_PARSE;
        }

        DUER_MEMMOVE(scheme, p_scheme, p_host - p_scheme);
        scheme[p_host - p_scheme] = '\0';

        //to find port
        p_host += DUER_STRLEN("://");
        char *p_port = DUER_STRCHR(p_host, ':');

        //to find host
        p_path = DUER_STRCHR(p_host, '/');

        if (p_port != NULL) {
            // the port is only follow the ":" before "/"
            if ((p_path == NULL) || (p_port < p_path)) {
                r_host_len = p_port - p_host;
                p_port++;

                if (DUER_SSCANF(p_port, "%hu", port) != 1) {
                    DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                    DUER_LOGE("Could not find port");
                    return DUER_HTTP_ERR_PARSE;
                }
            }
        } else {
            *port = 0;
        }

        if (r_host_len == 0) {
            if (!p_path) {
                r_host_len = DUER_STRLEN(p_host);
            } else {
                r_host_len = p_path - p_host;
            }
        }

        if (r_host_len > DUER_HTTP_HOST_LEN_MAX) {
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Host is too long(%d > %d)!", r_host_len, DUER_HTTP_HOST_LEN_MAX);
            return DUER_HTTP_ERR_PARSE;
        }

        DUER_MEMCPY(host, p_host, r_host_len);
        host[r_host_len] = '\0';
    }

    if (p_path) {
        //to find path
        p_fragment = DUER_STRCHR(p_path, '#');

        if (p_fragment != NULL) {
            r_path_len = p_fragment - p_path;
        } else {
            r_path_len = DUER_STRLEN(p_path);
        }

        if (path_len < r_path_len + 1) { //including NULL-terminating char
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Path str is too small (%d >= %d)", path_len, r_path_len + 1);
            return DUER_HTTP_ERR_PARSE;
        }

        DUER_MEMMOVE(path, p_path, r_path_len);
        path[r_path_len] = '\0';
    } else {
        if (path_len < 2) { // "/" is needed if there is no path
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Path str is too small (%d >= 2)", path_len);
            return DUER_HTTP_ERR_PARSE;
        }
        path[0] = '/';
        path[1] = '\0';
    }

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * wrapper socket "send" to send data
 *
 * PARAM:
 * @param[in] p_client: pointer of the http client
 * @param[in] buf:      data to be sent
 * @param[in] len:      data length to be sent, if it's 0, then it equals the 'buf' 's length
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_send(duer_http_client_t *p_client,
        const char *buf,
        size_t len)
{
    size_t written_len = 0;
    int    ret         = 0;

    DUER_LOGV("entry\n");

    if (len == 0) {
        len = DUER_STRLEN(buf);
    }

    DUER_LOGD("send %d bytes:(%s)\n", len, buf);

    p_client->ops.set_blocking(p_client->ops.n_handle, true);
    ret = p_client->ops.send(p_client->ops.n_handle, buf, len);

    if (ret > 0) {
        written_len += ret;
        p_client->upload_size += ret;
    } else if (ret == 0) {
        duer_ds_log_http(DUER_DS_LOG_HTTP_CONNECT_CLOSED_BY_SERVER, NULL);
        DUER_LOGE("Connection was closed by server");
        return DUER_HTTP_CLOSED;
    } else {
        duer_ds_log_http_report_err_code(DUER_DS_LOG_HTTP_SEND_FAILED, ret);
        DUER_LOGE("Connection error (send returned %d)", ret);
        return DUER_HTTP_ERR_CONNECT;
    }

    DUER_LOGD("Written %d bytes", written_len);

    DUER_LOGV("entry\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * wrapper socket "recv" to receive data
 *
 * PARAM:
 * @param[in]  p_client:   pointer of the http client
 * @param[in]  buf:        a buffer to store received data
 * @param[in]  max_len:    max size needed to read
 * @param[out] p_read_len: really size the data is read
 * @param[in]  to_try:     check to try or not when timeout
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv(duer_http_client_t *p_client,
        char *buf,
        size_t max_len,
        size_t *p_read_len,
        int to_try)
{
    int    ret             = 0;
    int    n_try_times     = 0;
    size_t sz_read_len     = 0;
    duer_http_result_t res      = DUER_HTTP_OK;

    DUER_LOGV("entry\n");

    DUER_LOGV("Trying to read %d bytes", max_len);

    while (sz_read_len < max_len) {
        DUER_LOGD("Trying(%d ) times to read at most %4d bytes [Not blocking] %d,%d",
                  n_try_times, max_len - sz_read_len, max_len, sz_read_len);
        p_client->ops.set_blocking(p_client->ops.n_handle, true);
        p_client->ops.set_timeout(p_client->ops.n_handle, DUER_HTTP_TIME_OUT);
        ret = p_client->ops.recv(p_client->ops.n_handle, buf + sz_read_len, max_len - sz_read_len);

        if (ret > 0) {
            sz_read_len += ret;
        } else if (ret == 0) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_CONNECT_CLOSED_BY_SERVER, NULL);
            res = DUER_HTTP_CLOSED;
            break;
        } else {
            if (ret != DUER_ERR_TRANS_WOULD_BLOCK) {
                DUER_LOGI("http recv error:%d", ret);
                duer_ds_log_http_report_err_code(DUER_DS_LOG_HTTP_RECEIVE_FAILED, ret);
                res = DUER_HTTP_ERR_CONNECT;
                break;
            }
        }

        if (!to_try || (sz_read_len == max_len)) {
            break;
        } else {
            n_try_times++;
            if (n_try_times >= DUER_HTTP_TIME_OUT_TRIES) {
                res = DUER_HTTP_ERR_TIMEOUT;
                break;
            }
        }

        if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
            DUER_LOGI("Stopped getting media data from url by stop flag!\n");
            break;
        }
    }

    DUER_LOGV("Read %d bytes", sz_read_len);

    buf[sz_read_len] = '\0';
    *p_read_len = sz_read_len;

    DUER_LOGV("leave\n");
    return res;
}

/**
 * DESC:
 * send http request header
 *
 * PARAM:
 * @param[in] p_client: pointer of the http client
 * @param[in] method:   method to comunicate with server
 * @param[in] path:     resource path
 * @param[in] host:     server host
 * @param[in] port:     server port
 *
 * RETURN     DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_send_request(duer_http_client_t *p_client,
        duer_http_method_t method,
        char *path,
        char *host,
        unsigned short port)
{
    int   ret           = 0;
    int   req_fmt_len   = 0;
    char *p_request_buf = NULL;

    DUER_LOGV("entry");

    req_fmt_len = DUER_STRLEN("%s %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nRange: bytes=%d-\r\n");
    req_fmt_len += DUER_HTTP_REQUEST_CMD_LEN_MAX + DUER_STRLEN(path) + DUER_STRLEN(host) \
                   + DUER_HTTP_RANGE_LEN_MAX;

    p_request_buf = (char *)DUER_MALLOC(req_fmt_len);

    if (!p_request_buf) {
        DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
        DUER_LOGE("Mallco memory(size:%d) for request buffer Failed!\n", req_fmt_len);
        return DUER_HTTP_ERR_FAILED;
    }

    DUER_MEMSET(p_request_buf, 0, req_fmt_len);

    if (p_client->recv_size > 0) {
        DUER_SNPRINTF(p_request_buf,
                      req_fmt_len,
                      "%s %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nRange: bytes=%d-\r\n",
                      s_hc_request_cmd[method],
                      path,
                      host,
                      p_client->recv_size);
    } else {
        DUER_SNPRINTF(p_request_buf, req_fmt_len, "%s %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\n",
                      s_hc_request_cmd[method], path, host);
    }

    DUER_LOGD(" request buffer %d bytes{%s}", req_fmt_len, p_request_buf);

    ret = duer_http_send(p_client, p_request_buf, 0);
    DUER_FREE(p_request_buf);

    if (ret != DUER_HTTP_OK) {
        duer_ds_log_http_report_err_code(DUER_DS_LOG_HTTP_SEND_FAILED, ret);
        DUER_LOGE("Could not write request");
        return DUER_HTTP_ERR_CONNECT;
    }

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * sending all http headers
 *
 * PARAM:
 * @param[in] p_client:  pointer of the http client
 * @param[in] buff:      provide buffer to send headers
 * @param[in] buff_size: 'buff' 's size
 *
 * RETURN     DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_send_headers(duer_http_client_t *p_client,
        char *buff,
        int buff_size)
{
    size_t nh         = 0;
    int    header_len = 0;
    duer_http_result_t ret = DUER_HTTP_OK;

    DUER_LOGV("enrty\n");

    DUER_LOGD("Send custom header(s) %d (if any)", p_client->sz_headers_count);

    for (nh = 0; nh < p_client->sz_headers_count * 2; nh += 2) {
        DUER_LOGD("hdr[%2d] %s: %s", nh, p_client->pps_custom_headers[nh],
                  p_client->pps_custom_headers[nh + 1]);

        header_len = DUER_STRLEN("%s: %s\r\n") + DUER_STRLEN(p_client->pps_custom_headers[nh]) \
                     + DUER_STRLEN(p_client->pps_custom_headers[nh + 1]);

        if (header_len >= buff_size) {
            DUER_LOGD("Http header (%d) is too long!", header_len);
            return DUER_HTTP_ERR_FAILED;
        }

        DUER_SNPRINTF(buff, buff_size, "%s: %s\r\n", p_client->pps_custom_headers[nh],
                      p_client->pps_custom_headers[nh + 1]);
        ret = duer_http_send(p_client, buff, 0);
        HC_CHECK_ERR(p_client, ret, "Could not write headers", DUER_HTTP_ERR_CONNECT);
        DUER_LOGD("send() returned %d", ret);
    }

    //Close headers
    DUER_LOGD("Close Http Headers");
    ret = duer_http_send(p_client, "\r\n", 0);
    HC_CHECK_ERR(p_client, ret, "Close http headers failed!", ret);

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * set http content length
 *
 * PARAM:
 * @param[in] content_len: http content lenght to be set
 *
 * RETURN     DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static void duer_http_set_content_len(duer_http_client_t *p_client, int content_len)
{
    DUER_LOGV("enrty\n");
    p_client->n_http_content_len = content_len;
    DUER_LOGV("leave\n");
}

/**
 * DESC:
 * set http content type
 *
 * PARAM:
 * @param[in] p_client:  pointer of the http client
 * @param[in] p_content_type: http content type to be set
 *
 * RETURN     DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static void duer_http_set_content_type(duer_http_client_t *p_client, char *p_content_type)
{
    DUER_LOGV("enrty\n");

    if (p_content_type) {
        DUER_SNPRINTF(p_client->p_http_content_type,
                      DUER_HTTP_CONTENT_TYPE_LEN_MAX,
                      "%s",
                      p_content_type);

        if (DUER_STRLEN(p_content_type) >= DUER_HTTP_CONTENT_TYPE_LEN_MAX) {
            p_client->p_http_content_type[DUER_HTTP_CONTENT_TYPE_LEN_MAX - 1] = '\0';
        }
    }

    DUER_LOGV("leave\n");
}

/**
 * DESC:
 * receive http response and parse it
 *
 * PARAM:
 * @param[in]     p_client:   pointer of the http client
 * @param[in/out] buf:        [in]pass a buffer to store received data;
 *                            [out]to store the remaining received data except http response
 * @param[in]     buf_size:   size of buffer
 * @param[out]    p_read_len: the remaining size of the data in 'buf'
 *
 * RETURN         DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv_rsp_and_parse(duer_http_client_t *p_client,
        char *buf,
        size_t buf_size,
        size_t *p_read_len)
{
    int    ret       = 0;
    size_t cr_lf_pos = 0;
    size_t rev_len   = 0;
    size_t max_len = buf_size - 1;
    char  *p_cr_lf   = NULL;

    DUER_LOGV("enrty\n");

    ret = duer_http_recv(p_client, buf, max_len, &rev_len, 1);

    DUER_LOGD("response received: %s\n", buf);

    if (ret != DUER_HTTP_OK) {
        duer_ds_log_http_report_err_code(DUER_DS_LOG_HTTP_RESPONSE_RECEIVE_FAILED, ret);
        HC_CHECK_ERR(p_client, ret, "Receiving  http response error!", DUER_HTTP_ERR_CONNECT);
    }

    p_cr_lf = DUER_STRSTR(buf, "\r\n");

    if (!p_cr_lf) {
        DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
        HC_CHECK_ERR(p_client,
                     DUER_HTTP_ERR_PRTCL,
                     "Response Protocol error",
                     DUER_HTTP_ERR_PRTCL);
    }

    cr_lf_pos = p_cr_lf - buf;
    buf[cr_lf_pos] = '\0';

    //to parse HTTP response
    if (DUER_SSCANF(buf, "HTTP/%*d.%*d %d %*[^\r\n]", &p_client->n_http_response_code) != 1) {
        DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
        //cannot match string, error
        DUER_LOGE("Not a correct HTTP answer : {%s}\n", buf);
        HC_CHECK_ERR(p_client,
                     DUER_HTTP_ERR_PRTCL,
                     "Response Protocol error",
                     DUER_HTTP_ERR_PRTCL);
    }

    if ((p_client->n_http_response_code < DUER_HTTP_MIN_VALID_RSP_CODE)
            || (p_client->n_http_response_code >= DUER_HTTP_MIN_INVALID_RSP_CODE)) {
        //Did not return a 2xx code;
        //TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers
        DUER_LOGE("Response code %d", p_client->n_http_response_code);
        duer_ds_log_http_report_err_code(DUER_DS_LOG_HTTP_RESPONSE_RECEIVE_FAILED,
                                         p_client->n_http_response_code);
        HC_CHECK_ERR(p_client,
                     DUER_HTTP_ERR_PRTCL,
                     "Response Protocol error",
                     DUER_HTTP_ERR_PRTCL);
    }

    if ((p_client->recv_size > 0)
            && (p_client->n_http_response_code == DUER_HTTP_MIN_VALID_RSP_CODE)) {
        DUER_LOGE("Server doesn't support Range");
        HC_CHECK_ERR(p_client,
                     DUER_HTTP_ERR_PRTCL,
                     "Response Protocol error",
                     DUER_HTTP_ERR_PRTCL);
    }

    rev_len -= (cr_lf_pos + DUER_HTTP_CR_LF_LEN);
    DUER_MEMMOVE(buf, &buf[cr_lf_pos + DUER_HTTP_CR_LF_LEN], rev_len);
    buf[rev_len] = '\0';

    if (p_read_len) {
        *p_read_len = rev_len;
    }

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * receive http response and parse it
 *
 * PARAM:
 * @param[in]     p_client:   pointer of the http client
 * @param[in/out] buf:        [in]pass a buffer to store received data;
 *                            [out]to store the remaining received data except http response
 * @param[in]     buf_size:   size of the buffer
 * @param[in/out] p_read_len: [in/out]the remaining size of the data in 'buf'
 *
 * RETURN         DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv_headers_and_parse(duer_http_client_t *p_client,
        char *buf,
        size_t buf_size,
        size_t *p_read_len)
{
    int    i             = 0;
    char  *key           = NULL;
    char  *value         = NULL;
    char  *p_cr_lf       = NULL;
    char  *new_location  = NULL;
    size_t len           = 0;
    size_t max_len       = buf_size - 1;
    size_t value_len     = 0;
    size_t cr_lf_pos     = 0;
    size_t rev_len       = 0;
    size_t new_rev_len   = 0;
    size_t location_len  = 0;
    size_t remain_pos    = 0;
    duer_http_result_t res    = DUER_HTTP_OK;
    int http_content_len = 0;
    duer_bool is_location_found = DUER_FALSE;
    duer_bool is_value_overlength = DUER_FALSE;

    DUER_LOGV("enrty\n");

    HC_CHECK_ERR(p_client, !p_read_len, "arg 'p_read_len' is null!", DUER_HTTP_ERR_FAILED);

    rev_len = *p_read_len;
    p_client->n_is_chunked = false;

    while (true) {
        p_cr_lf = DUER_STRSTR(buf, "\r\n");

        if (p_cr_lf == NULL) {
            if (rev_len < max_len) {
                res = duer_http_recv(p_client, buf + rev_len, max_len - rev_len,
                                     &new_rev_len, 0);
                rev_len += new_rev_len;
                buf[rev_len] = '\0';
                DUER_LOGD("total data:%d, new receive %d chars: [%s]", rev_len, new_rev_len, buf);

                if (res != DUER_HTTP_OK) {
                    DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                    HC_CHECK_ERR(p_client, res, "Receiving  http headers error!", res);
                }
                continue;
            }
        }

        key = NULL;
        value = NULL;

        if (p_cr_lf) {
            cr_lf_pos = p_cr_lf - buf;
            buf[cr_lf_pos] = '\0';
        }

        if (is_value_overlength) {
            value = buf;
        } else {
            //end of headers to break
            if (cr_lf_pos == 0) {
                DUER_LOGD("All Http Headers readed over!");
                rev_len -= DUER_HTTP_CR_LF_LEN;
                //Be sure to move NULL-terminating char as well
                DUER_MEMMOVE(buf, &buf[DUER_HTTP_CR_LF_LEN], rev_len + 1);
                break;
            }

            // parse the header
            for (i = 0; i < cr_lf_pos; i++) {
                if (buf[i] == ':') {
                    buf[i] = '\0';
                    key = buf;
                    value = &buf[i + 1];
                    break;
                }
            }

            if ((key == NULL) || (value == NULL)) {
                HC_CHECK_ERR(p_client,
                             DUER_HTTP_ERR_PRTCL,
                             "Could not parse header",
                             DUER_HTTP_ERR_PRTCL);
            }

            if (!p_cr_lf) {
                is_value_overlength = DUER_TRUE;
            }

            while ((*key != '\0') && ((*key == ' ') || (*key == '	'))) {
                    key++;
            }

            while ((*value != '\0') && ((*value == ' ') || (*value == '	'))) {
                    value++;
            }

            DUER_LOGD("Read header (%s: %s)", key, value);

            if (!DUER_STRNCASECMP(key, "Content-Length", max_len)) {
                DUER_SSCANF(value, "%d", &http_content_len);
                duer_http_set_content_len(p_client, http_content_len + p_client->recv_size);
                DUER_LOGI("Total size of the data need to read: %d bytes",
                          http_content_len + p_client->recv_size);
            } else if (!DUER_STRNCASECMP(key, "Transfer-Encoding", max_len)) {
                if (!DUER_STRNCASECMP(value, "Chunked", max_len - (value - buf))
                        || !DUER_STRNCASECMP(value, "chunked",  max_len - (value - buf))) {
                    p_client->n_is_chunked = true;

                    DUER_LOGI("Http client data is chunked transfored\n");

                    p_client->p_chunk_buf = (char *)DUER_MALLOC(DUER_HTTP_CHUNK_SIZE);
                    if (!p_client->p_chunk_buf) {
                        DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
                        HC_CHECK_ERR(p_client,
                                     !p_client->p_chunk_buf,
                                     "Malloc memory failed for p_chunk_buf!",
                                     DUER_HTTP_ERR_FAILED);
                    }
                }
            } else if (!DUER_STRNCASECMP(key, "Content-Type", max_len)) {
                duer_http_set_content_type(p_client, value);
            } else if (!DUER_STRNCASECMP(key, "Location", max_len)) {
                if (p_client->p_location) {
                    DUER_FREE(p_client->p_location);
                    p_client->p_location = NULL;
                }
                is_location_found = DUER_TRUE;
            } else {
                // do nothing
            }
        }

        if (p_cr_lf) {
            remain_pos = cr_lf_pos + DUER_HTTP_CR_LF_LEN;
        } else {
            // the latest character might be '\r', hence it should be processed next time
            remain_pos = rev_len - 1;
        }

        if (is_location_found) {
            if (p_cr_lf) {
                value_len = DUER_STRLEN(value);
            } else {
                value_len = DUER_STRLEN(value) - 1;
            }

            location_len = p_client->p_location ? DUER_STRLEN(p_client->p_location) : 0;
            len = value_len + location_len + 1;

            if (p_client->p_location) {
                new_location = DUER_REALLOC(p_client->p_location, len);
                if (!new_location) {
                    DUER_FREE(p_client->p_location);
                    p_client->p_location = NULL;
                    DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
                    HC_CHECK_ERR(p_client, DUER_HTTP_ERR_FAILED,
                                 "Malloc memory failed for p_location!", DUER_HTTP_ERR_FAILED);
                }
                p_client->p_location = new_location;
                DUER_STRNCAT(p_client->p_location, value, value_len);
            } else {
                p_client->p_location = DUER_MALLOC(len);
                if (!p_client->p_location) {
                    DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
                    HC_CHECK_ERR(p_client, DUER_HTTP_ERR_FAILED,
                                 "Malloc memory failed for p_location!", DUER_HTTP_ERR_FAILED);
                }
                DUER_SNPRINTF(p_client->p_location, len, "%s", value);
            }
        }

        // Including the NULL-Terminal
        DUER_MEMMOVE(buf, &buf[remain_pos], rev_len - remain_pos + 1);
        rev_len -= remain_pos;

        DUER_LOGD("remain_pos: %d, rev_len:%d", remain_pos, rev_len);

        if (p_cr_lf) {
            is_value_overlength = DUER_FALSE;
        }

        if (is_location_found && !is_value_overlength) {
            *p_read_len = rev_len;
            return DUER_HTTP_REDIRECTTION;
        }
    }

    *p_read_len = rev_len;

    DUER_LOGV("leave!remain data: %s\n", buf);
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * receive http chunked size, and save it into duer_http_client_t's 'sz_chunk_size'
 *
 * PARAM:
 * @param[in]     p_client:   pointer of the http client
 * @param[in/out] buf:        [in]pass a buffer to store received data;
 *                            [out]to store the remaining received data except http response
 * @param[in]     max_len:    max size needed to read each time
 * @param[in/out] p_read_len: [in&out]the remaining size of the data in 'buf'
 *
 * RETURN         DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv_chunked_size(duer_http_client_t *p_client,
        char *buf,
        size_t max_len,
        size_t *p_read_len)
{
    int    n               = 0;
    int    found_cr_lf     = false;
    size_t rev_len         = 0;
    size_t new_rev_len     = 0;
    size_t cr_lf_pos       = 0;
    duer_http_result_t res = DUER_HTTP_OK;

    DUER_LOGV("enrty\n");

    HC_CHECK_ERR(p_client, !p_read_len, "arg 'p_read_len' is null!", DUER_HTTP_ERR_FAILED);
    DUER_LOGD("args:max_len=%d, *p_read_len=%d", max_len, *p_read_len);

    rev_len = *p_read_len;

    while (!found_cr_lf) {
        if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
            DUER_LOGI("Stopped getting media data from url by stop flag!\n");
            return res;
        }

        // to check chunked size
        if (rev_len >= DUER_HTTP_CR_LF_LEN) {
            for (cr_lf_pos = 0; cr_lf_pos < rev_len - 2; cr_lf_pos++) {
                if (buf[cr_lf_pos] == '\r' && buf[cr_lf_pos + 1] == '\n') {
                    found_cr_lf = true;
                    DUER_LOGD("Found cr&lf at index %d", cr_lf_pos);
                    break;
                }
            }
        }

        if (found_cr_lf) {
            break;
        }

        //Try to read more
        if (rev_len < max_len) {
            res = duer_http_recv(p_client, buf + rev_len, max_len - rev_len, &new_rev_len, 0);
            rev_len += new_rev_len;
            buf[rev_len] = '\0';
            DUER_LOGD("Receive %d chars in buf: \n[%s]", new_rev_len, buf);

            if (res != DUER_HTTP_OK) {
                DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                HC_CHECK_ERR(p_client, res, "Receiving  http chunk headers error!", res);
            }
        } else {
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            HC_CHECK_ERR(p_client,
                         DUER_HTTP_ERR_PRTCL,
                         "Get http chunked size failed:not found cr lf in size line!",
                         DUER_HTTP_ERR_PRTCL);
        }
    }

    buf[cr_lf_pos] = '\0';
    DUER_LOGD("found_cr_lf=%d, cr_lf_pos=%d, buf=%s", found_cr_lf, cr_lf_pos, buf);

    //read chunked size
    n = DUER_SSCANF(buf, "%x", &p_client->sz_chunk_size);

    if (n != 1) {
        DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
        HC_CHECK_ERR(p_client,
                     DUER_HTTP_ERR_PRTCL,
                     "Get http chunked size failed: Could not read chunk length!",
                     DUER_HTTP_ERR_PRTCL);
    }

    DUER_LOGD("HTTP chunked size:%d\n", p_client->sz_chunk_size);

    rev_len -= (cr_lf_pos + DUER_HTTP_CR_LF_LEN);
    DUER_MEMMOVE(buf, &buf[cr_lf_pos + DUER_HTTP_CR_LF_LEN],
                 rev_len + 1); //Be sure to move NULL-terminating char as well
    *p_read_len = rev_len;

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

/**
 * DESC:
 * receiving http chunk data
 *
 * PARAM:
 * @param[in]  p_client:        pointer of the http client
 * @param[in]  buf:             to store received data with 'data_len_in_buf' data in it at first
 * @param[in]  max_len:         max size needed to read each time
 * @param[in]  data_len_in_buf: data remained in 'buf' at first
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv_chunk(duer_http_client_t *p_client,
        char *buf,
        size_t max_len,
        size_t data_len_in_buf)
{
    size_t need_to_read    = 0;
    size_t rev_len         = 0;
    size_t temp            = 0;
    size_t new_rev_len     = 0;
    size_t chunk_buff_len  = 0;
    size_t max_read_len    = 0;
    duer_http_result_t res = DUER_HTTP_OK;

    DUER_LOGV("enrty\n");

    DUER_LOGD("args:max_len=%d, data_len_in_buf=%d", max_len, data_len_in_buf);

    if (data_len_in_buf > max_len) {
        DUER_LOGE("Args err, data over buffer(%d > %d):", data_len_in_buf, max_len);
        HC_CHECK_ERR(p_client, DUER_HTTP_ERR_FAILED, "Args err!", DUER_HTTP_ERR_FAILED);
    }

    rev_len = data_len_in_buf;

    while (true) {
        res = duer_http_recv_chunked_size(p_client, buf, max_len, &rev_len);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_CHUNK_SIZE_RECEIVE_FAILED, NULL);
            DUER_LOGE("Receiving http chunked size error:res=%d", res);
            goto RET;
        }

        if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
            DUER_LOGI("Stopped getting media data from url by stop flag!\n");
            goto RET;
        }

        DUER_LOGD("*************HTTP chunk size is %d************\n", p_client->sz_chunk_size);

        //last chunk to return
        if (p_client->sz_chunk_size == 0) {
            DUER_LOGD("Receive the last HTTP chunk!!!");

            //to flush the last chunk buffer in the "RET"
            if (chunk_buff_len) {
                //add NULL terminate for print string
                p_client->p_chunk_buf[chunk_buff_len] = '\0';
            }

            goto RET;
        }

        //when chunk size is less than max_len of "buf"
        if (p_client->sz_chunk_size  + DUER_HTTP_CR_LF_LEN <= max_len) {
            if (rev_len < p_client->sz_chunk_size + DUER_HTTP_CR_LF_LEN) {
                if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
                    DUER_LOGI("Stopped getting media data from url by stop flag!\n");
                    goto RET;
                }

                res = duer_http_recv(
                          p_client,
                          buf + rev_len,
                          p_client->sz_chunk_size  + DUER_HTTP_CR_LF_LEN - rev_len,
                          &new_rev_len,
                          1);
                rev_len += new_rev_len;
                buf[rev_len] = '\0';
                DUER_LOGD("Receive %d chars in buf: \n[%s]", new_rev_len, buf);

                if (res != DUER_HTTP_OK) {
                    DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                    DUER_LOGE("Receiving  http chunked data error:res=%d", res);
                    goto RET;
                }
            }

            if (rev_len < p_client->sz_chunk_size + DUER_HTTP_CR_LF_LEN) {
                res = DUER_HTTP_ERR_PRTCL;
                DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                DUER_LOGE("No CR&LF after http data chunk: DUER_HTTP_ERR_PRTCL");
                goto RET;
            }

            //when received data is more than a chunk with "\r\n"
            if (rev_len >= p_client->sz_chunk_size + DUER_HTTP_CR_LF_LEN) {
                //for -1 reserve the last bytes to do nothing in p_chunk_buf
                if (chunk_buff_len + p_client->sz_chunk_size >= DUER_HTTP_CHUNK_SIZE - 1) {
                    temp = DUER_HTTP_CHUNK_SIZE - 1 - chunk_buff_len;
                    DUER_MEMMOVE(p_client->p_chunk_buf + chunk_buff_len, buf, temp);
                    //add NULL terminate for print string
                    p_client->p_chunk_buf[DUER_HTTP_CHUNK_SIZE - 1] = '\0';
                    chunk_buff_len += temp;
                    //call callback to output

                    if (p_client->data_hdlr_cb) {
                        p_client->data_hdlr_cb(p_client->p_data_hdlr_ctx, p_client->data_pos,
                                               p_client->p_chunk_buf, chunk_buff_len,
                                               p_client->p_http_content_type);
                        p_client->recv_size += chunk_buff_len;
                    }

                    p_client->data_pos = DUER_HTTP_DATA_MID;
                    DUER_MEMMOVE(p_client->p_chunk_buf,
                                 buf + temp,
                                 p_client->sz_chunk_size - temp);
                    chunk_buff_len = p_client->sz_chunk_size - temp;
                } else {
                    DUER_MEMMOVE(p_client->p_chunk_buf + chunk_buff_len, buf,
                                 p_client->sz_chunk_size);
                    chunk_buff_len += p_client->sz_chunk_size;
                }

                //remove "\r\n"
                if (buf[p_client->sz_chunk_size] != '\r'
                        || buf[p_client->sz_chunk_size + 1] != '\n') {
                    res = DUER_HTTP_ERR_PRTCL;
                    DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                    DUER_LOGE("No CR&LF after http data chunk when remove cr&lf");
                    goto RET;
                }

                rev_len -= (p_client->sz_chunk_size + DUER_HTTP_CR_LF_LEN);
                //Be sure to move NULL-terminating char as well
                DUER_MEMMOVE(buf, &buf[p_client->sz_chunk_size + DUER_HTTP_CR_LF_LEN], rev_len + 1);
            }
        } else { //when chunk size with "\r\n" is more than max_len of "buf"
            DUER_LOGD("Here to read large HTTP chunk (%d) bytes!", p_client->sz_chunk_size);

            if (p_client->sz_chunk_size <= rev_len) {
                res = DUER_HTTP_ERR_FAILED;
                DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                DUER_LOGE("Receiving  http large chunked data error: sz_chunk_size <= rev_len!");
                goto RET;
            }

            //read a large chunk data, (1)read all chunk data first
            need_to_read = p_client->sz_chunk_size - rev_len;

            while (need_to_read) {
                if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
                    DUER_LOGI("Stopped getting media data from url by stop flag!\n");
                    goto RET;
                }

                if (max_len - rev_len > need_to_read) {
                    max_read_len = need_to_read;
                } else {
                    max_read_len = max_len - rev_len;
                }

                res = duer_http_recv(p_client, buf + rev_len, max_read_len, &new_rev_len, 1);
                rev_len += new_rev_len;
                need_to_read -= new_rev_len;
                buf[rev_len] = '\0';
                DUER_LOGD("Receive %d chars in buf: \n", new_rev_len);

                if (res != DUER_HTTP_OK) {
                    DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                    DUER_LOGE("Receiving  http large chunk data error:res=%d", res);
                    goto RET;
                }

                //for sub 1 reserve '\0' the last bytes to do nothing in p_chunk_buf
                if (chunk_buff_len + rev_len >= DUER_HTTP_CHUNK_SIZE - 1) {
                    temp = DUER_HTTP_CHUNK_SIZE - 1 - chunk_buff_len;
                    DUER_MEMMOVE(p_client->p_chunk_buf + chunk_buff_len, buf, temp);
                    //add NULL terminate for print string
                    p_client->p_chunk_buf[DUER_HTTP_CHUNK_SIZE - 1] = '\0';
                    chunk_buff_len += temp;
                    //call callback to output

                    if (p_client->data_hdlr_cb) {
                        //wait_ms(1000); use for latency test
                        p_client->data_hdlr_cb(p_client->p_data_hdlr_ctx, p_client->data_pos,
                                               p_client->p_chunk_buf, chunk_buff_len,
                                               p_client->p_http_content_type);
                        p_client->recv_size += chunk_buff_len;
                    }

                    p_client->data_pos = DUER_HTTP_DATA_MID;
                    DUER_MEMMOVE(p_client->p_chunk_buf, buf + temp, rev_len - temp);
                    chunk_buff_len  = rev_len - temp;
                } else {
                    DUER_MEMMOVE(p_client->p_chunk_buf + chunk_buff_len, buf, rev_len);
                    chunk_buff_len += rev_len;
                }

                rev_len = 0;
            }

            DUER_LOGD("Read a large HTTP chunk (%d) bytes finished", p_client->sz_chunk_size);
            //(2)to receive "\r\n" and remove it
            DUER_LOGD("receive cr&lf and remove it");

            res = duer_http_recv(p_client, buf, DUER_HTTP_CR_LF_LEN, &new_rev_len, 1);
            buf[new_rev_len] = '\0';

            DUER_LOGD("Receive %d chars in buf: \n[0x%x, 0x%x]", new_rev_len, buf[0], buf[1]);

            if (res != DUER_HTTP_OK) {
                DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                DUER_LOGE("Receiving cr&lf http large chunked data error:res=%d", res);
                goto RET;
            }

            if ((new_rev_len != DUER_HTTP_CR_LF_LEN) || buf[0] != '\r' || buf[1] != '\n') {
                res = DUER_HTTP_ERR_PRTCL;
                DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
                DUER_LOGE("No CR&LF after http large data chunk:DUER_HTTP_ERR_PRTCL");
                goto RET;
            }

            // to reset rev_len when run here
            rev_len  = 0;
        }
    }//end of while(true)

RET:

    if ((res == DUER_HTTP_OK) && (chunk_buff_len > 0)) {
        if (p_client->data_hdlr_cb) {
            p_client->data_hdlr_cb(p_client->p_data_hdlr_ctx,
                                   p_client->data_pos,
                                   p_client->p_chunk_buf,
                                   chunk_buff_len,
                                   p_client->p_http_content_type);
            p_client->recv_size += chunk_buff_len;
            p_client->data_pos = DUER_HTTP_DATA_MID;
        }
    }

    DUER_LOGV("leave\n");
    return res;
}

/**
 * DESC:
 * receive http data
 *
 * PARAM:
 * @param[in]  p_client:        pointer of the http client
 * @param[in]  buf:             to store received data with 'data_len_in_buf' data in it at first
 * @param[in]  max_len:         max size needed to read each time
 * @param[in]  data_len_in_buf: data remained in 'buf' at first
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_recv_content_data(duer_http_client_t *p_client,
        char *buf,
        size_t max_len,
        size_t data_len_in_buf)
{
    size_t need_to_read = p_client->n_http_content_len - p_client->recv_size;
    size_t rev_len      = data_len_in_buf;
    size_t new_rev_len  = 0;
    duer_http_result_t res   = DUER_HTTP_OK;

    DUER_LOGD("enrty\n");

    if (data_len_in_buf > max_len || data_len_in_buf > need_to_read) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Args err, data over buffer(%d > %d or  %d):",
                  data_len_in_buf, max_len, need_to_read);

        HC_CHECK_ERR(p_client, DUER_HTTP_ERR_FAILED, "Args err!", DUER_HTTP_ERR_FAILED);
    }

    need_to_read -= rev_len;

    while (need_to_read || rev_len) {
        if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
            DUER_LOGI("Stopped getting media data from url by stop flag!\n");
            break;
        }

        DUER_LOGD("lefting %d bytes to read for all %d bytes(%f)", need_to_read,
                  p_client->n_http_content_len, 1.0 * need_to_read / p_client->n_http_content_len);

        res = duer_http_recv(
                  p_client, buf + rev_len,
                  (max_len - rev_len) > need_to_read ? need_to_read : (max_len - rev_len),
                  &new_rev_len,
                  1);

        rev_len += new_rev_len;
        need_to_read -= new_rev_len;
        buf[rev_len] = '\0';

        DUER_LOGV("Receive %d bytes total %d bytes in buf: \n[%s]", new_rev_len, rev_len, buf);

        if (rev_len) {
            if (!p_client->data_hdlr_cb) {
                DUER_LOGE("Data handler callback is NULL!");
                res = DUER_HTTP_ERR_FAILED;
                goto RET;
            }

            p_client->data_hdlr_cb(p_client->p_data_hdlr_ctx, p_client->data_pos, buf,
                                   rev_len, p_client->p_http_content_type);
            p_client->recv_size += rev_len;
            p_client->data_pos = DUER_HTTP_DATA_MID;
        }

        if (res != DUER_HTTP_OK) {
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Receive http data finished !errcode= %d when remaining %d bytes to receive",
                      res, need_to_read);
            break;
        }

        if (need_to_read > 0 && rev_len == 0) {
            DUER_DS_LOG_REPORT_HTTP_COMMON_ERROR();
            DUER_LOGE("Remaining %d bytes cannot receive,and give up\n", need_to_read);
            res = DUER_HTTP_ERR_FAILED;
            break;
        }

        rev_len = 0;
    }

RET:
    DUER_LOGD("leave\n");
    return res;
}

/**
 * DESC:
 * get http download progress
 *
 * PARAM:
 * @param[in]  p_client:   pointer of the http client
 * @param[out] total_size: to receive the total size to be download
 *                         If chunked Transfer-Encoding is used, we cannot know the total size
 *                         untile download finished, in this case the total size will be 0
 * @param[out] recv_size:  to receive the data size have been download
 *
 * @RETURN     none
 */
void duer_http_get_download_progress(duer_http_client_t *p_client,
                                     size_t *total_size,
                                     size_t *recv_size)
{
    if (!p_client || !total_size || !recv_size) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Args error!\n");
        return;
    }

    *total_size = p_client->n_http_content_len;
    *recv_size = p_client->recv_size;
}

/**
 * DESC:
 * parse the redirect location, create the new path if it is a relative url
 *
 * PARAM:
 * @param[in]  p_client:        pointer of the http client
 * @param[in]  p_path:          the path of the old url
 * @param[in]  path_len:        the max length of path
 * @param[out] is_relative_url: return whether it is a relative url
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_parse_redirect_location(duer_http_client_t *p_client,
        const char *p_path,
        const size_t path_len,
        bool *is_relative_url)
{
    char  *new_path = NULL;
    size_t len      = 0;

    DUER_LOGV("enrty\n");

    if ((!p_client) || (!is_relative_url) || (!p_path)) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Invalid param!\n");
        return DUER_HTTP_ERR_PARSE;
    }

    if (!p_client->p_location) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("Invalid location!\n");
        return DUER_HTTP_ERR_PARSE;
    }

    if (p_client->p_location[0] == 'h') {
        *is_relative_url = false;
        return DUER_HTTP_OK;
    }

    *is_relative_url = true;

    // the relative path's format may be "./path", "../path"
    if (p_client->p_location[0] != '/') {
        // one more byte for '/', one more byte for '\0'
        len = DUER_STRLEN(p_path) + DUER_STRLEN(p_client->p_location) + 2;
        new_path = DUER_MALLOC(len);

        if (!new_path) {
            DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
            DUER_LOGE("No enough memory!\n");
            return DUER_HTTP_ERR_PARSE;
        }

        DUER_SNPRINTF(new_path, len, "%s", p_path);

        // for example: the previous path is "p1/p2", the relative path is "../",
        // the new path should be "p1/p2/../", we should add a '/'
        if (p_path[DUER_STRLEN(p_path)] != '/') {
            DUER_STRNCAT(new_path, "/", 1);
        }

        DUER_STRNCAT(new_path, p_client->p_location, DUER_STRLEN(p_client->p_location));
        DUER_LOGI("new path: %s\n", new_path);
        DUER_FREE(p_client->p_location);
        p_client->p_location = NULL;
        p_client->p_location = new_path;
    }

    DUER_LOGV("leave\n");
    return DUER_HTTP_OK;
}

static duer_http_result_t duer_http_socket_connect(duer_http_client_t *p_client,
        const char *host,
        const unsigned short port)
{
    bool is_need_connect = false;
    duer_http_result_t ret = DUER_HTTP_OK;
    int rs = 0;
    int len = 0;

    if (!p_client->last_host) {
        is_need_connect = true;
    } else if (DUER_STRNCMP(host, p_client->last_host, DUER_HTTP_HOST_LEN_MAX)
               || (port != p_client->last_port)) {
        is_need_connect = true;
        http_client_close_socket_connect(p_client);
    } else {
        p_client->ops.set_timeout(p_client->ops.n_handle, 0);
        // p_client->ops.set_timeout(p_client->ops.n_handle, DUER_HTTP_TIME_OUT);
        rs = p_client->ops.recv(p_client->ops.n_handle, p_client->buf, 1);
        // connect had been closed by server or error happened
        if ((rs == 0) || ((rs < 0) && (rs != DUER_ERR_TRANS_WOULD_BLOCK))) {
            is_need_connect = true;
            http_client_close_socket_connect(p_client);
        } else {
            ret = DUER_HTTP_OK;
        }
    }

    if (!is_need_connect) {
        goto RET;
    }

    rs = p_client->ops.open(p_client->ops.n_handle);

    if (rs >= 0) {
        if(p_client->scheme == DUER_SCHEME_HTTPS) {
            char *ca_pem = duer_engine_get_rsa_cacrt();
            duer_trans_set_pk(p_client->ops.n_handle, ca_pem, DUER_STRLEN(ca_pem) + 1);
        } else {
            duer_trans_unset_pk(p_client->ops.n_handle);
        }
        p_client->ops.set_timeout(p_client->ops.n_handle, DUER_HTTP_TIME_OUT);
        rs = p_client->ops.connect(p_client->ops.n_handle, host, port);
        DUER_LOGD("n_handle %d, rs: %d", p_client->ops.n_handle, rs);
        if (rs < 0 ) {
            if (rs == DUER_INF_TRANS_IP_BY_HTTP_DNS) {
                DUER_LOGI("ip got by http dns");
                duer_ds_log_http(DUER_DS_LOG_HTTP_DNS_GET_IP, NULL);
            } else if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
                DUER_LOGD("http connect would block");
            } else {
                p_client->ops.close(p_client->ops.n_handle);
                DUER_LOGE("Failed to connect socket, rs: %d", rs);
                ret = DUER_HTTP_ERR_CONNECT;
                goto RET;
            }
        }
    } else {
        ret = DUER_HTTP_ERR_CONNECT;
        DUER_LOGE("Failed to open socket, rs: %d", rs);
        goto RET;
    }

    len = DUER_STRLEN(host);
    p_client->last_host = (char *)DUER_MALLOC(len + 1);

    if (!p_client->last_host) {
        ret = DUER_HTTP_ERR_FAILED;
    } else {
        DUER_SNPRINTF(p_client->last_host, len + 1, "%s", host);
        p_client->last_host[len] = '\0';
        p_client->last_port = port;
    }

RET:
    return ret;
}

/**
 * DESC:
 * connect to the given url
 *
 * PARAM:
 * @param[in]  p_client:           pointer of the http client
 * @param[in]  url:                url to be connected
 * @param[in]  method:             method to comunicate with server
 * @param[out] remain_size_in_buf: how many data received when connect
 *
 * RETURN      DUER_HTTP_OK if success, other duer_http_result_t type code if failed
 */
static duer_http_result_t duer_http_connect(duer_http_client_t *p_client,
        const char *url,
        duer_http_method_t method,
        size_t *remain_size_in_buf)
{
    char   scheme[DUER_HTTP_SCHEME_LEN_MAX];
    char  *p_path          = NULL;
    char  *host            = NULL;
    bool   is_relative_url = false;
    int    path_len_max    = 0;
    int    ret             = 0;
    int    redirect_count  = 0;
    size_t rev_len         = 0;
    unsigned short port    = 0;
    duer_http_result_t  res     = DUER_HTTP_OK;

    DUER_LOGV("enrty\n");

    host = (char *)DUER_MALLOC(DUER_HTTP_HOST_LEN_MAX + 1); //including NULL-terminating char

    if (host == NULL) {
        DUER_LOGE("No memory!");
        return DUER_HTTP_ERR_PARSE;
    }

    while (redirect_count <= DUER_HTTP_REDIRECTION_MAX) {
        if (!url) {
            duer_ds_log_http_report_with_url(DUER_DS_LOG_HTTP_URL_PARSE_FAILED, NULL);
            DUER_LOGE("url is NULL!");
            res = DUER_HTTP_ERR_FAILED;
            goto RET;
        }

        if (DUER_HTTP_CHUNK_SIZE <= DUER_HTTP_RSP_CHUNK_SIZE) {
            DUER_LOGE("http client chunk size config error!!!\n");
            res = DUER_HTTP_ERR_FAILED;
            goto RET;
        }

        path_len_max = DUER_STRLEN(url) + 1;
        p_path = (char *)DUER_MALLOC(path_len_max);

        if (!p_path) {
            DUER_DS_LOG_REPORT_HTTP_MEMORY_ERROR();
            DUER_LOGE("Mallco memory(size:%d) Failed!\n", path_len_max);
            res = DUER_HTTP_ERR_FAILED;
            goto RET;
        }

        //to parse url
        res = duer_http_parse_url(url, scheme, sizeof(scheme), host, &port, p_path,
                                  path_len_max, is_relative_url);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http_report_with_url(DUER_DS_LOG_HTTP_URL_PARSE_FAILED, url);
            DUER_LOGE("parseURL returned %d", res);
            res = DUER_HTTP_ERR_FAILED;
            goto RET;
        }

        p_client->scheme = DUER_SCHEME_HTTP;
        if (!strncmp(scheme, "HTTPS", strlen("HTTPS"))
                || !strncmp(scheme, "https", strlen("https"))) {
            p_client->scheme = DUER_SCHEME_HTTPS;
        }

        //to set default port
        if (port == 0) { //to do handle HTTPS->443
            if (p_client->scheme == DUER_SCHEME_HTTPS) {
                port = DUER_DEFAULT_HTTPS_PORT;
            } else {
                port = DUER_DEFAULT_HTTP_PORT;
            }
        }

        DUER_LOGD("Scheme: %s", scheme);
        DUER_LOGD("Host: %s", host);
        DUER_LOGD("Port: %d", port);
        DUER_LOGD("Path: %s", p_path);

        //to connect server
        DUER_LOGD("Connecting socket to server");

        ret = duer_http_socket_connect(p_client, host, port);

        if (ret != DUER_HTTP_OK) {
            duer_ds_log_http_report_with_url(DUER_DS_LOG_HTTP_SOCKET_CONN_FAILED, url);
            DUER_LOGE("Could not connect(%d): %s:%d\n", ret, host, port);
            res = DUER_HTTP_ERR_CONNECT;
            goto RET;
        }

        DUER_LOGD("Connecting socket to server success:%d", ret);

        //to send http request
        DUER_LOGD("Sending request");
        res = duer_http_send_request(p_client, method, p_path, host, port);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_REQUEST_SEND_FAILED, NULL);
            goto RET;
        }

        //to send http headers
        res = duer_http_send_headers(p_client, p_client->buf, DUER_HTTP_CHUNK_SIZE);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_HEADER_SEND_FAILED, NULL);
            goto RET;
        }

        //to receive http response
        DUER_LOGD("Receiving  http response");
        res = duer_http_recv_rsp_and_parse(p_client,
                                           p_client->buf,
                                           DUER_HTTP_RSP_CHUNK_SIZE + 1,
                                           &rev_len);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_RESPONSE_RECEIVE_FAILED, NULL);
            goto RET;
        }

        //to receive http headers and pasrse it
        DUER_LOGD("Receiving  http headers");
        res = duer_http_recv_headers_and_parse(p_client,
                                               p_client->buf,
                                               DUER_HTTP_CHUNK_SIZE,
                                               &rev_len);

        if (res == DUER_HTTP_REDIRECTTION) {
            DUER_LOGI("DUER_HTTP_REDIRECTTION");
            http_client_close_socket_connect(p_client);
            res = duer_http_parse_redirect_location(p_client, p_path,
                                                    path_len_max, &is_relative_url);

            if (res != DUER_HTTP_OK) {
                duer_ds_log_http_redirect_fail(p_path);
                goto RET;
            }

            DUER_FREE(p_path);
            p_path = NULL;
            url = p_client->p_location;
            redirect_count++;
            DUER_LOGI("Taking %d redirections to [%s]", redirect_count, url);
            continue;
        } else if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_HEADER_RECEIVE_FAILED, NULL);
            goto RET;
        } else {
            if (p_client->get_url_cb) {
                duer_ds_log_http_report_with_url(DUER_DS_LOG_HTTP_DOWNLOAD_URL, url);
                p_client->get_url_cb(url);
            }

            break;
        }
    }

    if (redirect_count > DUER_HTTP_REDIRECTION_MAX) {
        DUER_LOGE("Too many redirections!");
    }

RET:

    if (p_path) {
        DUER_FREE(p_path);
    }

    if (host) {
        DUER_FREE(host);
    }

    *remain_size_in_buf = rev_len;

    DUER_LOGV("leave\n");
    return res;
}

duer_http_result_t duer_http_get_data(duer_http_client_t *p_client, size_t remain_size_in_buf)
{
    duer_http_result_t res     = DUER_HTTP_OK;

    DUER_LOGV("enrty\n");

    //to receive http chunk data
    if (p_client->n_is_chunked) {
        DUER_LOGD("Receiving  http chunk data\n");
        res = duer_http_recv_chunk(p_client, p_client->buf, DUER_HTTP_CHUNK_SIZE - 1,
                                   remain_size_in_buf);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_CHUNK_RECEIVE_FAILED, NULL);
        }
    } else {
        //to receive http data content
        DUER_LOGD("Receiving  http data content\n");
        res = duer_http_recv_content_data(p_client, p_client->buf, DUER_HTTP_CHUNK_SIZE - 1,
                                          remain_size_in_buf);

        if (res != DUER_HTTP_OK) {
            duer_ds_log_http(DUER_DS_LOG_HTTP_CONTENT_RECEIVE_FAILED, NULL);
        }
    }

    if (res == DUER_HTTP_OK) {
        DUER_LOGI("Completed HTTP transaction");
    }

    DUER_LOGV("leave\n");
    return res;
}

static void duer_http_conn_keep_timer_cb(void *argument)
{
    duer_ds_log_http_persisten_conn_timeout(((duer_http_client_t *)argument)->last_host);
    http_client_close_socket_connect(argument);
}

static void duer_http_ds_log_statistic(duer_http_client_t *p_client,
                                       int start_time,
                                       const size_t pos,
                                       int ret,
                                       const char *url)
{
    int time_spend = 0;
    duer_ds_log_http_statistic_t statistic_info;
    duer_ds_log_http_code_t code = DUER_DS_LOG_HTTP_DOWNLOAD_FINISHED;

    DUER_MEMSET(&statistic_info, 0, sizeof(duer_ds_log_http_statistic_t));
    statistic_info.url = url;
    statistic_info.ret_code = ret;
    statistic_info.upload_size = p_client->upload_size;
    statistic_info.content_len = p_client->n_http_content_len;
    statistic_info.is_chunked = p_client->n_is_chunked;
    statistic_info.download_size = p_client->recv_size - pos;
    statistic_info.resume_count = p_client->resume_retry_count;
    time_spend = duer_timestamp() - start_time;

    if (time_spend > 0) {
        statistic_info.download_speed = ((float)statistic_info.download_size) / time_spend * 1000;
    }

    if (ret != DUER_HTTP_OK) {
        code = DUER_DS_LOG_HTTP_DOWNLOAD_FAILED;
    } else if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
        code = DUER_DS_LOG_HTTP_DOWNLOAD_STOPPED;
    } else {
        code = DUER_DS_LOG_HTTP_DOWNLOAD_FINISHED;
    }

    duer_ds_log_http_download_exit(code, &statistic_info);
}

duer_http_result_t duer_http_get(duer_http_client_t *p_client,
                                 const char *url,
                                 const size_t pos,
                                 const int connect_keep_time)
{
    size_t         rev_len = 0;
    duer_http_result_t  ret     = DUER_HTTP_OK;
    int started_time = 0;

    DUER_LOGV("enrty\n");

    if (!p_client || !url) {
        DUER_DS_LOG_REPORT_HTTP_PARAM_ERROR();
        DUER_LOGE("PARAM error");
        return DUER_HTTP_ERR_FAILED;
    }

    duer_ds_log_http_report_with_url(DUER_DS_LOG_HTTP_DOWNLOAD_STARTED, url);
    DUER_LOGD("http get url: %s\n", url);

    started_time = duer_timestamp();

    if (p_client->connect_keep_timer) {
        duer_timer_stop(p_client->connect_keep_timer);
        duer_timer_release(p_client->connect_keep_timer);
        p_client->connect_keep_timer = NULL;
    }

    p_client->data_pos = DUER_HTTP_DATA_FIRST;
    p_client->resume_retry_count = 0;
    p_client->recv_size = pos;
    p_client->upload_size = 0;
    duer_http_set_content_len(p_client, 0);

    p_client->buf = DUER_MALLOC(DUER_HTTP_CHUNK_SIZE);

    if (!p_client->buf) {
        ret = DUER_HTTP_ERR_FAILED;
        goto RET;
    }

    while (p_client->resume_retry_count <= DUER_HTTP_RESUME_COUNT_MAX) {
        if (p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) {
            DUER_LOGI("Stopped getting media data from url by stop flag!\n");
            break;
        }

        // create http connect
        ret = duer_http_connect(p_client, url, DUER_HTTP_GET, &rev_len);

        if ((ret == DUER_HTTP_ERR_CONNECT) || (ret == DUER_HTTP_ERR_TIMEOUT)) {
            p_client->resume_retry_count++;
            DUER_LOGI("Try to resume from break-point %d time\n", p_client->resume_retry_count);
            continue;
        }

        if (ret != DUER_HTTP_OK) {
            break;
        }

        // get http data
        ret = duer_http_get_data(p_client, rev_len);

        if ((ret == DUER_HTTP_ERR_CONNECT)
                || (ret == DUER_HTTP_ERR_TIMEOUT)
                || (ret == DUER_HTTP_CLOSED)) {
            DUER_LOGW("duer http get data ret:%d", ret);
            p_client->resume_retry_count++;
            if (p_client->resume_retry_count <= DUER_HTTP_RESUME_COUNT_MAX) {
                DUER_LOGI("Resume from break-point:retry %d time\n", p_client->resume_retry_count);
            } else {
                DUER_LOGE("Resume from break-point too many times\n");
            }

            continue;
        }

        break;
    }

    DUER_LOGI("HTTP: %d bytes received\n", p_client->recv_size - pos);

    if (p_client->recv_size == pos) {
        duer_ds_log_http(DUER_DS_LOG_HTTP_ZERO_BYTE_DOWNLOAD, NULL);
    }

RET:

    if ((p_client->data_pos != DUER_HTTP_DATA_FIRST) && (p_client->data_hdlr_cb)) {
        p_client->data_hdlr_cb(
            p_client->p_data_hdlr_ctx,
            DUER_HTTP_DATA_LAST,
            NULL,
            0,
            p_client->p_http_content_type);
    }

    duer_http_ds_log_statistic(p_client, started_time, pos, ret, url);

    if ((p_client->check_stop_notify_cb && p_client->check_stop_notify_cb()) ||
            (connect_keep_time == 0) || (ret != DUER_HTTP_OK)) {
        http_client_close_socket_connect(p_client);
    } else {
        p_client->connect_keep_timer = duer_timer_acquire(duer_http_conn_keep_timer_cb,
                                       p_client,
                                       DUER_TIMER_ONCE);

        if (p_client->connect_keep_timer) {
            duer_timer_start(p_client->connect_keep_timer, connect_keep_time);
        } else {
            http_client_close_socket_connect(p_client);
            DUER_LOGE("Failed to create timer to close persistent connect");
            ret = DUER_HTTP_ERR_FAILED;
        }
    }

    if (p_client->p_location) {
        DUER_FREE(p_client->p_location);
        p_client->p_location = NULL;
    }

    if (p_client->p_chunk_buf) {
        DUER_FREE(p_client->p_chunk_buf);
        p_client->p_chunk_buf = NULL;
    }

    if (p_client->buf) {
        DUER_FREE(p_client->buf);
        p_client->buf = NULL;
    }

    DUER_LOGV("leave\n");
    return ret;
}

