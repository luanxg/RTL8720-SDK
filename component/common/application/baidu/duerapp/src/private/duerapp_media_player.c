#include <platform_opts.h>
#include <FreeRTOS.h>
#include <task.h>
#include <platform/platform_stdlib.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <time.h>

#include "osdep_service.h"
#include "semphr.h"

#include "lightduer_memory.h"
#include "lightduer_dcs.h"
#include "lightduer_ds_log_audio.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"

#include "duerapp.h"
#include "duerapp_config.h"
#include "duerapp_audio.h"
#include "duerapp_media.h"
#include "mp3_codec.h"
#include "duerapp_peripheral_audio.h"

//#define DUER_SUPPORT_WAV

#define MAX_RETYR_CNT  	50
#define MAX_CONN_RETYR  3
#define WHILE_DELAY     2
#define MAX_OFFSET_LEN  10

#define HOST_NAME_LEN   256
#define URI_MAX_LEN     512
#define REQ_MAX_LEN		512
#define RCV_BUF_SIZE 	3024
#define RCV_SND_TIMEOUT (5*1000)

#define HTTP_OK         200
#define HTTP_PARTICAL   206
#define HTTP_REDIRECT   302
#define HTTP_NOT_FOUND  404

#define AUDIO_PKT_BEGIN (1)
#define AUDIO_PKT_DATA  (2)
#define AUDIO_PKT_END   (3)

#define MP3_MAX_FRAME_SIZE (1600)
#define MP3_DATA_CACHE_SIZE ( RCV_BUF_SIZE + 2 * MP3_MAX_FRAME_SIZE )

#define WAV_MAX_FRAME_SIZE (4608)
#define WAV_DATA_CACHE_SIZE ( RCV_BUF_SIZE + WAV_MAX_FRAME_SIZE )

//static uint32_t mp3_data_cache_len = 0;
//SDRAM_BSS_SECTION static uint8_t mp3_data_cache[MP3_DATA_CACHE_SIZE] = {0};

static duer_mutex_t s_qlen_lock;
static uint32_t s_queue_len = 0;

typedef struct _audio_pkt_t {
	uint8_t type;
	uint8_t *data;
	uint32_t data_len;
} audio_pkt_t;

#define AUDIO_PKT_QUEUE_LENGTH (10) //(100)
static xQueueHandle audio_pkt_queue;

extern T_APP_DUER *pDuerAppInfo;
extern mp3_decoder_t mp3;
extern signed short pcm_buf[];

static char *strncasestr(char *str, char *sub)
{
	if (!str || !sub)
		return NULL;

	int len = strlen(sub);

	if (len == 0)
		return NULL;

	while (*str)
	{
		if (strncasecmp(str, sub, len) == 0)
			return str;
		++str;
	}
	return NULL;
}

static int http_get_host_info(char *downloadurl, http_t *info) {

	char hostlen;
	char *p_start = NULL;
	char *p_end = NULL;
	char *p_port = NULL;

	HTTP_LOGD("Downloadurl: %s", downloadurl);
	p_start = strstr(downloadurl, "http://");
	if (p_start == NULL)
		goto Error;

	p_start = p_start + strlen("http://");
	p_end = strstr( p_start, "/");
	if ( p_end == NULL )
		goto Error;
	info->host = (char *)DUER_MALLOC(p_end - p_start + 1);
	if (NULL == info->host)
	{
		HTTP_LOGE("Host name malloc fail!");
		goto Error;
	}
	memcpy(info->host, p_start, p_end - p_start);
	(info->host)[p_end - p_start] = '\0';
	hostlen = p_end - p_start;

	info->dest_port = 80;
	p_port = strstr( (info->host), ":");
	if (p_port) {
		*p_port = '\0';
		info->dest_port = atoi(p_port + 1);
		if (info->dest_port <= 0 || info->dest_port >= 65535)
		{
			DUER_LOGE("url port invaild\n");
			goto Error;
		}
	}

	p_start = p_end;
	if (p_start == NULL)
		goto Error;
	p_end = p_start + strlen(downloadurl) - hostlen;
	if (p_end == NULL)
		goto Error;

	info->url = (char *)DUER_MALLOC(p_end - p_start + 1);
	if (NULL == info->url) {
		HTTP_LOGE("URL malloc fail!");
		goto Error;
	}
	memcpy(info->url, p_start, p_end - p_start);
	(info->url)[p_end - p_start] = '\0';

	HTTP_LOGD("Host: %s  URL:%s  Dest Port: %d", info->host, info->url, info->dest_port);
	return 0;

Error:
	SAFE_FREE(info->host);
	SAFE_FREE(info->url);
	return -1;
}

