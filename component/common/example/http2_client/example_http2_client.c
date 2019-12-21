#include <platform_opts.h>

#if CONFIG_EXAMPLE_HTTP2_CLIENT

#include <platform/platform_stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wifi_conf.h"
#include "lwip/netdb.h"

#include <nghttp2/nghttp2.h>

#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define MAKE_NV_CS(NAME, VALUE)                                                \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, strlen(VALUE),        \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define REQ_HOST "nghttp2.org"
#define REQ_PORT 80
#define REQ_PATH "/"
#define REQ_HOSTPORT "nghttp2.org:80"

// https://tools.keycdn.com/http2-test

static nghttp2_session *hp2_session = NULL;

struct Connection {
    nghttp2_session *session;

    int sockfd;
};

struct Request {
    char *host;
    /* In this program, path contains query component as well. */
    char *path;
    /* This is the concatenation of host and port with ":" in between. */
    char *hostport;
    /* Stream ID for this request. */
    int32_t stream_id;
    uint16_t port;
};

static ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
{
    struct Connection *conn;
    int ret;
    int so_error;
    socklen_t len = sizeof(so_error);

    conn = (struct Connection *)user_data;

    ret = write(conn->sockfd, data, length);

    if (ret < 0) {
        getsockopt(conn->sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error == EAGAIN || so_error == 0) {
            ret = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            ret = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    } else if (ret == 0) {
        ret = NGHTTP2_ERR_WOULDBLOCK;
    } else {
    }

    return ret;
}

static ssize_t recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
{
    struct Connection *conn;
    int ret;
    int so_error;
    socklen_t len = sizeof(so_error);

    conn = (struct Connection *)user_data;

    ret = read(conn->sockfd, buf, length);
    if (ret < 0) {
        getsockopt(conn->sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error == EAGAIN || so_error == 0) {
            ret = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            ret = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    } else if (ret == 0) {
        ret = NGHTTP2_ERR_EOF;
    } else {
    }
}

static int on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
{
    size_t i;
    (void)user_data;

    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (nghttp2_session_get_stream_user_data(session, frame->hd.stream_id)) {
            const nghttp2_nv *nva = frame->headers.nva;
            printf("[INFO] C ----------------------------> S (HEADERS)\n");
            for (i = 0; i < frame->headers.nvlen; ++i) {
                printf("\t%.*s: %.*s\n", nva[i].namelen, nva[i].name, nva[i].valuelen, nva[i].value);
            }
        }
        break;
    case NGHTTP2_RST_STREAM:
        printf("[INFO] C ----------------------------> S (RST_STREAM)\n");
        break;
    case NGHTTP2_GOAWAY:
        printf("[INFO] C ----------------------------> S (GOAWAY)\n");
        break;
    }
    return 0;
}

static int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
{
    size_t i;
    (void)user_data;

    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
            const nghttp2_nv *nva = frame->headers.nva;
            struct Request *req;
            req = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (req) {
                printf("[INFO] C <---------------------------- S (HEADERS)\n");
                for (i = 0; i < frame->headers.nvlen; ++i) {
                    printf("\t%.*s: %.*s\n", nva[i].namelen, nva[i].name, nva[i].valuelen, nva[i].value);
                }
            }
        }
        break;
    case NGHTTP2_RST_STREAM:
        printf("[INFO] C <---------------------------- S (RST_STREAM)\n");
        break;
    case NGHTTP2_GOAWAY:
        printf("[INFO] C <---------------------------- S (GOAWAY)\n");
        break;
    }
    return 0;
}

static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
{
    struct Request *req;
    (void)error_code;
    (void)user_data;

    req = nghttp2_session_get_stream_user_data(session, stream_id);
    if (req) {
        int ret;
        if ( (ret = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR)) != 0 ) {
            printf("[ERROR] nghttp2_session_terminate_session:%d\n", ret);
        }
    }
    return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
{
    struct Request *req;
    (void)flags;
    (void)user_data;

    req = nghttp2_session_get_stream_user_data(session, stream_id);
    if (req) {
        printf("[INFO] C <---------------------------- S (DATA chunk) %d\n", len);
        printf("%.*s\n", len, data);
    }
    return 0;
}

static void setup_nghttp2_callbacks(nghttp2_session_callbacks *callbacks) {
    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_recv_callback(callbacks, recv_callback);
    nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, on_frame_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
}