static int http_connect_server(http_t* info) {

	int ret = 0;
	int retry_cnt = 0;
	struct sockaddr_in server_addr;
	struct hostent *server_host;
	int recv_timeout_ms = RCV_SND_TIMEOUT;

	while (retry_cnt < MAX_CONN_RETYR) {
		if ((info->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			HTTP_LOGE("http download thread: create socket failed!");
			return -1;
		}
		setsockopt(info->server_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_ms, sizeof(recv_timeout_ms));

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(info->dest_port);

		// Support SERVER_HOST in IP or domain name
		server_host = gethostbyname(info->host);
		if (server_host) {
			memcpy((void *) &server_addr.sin_addr, (void *) server_host->h_addr, server_host->h_length);
		} else {
			HTTP_LOGE("ERROR: http download thread: get host by name failed!");
			return -1;
		}
		ret = connect(info->server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
		if (ret != 0) {
			HTTP_LOGE("WARNNING: Connect %s failed. Try again, cnt %d!", info->host, retry_cnt);
			if (info->server_fd >= 0)
				close(info->server_fd);
			retry_cnt++;
			continue;
		} else {
			HTTP_LOGD("Server Connected");
			return 0;
		}
		vTaskDelay(WHILE_DELAY);
	}

	HTTP_LOGE("ERROR: Connect %s failed.", info->host);
	return -1;
}

static int http_send_request(http_t* info) {
	int ret = 0;
	int req_length = 0;
	char *req_buf;

	req_length = 100 + strlen(info->url) + strlen(info->host) + MAX_OFFSET_LEN;
	req_buf = (char *)DUER_MALLOC_SDRAM(req_length);
	if (NULL == req_buf) {
		HTTP_LOGE("http download thread: Allocate req_buf failed!");
		return -1;
	}
	memset(req_buf, 0x0, req_length);
	snprintf(req_buf, req_length - 1, "GET %s HTTP/1.1\r\n"
	         //"Accept: */*\r\n"
	         //"User-Agent: Mozilla/5.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n"
	         "Host: %s\r\n"
	         "Range: bytes=%d-\r\n"
	         "Connection: Keep-Alive\r\n\r\n", info->url, info->host, info->offset);

	HTTP_LOGD("request: %s\r\n", req_buf);

	ret = write(info->server_fd, req_buf, strlen(req_buf));
	SAFE_FREE(req_buf);
	return ret < 0 ? -1 : 0;
}

static int http_connect_server_and_send_request(http_t* info) {
	//connect server and send request
	if (http_connect_server(info)) {
		HTTP_LOGE("http download thread: Create socket fail!");
		return -1;
	}

	if (http_send_request(info)) {
		HTTP_LOGE("ERROR: post data failed!");
		return -1;
	}

	return 0;
}

extern duer_media_info_t MP3Info;
static int http_parse_header(http_t* info, int buffer_len) {

	int header_len = 0;
	int body_len = 0;
	int new_url_len = 0;

	char *p = NULL;
	char *buffer = NULL;
	char *next_attr_begin = NULL;
	char *header_end = NULL;
	char *current_attr_end = NULL;
	char *http_rsp_code = NULL;
	char *body_begin = NULL;

	buffer = info->buffer;
	header_end = strstr(buffer, "\r\n\r\n");
	if (!header_end) {
		HTTP_LOGE("Error: Incomplete header!");
		return -1;
	}

	body_begin = header_end + strlen("\r\n\r\n");
	body_len = buffer_len - (body_begin - buffer);

	info->cur_ptr = body_begin;
	info->bytesleft = body_len;

	HTTP_LOGD("buffer_len %d,body_len: %d\n",
	          buffer_len, body_len);

	*(body_begin - 2) = 0;
	HTTP_LOGD( "Http Header: %s\n", buffer);

	next_attr_begin = buffer;

	p = strchr(next_attr_begin, ' ');
	if (!p || !strncasestr(next_attr_begin, "HTTP"))
	{
		HTTP_LOGE( "bad http head\n");
		return -1;
	}
	info->status_code = atoi(p + 1);
	HTTP_LOGI("http status code: %d\n", info->status_code);

	current_attr_end = strstr(next_attr_begin, "\r\n");
	next_attr_begin = current_attr_end + 2;

	if (p = strncasestr(next_attr_begin, "Content-length"))
	{
		p = strchr(p, ':');
		p += 2;
		info->content_len = atoi(p);
		HTTP_LOGI("Content-length: %d\n", info->content_len);
		MP3Info.content_len = info->content_len;
	}
	if (p = strncasestr(next_attr_begin, "Content-Range"))
	{
		p = strchr(p, '/');
		p += 1;
		info->range_len = atoi(p);
		HTTP_LOGI("Content-Range len: %d\n", info->range_len);
		MP3Info.range_len = info->range_len;
	}
	if (p = strncasestr(next_attr_begin, "Transfer-Encoding"))
	{
		if (strncasestr(buffer, "chunked"))
		{
			HTTP_LOGI("Transfer-Encoding: chunked\n");
			info->chunked_flag = 1;
		}
	}
	if (p = strncasestr(next_attr_begin, "Location"))
	{
		p += strlen("Location: ");
		new_url_len = strstr(p, "\r\n") - p;
		info->redirect_url = (char *)DUER_MALLOC(new_url_len + 1);
		if (info->redirect_url == NULL) {
			HTTP_LOGE("HTTP Redirect Malloc failed, goto EXIT\r\n");
			return -1;
		}
		strncpy(info->redirect_url, p, new_url_len);
		HTTP_LOGI("Redircet URL %s\r\n", info->redirect_url);
	}

	if (!info->chunked_flag) {
		info->offset += info->bytesleft;
		MP3Info.downloadBytes += info->bytesleft;
		MP3Info.cur_download_offset = info->offset;
	}

	return 0;
}

static void http_remove_header(http_t *info, int *buffer_len) {

	int body_len = 0;
	char *buffer = NULL;
	char *header_end = NULL;
	char *body_begin = NULL;

	buffer = info->buffer + info->bytesleft;
	header_end = strstr(buffer, "\r\n\r\n");

	if (header_end) {
		body_begin = header_end + strlen("\r\n\r\n");
		body_len = *buffer_len - (body_begin - buffer);
		DUER_LOGI("header len %d / recv bytes %d", body_begin - buffer, *buffer_len);
		memmove(buffer, body_begin, body_len);
		memset(buffer + body_len, 0x00, *buffer_len - body_len);
		*buffer_len = body_len;
	}
}

static void clean_up(http_t* info) {

	if (!info)
		return;

	if (info->server_fd >= 0)
		close(info->server_fd);

	SAFE_FREE(info->host);
	SAFE_FREE(info->url);
	SAFE_FREE(info->redirect_url);
	SAFE_FREE(info->buffer);
	SAFE_FREE(info);
}

static int http_play_before_finish(int stopReason) {


	HTTP_LOGD("enter play before finish..");

	audio_player_stop(stopReason);
	duer_dcs_audio_on_stuttered(false);

	if (DUER_PLAY_STOP_BY_FLAG == stopReason) {
		duer_media_player_rehabilitation();
	} else if (DUER_PLAY_OK == stopReason) {
		if (AUDIO == MP3Info.ddt) {
			HTTP_LOGI("report duer_dcs_audio_on_finished..");
			duer_dcs_audio_on_finished();
		} else if (SPEAK == MP3Info.ddt) {
			duer_dcs_speech_on_finished();
		} else if (ALERT == MP3Info.ddt) {
			duer_dcs_alert_on_finish(MP3Info.token);
		}
	} else {
		DUER_LOGE("media stop with error");
	}
	return DUER_OK;
}


static void audio_queue_reset(){
	duer_mutex_lock(s_qlen_lock);
	s_queue_len = 0;
	duer_mutex_unlock(s_qlen_lock);
}

static void audio_queue_increase(){
	duer_mutex_lock(s_qlen_lock);
	s_queue_len ++;
	duer_mutex_unlock(s_qlen_lock);
}

static void audio_queue_decrease(){
	duer_mutex_lock(s_qlen_lock);
	s_queue_len --;
	duer_mutex_unlock(s_qlen_lock);
}

static int http_process_unchucked(http_t *info) {

	int err = DUER_PLAY_OK;
	int condition = 0;
	int retry_cnt = 0;
	int retry_flag = 0;
	int recv_bytes = 0;
	int except_size = RCV_BUF_SIZE;
	audio_pkt_t pkt_data;
	volatile u32 tk1, tk2;

	audio_pkt_t pkt_begin = { .type = AUDIO_PKT_BEGIN, .data = NULL, .data_len = 0 };
	xQueueSend( audio_pkt_queue, ( void * ) &pkt_begin, portMAX_DELAY );
	audio_queue_increase();

	tk1 = rtw_get_current_time();

	HTTP_LOGD("left bytes %d", info->bytesleft);
	do {
		if (info->bytesleft == 0) { //decode_bytesleft > 0,last decode done but has some bytes remaining,need read more
			if ((recv_bytes = read(info->server_fd, info->buffer, except_size)) <= 0) {
				HTTP_LOGW("WARNING: read ret %d, continue\n", recv_bytes);
				if (info->server_fd >= 0)
					close(info->server_fd);

				if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread)
					break;

				if (retry_cnt ++ < MAX_RETYR_CNT) {
					
					if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread)
						break;
					http_connect_server_and_send_request(info);
					retry_flag = 1;
					vTaskDelay(WHILE_DELAY);
					continue;
				} else {
					HTTP_LOGE("Error: read fail\n");
					err = DUER_PLAY_ERROR_READ_FAIL;
					goto Exit;
				}
			}

			retry_cnt = 0;
			if (retry_flag) {
				printf("\r\n bytesLeft %d\r\n", info->bytesleft);
				http_remove_header(info, &recv_bytes);
				retry_flag = 0;
			}
			info->offset += recv_bytes;  //update current offset
			MP3Info.cur_download_offset = info->offset;
			MP3Info.downloadBytes += recv_bytes;  //bytes downloaded in current playing
			info->bytesleft += recv_bytes;
			info->cur_ptr = info->buffer;
			HTTP_LOGD("++recv_bytes:%d,bytesleft %d", recv_bytes, info->bytesleft);
		}

		if (info->bytesleft > 0) {

			pkt_data.type = AUDIO_PKT_DATA;
			pkt_data.data_len = info->bytesleft;
			pkt_data.data = (uint8_t *) DUER_MALLOC_SDRAM (info->bytesleft);

			while ( pkt_data.data == NULL) {
				DUER_LOGD("Malloc pkt buffer fail, waiting ...");
				vTaskDelay(500);
				pkt_data.data = (uint8_t *) DUER_MALLOC_SDRAM (info->bytesleft);
			}
			memcpy( pkt_data.data, info->cur_ptr, info->bytesleft);

			// for queue block
			while (s_queue_len > AUDIO_PKT_QUEUE_LENGTH - 1) {
				// when decode stop, exit download thread
				if (pDuerAppInfo->duerStopByPlayThread || pDuerAppInfo->duer_play_stop_flag){
					DUER_LOGW("play exited or stop by other");
					break;
				}
				DUER_LOGD("audio queue(%d) full, waiting...", s_queue_len);
				vTaskDelay(100);
			}
			
			xQueueSend( audio_pkt_queue, ( void * ) &pkt_data, portMAX_DELAY );
			audio_queue_increase();

			info->bytesleft = 0;
			info->cur_ptr = info->buffer;
		}
		vTaskDelay(WHILE_DELAY);
		condition = info->range_len <= 0 ? (MP3Info.downloadBytes < info->content_len) : (info->offset < info->range_len);
	} while (!pDuerAppInfo->duer_play_stop_flag && !pDuerAppInfo->duerStopByPlayThread && condition);

Exit:

	tk2 = rtw_get_current_time() - tk1;
	HTTP_LOGI("download-unchucked exit, downloadBytes %d / content_len %d, speed: %f kb/s\n", MP3Info.downloadBytes
	          , info->content_len, (MP3Info.downloadBytes * 1.0 / 1024) / (tk2 * 1.0 / 1000));
	return 0;
}

static int http_process_chucked(http_t *info) {

	int err = DUER_PLAY_OK;
	int condition = 1;
	int retry_cnt = 0;
	int retry_flag = 0;
	int recv_bytes = 0;
	int except_size = RCV_BUF_SIZE;
	int header_len = 0;
	int sz = 0;
	int first_process = 1;

	char *ptr1 = NULL; //pointe to start of undecode
	char *ptr2 = NULL; //pointe to start of unparse
	char *ptr3 = NULL; //in parse, current ptr
	char *ptrEnd = NULL;  // point to end of bytes in info->buffer
	char *chunkEnd = NULL;

	audio_pkt_t pkt_data;
	duer_chunk_state state = state_chunk_size_processing;
	HTTP_LOGI("left bytes %d", info->bytesleft);


	audio_pkt_t pkt_begin = { .type = AUDIO_PKT_BEGIN, .data = NULL, .data_len = 0 };
	xQueueSend( audio_pkt_queue, ( void * ) &pkt_begin, portMAX_DELAY );
	audio_queue_increase();

	do {
		if ((info->bytesleft == 0 && first_process) || !first_process ) {

			except_size = RCV_BUF_SIZE - info->bytesleft;
			memset(info->buffer + info->bytesleft, 0x0, except_size);
			if ((recv_bytes = read(info->server_fd, info->buffer + info->bytesleft, except_size)) <= 0) {
				HTTP_LOGW("WARNING: read ret %d,continue\n", recv_bytes);
				if (info->server_fd >= 0)
					close(info->server_fd);

				if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread)
					break;

				if (retry_cnt ++ < MAX_RETYR_CNT) {
					http_connect_server_and_send_request(info);
					retry_flag = 1;
					continue;
				} else {
					HTTP_LOGE("Error: read fail\n");
					err = DUER_PLAY_ERROR_READ_FAIL;
					goto Exit;
				}
			}
			if (retry_flag) {
				http_remove_header(info, &recv_bytes);
				state = state_chunk_size_processing;
				//info->chunked_len = 0;
				retry_flag = 0;
			}
			info->offset += recv_bytes;  //update current offset
			info->bytesleft += recv_bytes;   //bytes in buffer now
			HTTP_LOGD("++recv_bytes:%d,bytesleft %d", recv_bytes, info->bytesleft);
		} else {
			memmove(info->buffer, info->cur_ptr, info->bytesleft);
			memset(info->buffer + info->bytesleft, 0x00, RCV_BUF_SIZE - info->bytesleft);
		}

		ptr1 = info->buffer;
		ptr3 = ptr2 = ptr1;   //ptr3: cur ptr
		ptrEnd = ptr1 + info->bytesleft;

		// parse chunk data and insert into info->buffer
		while (ptr3 < ptrEnd) {

			if (state == state_chunk_size_processing) {

				chunkEnd = strstr(ptr3, "\r\n");
				if (chunkEnd) {
					info->chunked_len = strtol(ptr3, NULL, 16);
					HTTP_LOGD("part len %d", info->chunked_len);
					if (info->chunked_len == 0) {
						HTTP_LOGI("chunked over!");
						break;
					}
					ptr3 = chunkEnd + 2;  // now ptr3 point to chunk_data_begin
					state = state_chunk_data_processing;
				} else {
					// size info not enough, not a complete chunk, need read
					sz = ptrEnd - ptr3;
					HTTP_LOGD("sz %d", sz);  // 1
					memmove(ptr2, ptr3, sz); // copy left to unparse zone
					info->bytesleft = ptr2 - ptr1 + sz;
					memset(ptr2 + sz, 0x00, RCV_BUF_SIZE - info->bytesleft);
					ptrEnd = ptr2 +  sz;
					state = state_chunk_size_processing;
					break;
				}
			}

			if (state == state_chunk_data_processing) {

				if ((ptrEnd - ptr3) >= info->chunked_len) {
					memmove(ptr2, ptr3, info->chunked_len);
					HTTP_LOGD("enough ,insert %d bytes", info->chunked_len);  //1437

					info->offset += info->chunked_len;
					MP3Info.cur_download_offset = info->offset;

					ptr2 += info->chunked_len;
					ptr3 += info->chunked_len;
					state = state_chunk_r_processing;
				} else {
					//data info not enough, not a complete chunk, need read
					sz = ptrEnd - ptr3;
					memmove(ptr2, ptr3, sz);
					HTTP_LOGD("not enough ,insert %d bytes", sz);

					info->offset += sz;
					//MP3Info.downloadBytes += sz;
					MP3Info.cur_download_offset = info->offset;

					ptr2 += sz;
					ptr3 = ptr2;
					info->chunked_len -= sz;
					HTTP_LOGD("++in data processing, partlen %d", info->chunked_len);

					sz = 0;
					info->bytesleft = ptr2 - ptr1;
					memset(ptr2, 0x00, RCV_BUF_SIZE - info->bytesleft);
					break;
				}
			}

			if (state == state_chunk_r_processing) {
				if ((ptrEnd - ptr3) == 0) {
					break;
				}
				ptr3++;
				state = state_chunk_n_processing;
			}

			if (state == state_chunk_n_processing) {
				if ((ptrEnd - ptr3)  == 0) {
					break;
				}
				ptr3++;
				state = state_chunk_size_processing;
			}
			vTaskDelay(WHILE_DELAY);
		}

		info->bytesleft = ptr2 - ptr1 + sz;
		HTTP_LOGD("++bytesleft %d, sz:%d", info->bytesleft, sz);
		if (info->bytesleft - sz < 0) {
			goto Exit;
		}

		// decode mp3 before read
		info->cur_ptr = ptr1;
		HTTP_LOGD("++sz: %d", sz);

		if (info->bytesleft - sz > 0) {
			pkt_data.type = AUDIO_PKT_DATA;
			pkt_data.data_len = info->bytesleft - sz;
			pkt_data.data = (uint8_t *) DUER_MALLOC_SDRAM (pkt_data.data_len);
			MP3Info.downloadBytes += pkt_data.data_len;

			while ( pkt_data.data == NULL) {
				DUER_LOGD("Malloc pkt buffer fail, waiting ...");
				vTaskDelay(500);
				pkt_data.data = (uint8_t *) DUER_MALLOC_SDRAM (pkt_data.data_len);
			}
			memcpy( pkt_data.data, info->cur_ptr, pkt_data.data_len);

			// for queue block
			while (s_queue_len > AUDIO_PKT_QUEUE_LENGTH - 1) {
				// when decode stop, exit download thread
				if (pDuerAppInfo->duerStopByPlayThread || pDuerAppInfo->duer_play_stop_flag){
					DUER_LOGW("play exited or stop by other");
					break;
				}
				DUER_LOGD("audio queue(%d) full, waiting...",s_queue_len);
				vTaskDelay(100);
			}
			xQueueSend( audio_pkt_queue, ( void * ) &pkt_data, portMAX_DELAY );
			audio_queue_increase();	
		}

		if(sz > 0)
			memmove(info->buffer, info->cur_ptr + info->bytesleft - sz, sz);
		info->bytesleft = sz;
		sz = 0;
		first_process = 0;
		vTaskDelay(WHILE_DELAY);
	} while (!pDuerAppInfo->duer_play_stop_flag && !pDuerAppInfo->duerStopByPlayThread && info->chunked_len != 0);