static int connect_to(const char *host, uint16_t port) {
    int ret = 0;
    int sockfd = -1;
    struct hostent *server;
    struct sockaddr_in serv_addr;

    do {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            printf("[ERROR] Create socket failed\n");
            ret = -1;
            break;
        }

        server = gethostbyname(host);
        if(server == NULL) {
            printf("[ERROR] Get host ip failed\n");
            ret = -1;
            break;
        }

        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

        if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
            printf("[ERROR] connect failed\n");
            ret = -1;
            break;
        }

        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &(int){ 1 }, sizeof(int));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &(int){ 1000 }, sizeof(int));
    } while (0);

    if (ret == 0) {
        return sockfd;
    } else {
        return -1;
    }
}

/*
 * Submits the request |req| to the connection |connection|.  This
 * function does not send packets; just append the request to the
 * internal queue in |connection->session|.
 */
static void submit_request(struct Connection *connection, struct Request *req) {
    int32_t stream_id;
    /* Make sure that the last item is NULL */
    const nghttp2_nv nva[] = {MAKE_NV(":method", "GET"),
                            MAKE_NV_CS(":path", req->path),
                            MAKE_NV(":scheme", "http"),
                            MAKE_NV_CS(":authority", req->hostport),
                            MAKE_NV("accept", "*/*"),
                            MAKE_NV("user-agent", "nghttp2/" NGHTTP2_VERSION)};

    stream_id = nghttp2_submit_request(connection->session, NULL, nva,
                                     sizeof(nva) / sizeof(nva[0]), NULL, req);

    if (stream_id < 0) {
        printf("[ERROR] nghttp2_submit_request:%d\n", stream_id);
    }

    req->stream_id = stream_id;
    printf("[INFO] Stream ID = %d\n", stream_id);
}

static struct Request * request_init(char *host, int16_t port, char *path, char *hostport) {
    struct Request *req = NULL;

    req = (struct Request *) malloc ( sizeof(struct Request) );
    if (req == NULL) {
        return NULL;
    }

    req->host = host;
    req->port = port;
    req->path = path;
    req->hostport = hostport;
    req->stream_id = -1;

    return req;
}

static void example_http2_client_thread(void *pvParameters)
{
    int ret;
    struct Connection *conn;
    struct Request *req;
    nghttp2_session_callbacks *callbacks;

    printf("waiting for wifi connection...\n");
    vTaskDelay(1000);
    while ( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS ) {
        vTaskDelay(1000);
    }
    printf("wifi connected\n");

    if ( (req = request_init(REQ_HOST, REQ_PORT, REQ_PATH, REQ_HOSTPORT)) == NULL ) {
        printf("[ERROR] Create request failed\n");
    }

    conn = (struct Connection *) malloc ( sizeof(struct Connection) );
    if (conn == NULL) {
        printf("[ERROR] Create connection failed\n");
    }

    printf("connting to server...\n");
    conn->sockfd = connect_to(req->host, req->port);
    if ( conn->sockfd == -1 ) {
        printf("[ERROR] connect failed\n");
    }
    printf("connected\n");

    if ( (ret = nghttp2_session_callbacks_new(&callbacks)) != 0 ) {
        printf("[ERROR] nghttp2_session_callbacks_new: %d\n", ret);
    }

    setup_nghttp2_callbacks(callbacks);

    ret = nghttp2_session_client_new(&(conn->session), callbacks, conn);
    nghttp2_session_callbacks_del(callbacks);
    if (ret != 0) {
        printf("[ERROR] nghttp2_session_client_new: %d\n", ret);
    }

    if ( (ret = nghttp2_submit_settings(conn->session, NGHTTP2_FLAG_NONE, NULL, 0)) != 0 ) {
        printf("[ERROR] nghttp2_submit_settings: %d\n", ret);
    }

    submit_request(conn, req);

    while ( nghttp2_session_want_read(conn->session) || nghttp2_session_want_write(conn->session) ) {
        if (nghttp2_session_want_write(conn->session)) {
            ret = nghttp2_session_send(conn->session);
            if (ret < 0) {
                printf("[ERROR]: send err:%d\n", ret);
                break;
            }
        }

        if (nghttp2_session_want_read(conn->session)) {
            ret = nghttp2_session_recv(conn->session);
            if (ret < 0) {
                printf("[ERROR] recv err:%d\n", ret);
                if (ret == NGHTTP2_ERR_CALLBACK_FAILURE) {
                    ret = -1;
                    break;
                }
            }
        }
    }

    nghttp2_session_del(conn->session);
    shutdown(conn->sockfd, SHUT_RDWR);
    close(conn->sockfd);

    free(req);
    free(conn);

    printf("example end\n");

    vTaskDelete(NULL);
}

void example_http2_client(void)
{
    if(xTaskCreate(example_http2_client_thread, ((const char*)"example_http2_client_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
        printf("\n\r%s xTaskCreate(example_http2_client_thread) failed", __FUNCTION__);
}

#endif