Exit:

	HTTP_LOGI("download-chunked exit, download offset %d / range_len %d\r\n", MP3Info.downloadBytes, info->range_len);
	return 0;
}


void file_download_thread(void *param) {
	volatile u32 tim0, tim1;
	int reason = DUER_EXIT_NO_ERROR;
	int retry_cnt = 0;
	int recv_bytes = 0;
	int except_size = RCV_BUF_SIZE;
	char *redirect_url = NULL;
	http_t *info = NULL;

	audio_pkt_t pkt_end = { .type = AUDIO_PKT_END, .data = NULL, .data_len = 0 };

	duer_media_param_t *duer_param = (duer_media_param_t*)param;
	char *downloadurl = duer_param->download_url;

Http_Force_Retry:
	info = (http_t *)DUER_MALLOC(sizeof(http_t));
	if (!info) {
		HTTP_LOGE( "malloc failed\n");
		return;
	}
	memset(info, 0x0, sizeof(http_t));
	info->server_fd = -1;
	info->offset = duer_param->offset;

	MP3Info.downloadBytes = 0;

//Http_Force_Retry:
	//get host info
	if (http_get_host_info(downloadurl, info)) {
		HTTP_LOGE("http download thread: Get host and url failed!");
		reason = DUER_EXIT_GET_HOST_URL_FAILED;
		goto exit;
	}

	if (http_connect_server_and_send_request(info)) {
		reason = DUER_EXIT_POST_DATA_FAILED;
		goto exit;
	}

	//malloc recv buf
	info->buffer = (char *)DUER_MALLOC(RCV_BUF_SIZE + 1);
	//info->buffer = (char *) DUER_MALLOC_SDRAM (RCV_BUF_SIZE + 1); // use sdram
	if (NULL == info->buffer) {
		HTTP_LOGE("http download thread: Allocate rcv_buf failed!");
		reason = DUER_EXIT_MALLOC_FAILED;
		goto exit;
	}
	memset(info->buffer, 0, RCV_BUF_SIZE + 1);

	except_size = RCV_BUF_SIZE;
	while (retry_cnt < MAX_RETYR_CNT) {

		if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread)
			break;
		
		if ((recv_bytes = read(info->server_fd, info->buffer, except_size)) <= 0) {
			HTTP_LOGW("WARNING: read ret %d,goto Try_again\n", recv_bytes);
			if (info->server_fd >= 0)
				close(info->server_fd);
			http_connect_server_and_send_request(info);
			retry_cnt ++;
			continue;
		} else {
			//parse http header,we can know whether chunked or not
			reason = http_parse_header(info, recv_bytes);
			if (reason == 0) { //parse header OK
				break;
			} else { //error
				HTTP_LOGE("ERROR: process http header failed, reason %d", reason);
				reason = DUER_EXIT_HTTP_HEADER_PARSE_FAILED;
				goto exit;
			}
		}
		vTaskDelay(WHILE_DELAY);
	}

	switch (info->status_code) {
	case HTTP_OK:
	case HTTP_PARTICAL:
		if (!info->chunked_flag) {
			http_process_unchucked(info);
		} else {
			http_process_chucked(info);
		}

		break;
	case HTTP_REDIRECT:
		HTTP_LOGI("redirect: %s\n", info->redirect_url);
		redirect_url = (char *)DUER_MALLOC(URI_MAX_LEN);
		memset(redirect_url, 0x0, URI_MAX_LEN);
		strncpy(redirect_url, info->redirect_url, URI_MAX_LEN - 1);
		clean_up(info);
		downloadurl = redirect_url;
		retry_cnt = 0;
		goto Http_Force_Retry;
	case HTTP_NOT_FOUND:
		HTTP_LOGE("Page not found\n");
		goto exit;
	default:
		HTTP_LOGI("Not supported http code %d\n", info->status_code);
		goto exit;
	}

exit:
	xQueueSend( audio_pkt_queue, ( void * ) &pkt_end, portMAX_DELAY );
	audio_queue_increase();
	xSemaphoreTake(pDuerAppInfo->xSemaphoreMediaPlayDone, portMAX_DELAY);

	if (pDuerAppInfo->duer_play_stop_flag)  //judge this flag first
		reason = DUER_PLAY_STOP_BY_FLAG;
	else if ((info->chunked_len == 0 && MP3Info.ddt == SPEAK) || info->offset >= info->content_len)
		reason = DUER_PLAY_OK;
	else
		reason = DUER_PLAY_ERROR_UNKNOWN;

	http_play_before_finish(reason);

	audio_queue_reset();
	clean_up(info);
	SAFE_FREE(redirect_url);

	HTTP_LOGI("download thread exit!");
	//SAFE_FREE(duer_param->download_url);
	xSemaphoreGive(pDuerAppInfo->xSemaphoreHttpDownloadDone);

	vTaskDelete(NULL);
}

static void duer_mute_callback(void *param)
{
	HTTP_LOGI("reset mute");
	duer_media_set_mute(false);
	pDuerAppInfo->muteResetFlag = true;
}

void audio_play_http_mp3()
{
	audio_pkt_t pkt;
	player_t *mp3_para = NULL;
	mp3_info_t frame_info = {0};

	char *inbuf;
	int bytesLeft = 0;
	int ret, offset;
	int first_frame = 1;
	int first_cache = 1;
	int timeout = 0;

	int MaxWaitMS = 0;
	int MaxWaitQLEN = 0;

	uint32_t mp3_data_cache_len = 0;
	uint8_t* mp3_data_cache = (uint8_t*)DUER_MALLOC_SDRAM(MP3_DATA_CACHE_SIZE);
	memset(mp3_data_cache, 0x00, MP3_DATA_CACHE_SIZE);
	
	MaxWaitMS = (MP3Info.ddt == AUDIO) ? 8000 : 100;
	//MaxWaitQLEN = (MP3Info.ddt == AUDIO) ? AUDIO_PKT_QUEUE_LENGTH * 2 / 3 : 1;
	MaxWaitQLEN = (MP3Info.ddt == AUDIO) ? AUDIO_PKT_QUEUE_LENGTH - 1 : 1;

	duer_timer_handler mute_handle = NULL;

	mp3_para = (player_t *) DUER_MALLOC(sizeof(player_t));
	memset(mp3_para, 0x00, sizeof(player_t));
	memset(pcm_buf, 0x00, AUDIO_MAX_DMA_PAGE_SIZE);

	pDuerAppInfo->muteResetFlag = false;

	while (!pDuerAppInfo->duer_play_stop_flag && !pDuerAppInfo->duerStopByPlayThread) {

		if (xQueueReceive( audio_pkt_queue, &pkt, 50/*portMAX_DELAY*/ ) != pdTRUE) {
			if (!first_cache) {
				pDuerAppInfo->duerPlayStuttered = true;
				duer_dcs_audio_on_stuttered(true);
				MaxWaitMS = (Rand2() % 5 + 3) * 1000;
			} else {
				MaxWaitMS = 2000;
			}

			while (s_queue_len < MaxWaitQLEN && timeout < MaxWaitMS) {

				if (pDuerAppInfo->duer_play_stop_flag)
					goto Exit;

				vTaskDelay(100);
				timeout += 100;
			}
			HTTP_LOGW("No pkt,waiting %dms,queue_len %d...\n", timeout, s_queue_len);

			first_cache = 0;
			timeout = 0;
			continue;
		}

		audio_queue_decrease();

		if (pkt.type == AUDIO_PKT_BEGIN) {
			mp3_data_cache_len = 0;
			memset(mp3_data_cache, 0x00, MP3_DATA_CACHE_SIZE);
		}

		if (pkt.type == AUDIO_PKT_DATA) {
			if (mp3_data_cache_len + pkt.data_len >= MP3_DATA_CACHE_SIZE) {
				HTTP_LOGD("mp3 data cache overflow %d %d\r\n", mp3_data_cache_len, pkt.data_len);
				memset(mp3_data_cache, 0x00, MP3_DATA_CACHE_SIZE);
				mp3_data_cache_len = 0;
				memcpy( mp3_data_cache + mp3_data_cache_len, pkt.data, pkt.data_len );
				mp3_data_cache_len += pkt.data_len;
			} else {
				memcpy( mp3_data_cache + mp3_data_cache_len, pkt.data, pkt.data_len );
				mp3_data_cache_len += pkt.data_len;
			}

			MP3Info.cur_played_offset += pkt.data_len;
			SAFE_FREE(pkt.data);

			inbuf = mp3_data_cache;
			bytesLeft = mp3_data_cache_len;

			while (bytesLeft > mp3_para->frame_size) {

				if(pDuerAppInfo->duer_play_stop_flag) // stop by user
					goto Exit;

				if (first_frame) {
					offset = MP3FindSyncWord(inbuf, bytesLeft);  //27

					if (offset >= 0) {
						if (MP3GetFirstFrameInfo(&MP3Info, &inbuf[offset], bytesLeft - offset) == 0) {
							inbuf += offset;
							bytesLeft -= offset;

							mp3_para->nb_channels = MP3Info.nb_channels;
							mp3_para->sample_rate = MP3Info.sample_rate;
							mp3_para->bit_rate = MP3Info.bit_rate;
							mp3_para->frame_size = MP3Info.frame_size;

							HTTP_LOGI("frame info: offset %d, br %d, nChans %d, sr %d, frame_size %d",
							          offset, mp3_para->bit_rate, mp3_para->nb_channels,
							          mp3_para->sample_rate, mp3_para->frame_size);

							ret = initialize_audio_as_player(mp3_para);
							if (ret != 0) {
								HTTP_LOGE("ERROR: MP3 format not support\n");
								ret = DUER_PLAY_ERROR_UNSUPPORT_CODEC;
								pDuerAppInfo->duerStopByPlayThread = true;
								goto Exit; // notify downloadthread end
							}
							mp3_para->last_sr = mp3_para->sample_rate;

							if (pDuerAppInfo->mute == false && MP3Info.ddt == AUDIO) {
								mute_handle = duer_timer_acquire(duer_mute_callback, NULL, DUER_TIMER_ONCE);
								if (mute_handle) {
									duer_media_set_mute(true);
									duer_timer_start(mute_handle, 100);
								}
							}
							audio_player_start();
							first_frame = 0;
						}
					} else {
						HTTP_LOGE("err: offset %d", offset);
						break;
					}
				}

				HTTP_LOGD("++in %d", bytesLeft);

				offset = MP3FindSyncWord(inbuf, bytesLeft);
				HTTP_LOGD("offset %d  %p", offset, inbuf);

				if (offset >= 0 && MP3GetNextFrameInfo(mp3_para, &inbuf[offset]) == 0) {

					inbuf += offset;
					bytesLeft -= offset;
					if (bytesLeft < mp3_para->frame_size) {
						HTTP_LOGW("warning : data underflow, cache more...");
						vTaskDelay(3000); //cache more
						break;
					}

					mp3_decode(mp3, inbuf, mp3_para->frame_size, pcm_buf, &frame_info);
					if (frame_info.audio_bytes <= 0) {
						HTTP_LOGE("err: %d\r\n", ret);
						// cleanup the bad frame
						inbuf += mp3_para->frame_size;
						bytesLeft -= mp3_para->frame_size;
						break;
					} else {
						if (frame_info.sample_rate != 0 && mp3_para->last_sr != frame_info.sample_rate) {
							DUER_LOGI("change sample rate %d > %d", mp3_para->last_sr, frame_info.sample_rate);
							mp3_para->sample_rate = frame_info.sample_rate;
							mp3_para->nb_channels = frame_info.channels;
							initialize_audio_as_player(mp3_para);
							mp3_para->last_sr = frame_info.sample_rate;
						}
						audio_play_pcm(pcm_buf, frame_info.audio_bytes);
						inbuf += mp3_para->frame_size;
						bytesLeft -= mp3_para->frame_size;
					}
				} else {
					// cleanup the bad frame
					HTTP_LOGW("warning : bad frame\r\n");
					inbuf += mp3_para->frame_size;
					bytesLeft -= mp3_para->frame_size;
					break;
				}
				//vTaskDelay(WHILE_DELAY);
			}

			HTTP_LOGD("bytesLeft %d ,mp3_data_cache_len %d", bytesLeft, mp3_data_cache_len);
			if (bytesLeft > 0) {
				memmove(mp3_data_cache, mp3_data_cache + mp3_data_cache_len - bytesLeft, bytesLeft);
				mp3_data_cache_len = bytesLeft;
			} else {
				mp3_data_cache_len = 0;
			}
		}

		if (pkt.type == AUDIO_PKT_END) {
			break;
		}
		//vTaskDelay(WHILE_DELAY);
	}

Exit:
	if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread) {
		MP3Info.cur_played_offset = MP3Info.cur_played_offset > bytesLeft ?	(MP3Info.cur_played_offset - bytesLeft) : MP3Info.cur_played_offset;
		MP3Info.cur_played_offset = MP3Info.cur_played_offset > mp3_para->frame_size ?	(MP3Info.cur_played_offset - mp3_para->frame_size) : MP3Info.cur_played_offset;
		while (xQueueReceive( audio_pkt_queue, &pkt, 0 ) == pdTRUE ) {
			audio_queue_decrease();
			SAFE_FREE(pkt.data);
		}
		HTTP_LOGI("audio queue cleared");
	}

	pDuerAppInfo->duerStopByPlayThread = true;

	HTTP_LOGI("play finished, decoded offset %d\r\n", MP3Info.cur_played_offset);

	if (mute_handle && !pDuerAppInfo->muteResetFlag) { // not reset
		if (duer_media_get_mute()) {
			duer_media_set_mute(false);
		}
		duer_timer_release(mute_handle);
		mute_handle = NULL;
	}
	SAFE_FREE(mp3_para);
	SAFE_FREE(mp3_data_cache);
	xSemaphoreGive(pDuerAppInfo->xSemaphoreMediaPlayDone);
}


void audio_play_http_wav() {
	audio_pkt_t pkt;
	char *inbuf;
	player_t *WAVPara = NULL;
	mp3_info_t frame_info = {0};

	int ret = 0;
	int bytesLeft = 0;
	int first_frame = 1;
	int first_cache = 1;
	int timeout = 0;
	int WAVHeaderLen = sizeof(struct WAV_Format);

	int MaxWaitMS = 0;
	int MaxWaitQLEN = 0;

	uint32_t wav_data_cache_len = 0;
	uint8_t* wav_data_cache = (uint8_t*)DUER_MALLOC_SDRAM(WAV_DATA_CACHE_SIZE);
	memset(wav_data_cache, 0x00, WAV_DATA_CACHE_SIZE);
	
	MaxWaitMS = (MP3Info.ddt == AUDIO) ? 8000 : 100;
	MaxWaitQLEN = (MP3Info.ddt == AUDIO) ? AUDIO_PKT_QUEUE_LENGTH * 2 / 3 : 1;

	duer_timer_handler mute_handle = NULL;

	WAVPara = (player_t *) DUER_MALLOC(sizeof(player_t));
	memset(WAVPara, 0x00, sizeof(player_t));

	pDuerAppInfo->muteResetFlag = false;

	while (!pDuerAppInfo->duer_play_stop_flag && !pDuerAppInfo->duerStopByPlayThread) {

		if (xQueueReceive( audio_pkt_queue, &pkt, 50/*portMAX_DELAY*/ ) != pdTRUE) {
			if (!first_cache) {
				pDuerAppInfo->duerPlayStuttered = true;
				duer_dcs_audio_on_stuttered(true);
				MaxWaitMS = (Rand2() % 5 + 3) * 1000;
			} else {
				MaxWaitMS = 2000;
			}

			while (s_queue_len < MaxWaitQLEN && timeout < MaxWaitMS) {

				if (pDuerAppInfo->duer_play_stop_flag)
					goto Exit;

				vTaskDelay(100);
				timeout += 100;
			}
			HTTP_LOGW("No pkt,waiting %dms,queue_len %d...\n", timeout, s_queue_len);

			first_cache = 0;
			timeout = 0;
			continue;
		}
		audio_queue_decrease();

		if (pkt.type == AUDIO_PKT_BEGIN) {
			wav_data_cache_len = 0;
			memset(wav_data_cache, 0x00, WAV_DATA_CACHE_SIZE);
		}

		if (pkt.type == AUDIO_PKT_DATA) {
			if (wav_data_cache_len + pkt.data_len >= WAV_DATA_CACHE_SIZE) {
				HTTP_LOGD("mp3 data cache overflow %d %d\r\n", wav_data_cache_len, pkt.data_len);
				memset(wav_data_cache, 0x00, MP3_DATA_CACHE_SIZE);
				wav_data_cache_len = 0;
				memcpy( wav_data_cache + wav_data_cache_len, pkt.data, pkt.data_len );
				wav_data_cache_len += pkt.data_len;
			} else {
				memcpy( wav_data_cache + wav_data_cache_len, pkt.data, pkt.data_len );
				wav_data_cache_len += pkt.data_len;
			}

			MP3Info.cur_played_offset += pkt.data_len;
			SAFE_FREE(pkt.data);

			inbuf = wav_data_cache;
			bytesLeft = wav_data_cache_len;

			while (bytesLeft > WAVPara->audio_bytes) {
				if(pDuerAppInfo->duer_play_stop_flag) // stop by user
					goto Exit;
				
				// get wav info
				if (first_frame) {
					if(bytesLeft >= WAVHeaderLen){
						if(WAVGetMediaInfo(WAVPara, inbuf, bytesLeft) != 0){
							DUER_LOGE("error: can't get wav info");
							ret = DUER_PLAY_ERROR_UNSUPPORT_CODEC;
							pDuerAppInfo->duerStopByPlayThread = true;
							goto Exit; // notify downloadthread end	
						}

						inbuf += WAVHeaderLen;
						bytesLeft -= WAVHeaderLen;
						initialize_audio_as_player(WAVPara);
						WAVPara->last_sr = WAVPara->sample_rate;
						HTTP_LOGI("frame info: br %d, nChans %d, sr %d, frame_size %d, audioBytes %d",
							          WAVPara->bit_rate, WAVPara->nb_channels,
							          WAVPara->sample_rate, WAVPara->frame_size, WAVPara->audio_bytes);

						if (pDuerAppInfo->mute == false && MP3Info.ddt == AUDIO) {
							mute_handle = duer_timer_acquire(duer_mute_callback, NULL, DUER_TIMER_ONCE);
							if (mute_handle) {
								duer_media_set_mute(true);
								duer_timer_start(mute_handle, 100);
							}
						}
						audio_player_start();
						first_frame = 0;
					}else {
						HTTP_LOGW("warning : data underflow, cache more ...");
						vTaskDelay(3000); //cache more
						break;
					}
				}

				// feed pcm to codec
				HTTP_LOGD("++in %d, frame_size %d", bytesLeft, WAVPara->frame_size);
				if (bytesLeft < WAVPara->audio_bytes) {
					HTTP_LOGW("warning : data underflow, cache more ...");
					vTaskDelay(3000); //cache more
					break;
				} else {
					audio_play_pcm(inbuf, WAVPara->audio_bytes);
					inbuf += WAVPara->audio_bytes;
					bytesLeft -= WAVPara->audio_bytes;
				}		
			}
			HTTP_LOGD("bytesLeft %d ,mp3_data_cache_len %d", bytesLeft, wav_data_cache_len);
			if (bytesLeft > 0) {
				memmove(wav_data_cache, wav_data_cache + wav_data_cache_len - bytesLeft, bytesLeft);
				wav_data_cache_len = bytesLeft;
			} else {
				wav_data_cache_len = 0;
			}
		}

		if (pkt.type == AUDIO_PKT_END) {
			break;
		}
		//vTaskDelay(WHILE_DELAY);
	}

Exit:
	if (pDuerAppInfo->duer_play_stop_flag || pDuerAppInfo->duerStopByPlayThread) {
		MP3Info.cur_played_offset = MP3Info.cur_played_offset > bytesLeft ?	(MP3Info.cur_played_offset - bytesLeft) : MP3Info.cur_played_offset;
		MP3Info.cur_played_offset = MP3Info.cur_played_offset > WAVPara->frame_size ?	(MP3Info.cur_played_offset - WAVPara->frame_size) : MP3Info.cur_played_offset;
		while (xQueueReceive( audio_pkt_queue, &pkt, 0 ) == pdTRUE ) {
			audio_queue_decrease();
			SAFE_FREE(pkt.data);
		}
		HTTP_LOGI("audio queue cleared");
	}

	pDuerAppInfo->duerStopByPlayThread = true;
	HTTP_LOGI("play finished, decoded offset %d\r\n", MP3Info.cur_played_offset);

	if (mute_handle && !pDuerAppInfo->muteResetFlag) { // not reset
		if (duer_media_get_mute()) {
			duer_media_set_mute(false);
		}
		duer_timer_release(mute_handle);
		mute_handle = NULL;
	}
	SAFE_FREE(WAVPara);
	SAFE_FREE(wav_data_cache);
	xSemaphoreGive(pDuerAppInfo->xSemaphoreMediaPlayDone);
}

void duer_http_audio_thread(void* param)
{	
	xSemaphoreTake(pDuerAppInfo->xSemaphoreHTTPAudioThread, portMAX_DELAY);
	audio_pkt_t pkt;
	duer_media_param_t *media_param = (duer_media_param_t*)param;
	if(pDuerAppInfo->voice_is_triggered || pDuerAppInfo->audio_is_pocessing){
		DUER_LOGE("recording in processing..exit play");
		goto Exit;
	}
	
	pDuerAppInfo->duer_play_stop_flag = 0;
	pDuerAppInfo->audio_is_playing = 1;
	if(!audio_pkt_queue)
		audio_pkt_queue = xQueueCreate(AUDIO_PKT_QUEUE_LENGTH, sizeof(audio_pkt_t));
	if(!s_qlen_lock)
		s_qlen_lock = duer_mutex_create();

	DUER_LOGE("duer_http_audio_thread enter");
	if (!s_qlen_lock) {
		DUER_LOGE("Failed to create s_qlen_lock");
	}
	audio_queue_reset();

	MP3Info.ddt = media_param->ddt;
	MP3Info.cur_played_offset = media_param->offset;
	if (media_param->ddt == ALERT) {
		strcpy(MP3Info.token, media_param->token);
	}
	
	pDuerAppInfo->duerPlayStuttered = false;
	pDuerAppInfo->duerStopByPlayThread = false;

	if (xTaskCreate(file_download_thread, ((const char*)"file_download_thread"), 1024, media_param, tskIDLE_PRIORITY + 3, NULL) != pdPASS) {
		printf("\n\r%s xTaskCreate(file_download_thread) failed", __FUNCTION__);
		goto Error;
	}
#ifdef DUER_SUPPORT_WAV
	if (strstr( media_param->download_url, ".wav")) {
		DUER_LOGI("wav file, playing...");
		audio_play_http_wav();
	} else {
#endif
		audio_play_http_mp3();
#ifdef DUER_SUPPORT_WAV
	}
#endif
	xSemaphoreTake(pDuerAppInfo->xSemaphoreHttpDownloadDone, portMAX_DELAY);
	while (xQueueReceive( audio_pkt_queue, &pkt, 0 ) == pdTRUE ) {
		audio_queue_decrease();
		SAFE_FREE(pkt.data);

	}
	
Error:
	duer_mutex_destroy(s_qlen_lock); 
	s_qlen_lock = NULL;
	vQueueDelete(audio_pkt_queue);
	audio_pkt_queue = NULL;
	
Exit:	
	//SAFE_FREE(media_param->download_url);
	pDuerAppInfo->audio_is_playing = 0;
	xSemaphoreGive(pDuerAppInfo->xSemaphoreHTTPAudioThread);
	DUER_LOGI("duer_http_audio_thread exit");
	vTaskDelete(NULL);
}
