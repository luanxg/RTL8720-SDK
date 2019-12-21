#include "mmf_source.h"
#include "mmf_source_rtp.h"

//static u8* priv_buf[AUDIO_BUF_DEPTH];

static rtp_exbuf_t rtp_buf;
static int q_depth = 0;

static long listen_time_s = 0;
static long listen_time_us = 20000; //20ms
static uint16_t supported_payload_type = RTP_PT_PCMU;

int rtp_client_start(void *param)
{
	rtp_client_t *rtp_ctx = (rtp_client_t *)param;
	int ret = 0;
	
	if (rtp_ctx->rtp_shutdown)
		return ret;

	u32 start_time, current_time;

	int len, offset;
	rtp_hdr_t rtp_header;
	struct sockaddr_in rtp_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in); 
	fd_set read_fds;
	struct timeval listen_timeout;
	int mode = 0;
	int opt = 1;
	u16 last_seq = 0;
	wext_get_mode(WLAN0_NAME, &mode);
	printf("\n\rwlan mode:%d", mode);
	
	switch(mode)
	{
		case(IW_MODE_MASTER)://AP mode
			while(wifi_is_ready_to_transceive(RTW_AP_INTERFACE) < 0)
			{
				vTaskDelay(1000);
			}
			break;
		case(IW_MODE_INFRA)://STA mode
		case(IW_MODE_AUTO)://when ameba doesn't join bss
			while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) < 0)
			{
				vTaskDelay(1000);
			}
			break;
		default:
			printf("\n\rillegal wlan state!rtp client cannot start");
			return ret;
	}
	
	wext_get_mode(WLAN0_NAME, &mode);
	printf("\n\rwlan mode:%d", mode);
	vTaskDelay(1000);

	if((rtp_ctx->connect_ctx.socket_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("\n\rcreate rtp client socket failed!");
		return 0;//???
	}
	
	if((setsockopt(rtp_ctx->connect_ctx.socket_id, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) < 0){
		printf("\r\n Error on setting socket option");
		goto exit;
	}
	memset(&rtp_addr, 0, addrlen);
	rtp_addr.sin_family = AF_INET;
	rtp_addr.sin_addr.s_addr = INADDR_ANY;
	rtp_addr.sin_port = htons(rtp_ctx->connect_ctx.server_port);
	if (bind(rtp_ctx->connect_ctx.socket_id,(struct sockaddr *)&rtp_addr, addrlen)<0) {
		printf("bind failed\r\n");
		goto exit;
	}
	
	rtp_ctx->rtp_socket_closed = 0;
	while(!rtp_ctx->rtp_shutdown)
	{
		FD_ZERO(&read_fds);
		listen_timeout.tv_sec = listen_time_s;
		listen_timeout.tv_usec = listen_time_us;
		FD_SET(rtp_ctx->connect_ctx.socket_id, &read_fds);
		if(select(MAX_SELECT_SOCKET, &read_fds, NULL, NULL, &listen_timeout))
		{
			//q_depth = uxQueueSpacesAvailable(rtp_ctx->done_queue);
			//if(q_depth > 0)
			//if(xQueueReceive(rtp_ctx->done_queue, (void *)&rtp_buf, 0xFFFFFFFF) != pdTRUE){
			if(xQueueReceive(rtp_ctx->done_queue, &rtp_buf, 1000) != pdTRUE){
				printf("no done_buf\n\r");
				continue;
				//insert silence here 
			}
			//printf("\n\ro");
			memset(rtp_buf.buf, 0, AUDIO_BUF_SIZE);
			len = recvfrom(rtp_ctx->connect_ctx.socket_id, rtp_buf.buf, AUDIO_BUF_SIZE, 0, (struct sockaddr *)&rtp_addr, &addrlen); 
			rtp_buf.len = len;
			//rtp_parse_header(rtp_buf.buf, &rtp_header, 1);
			//printf("\n\r[seq %d] ",ntohs(rtp_header.seq));
			//xQueueSend(rtp_ctx->cache_queue, (void *)&rtp_buf, 0xFFFFFFFF);
			if(xQueueSend(rtp_ctx->cache_queue, (void *)&rtp_buf, 1000) != pdTRUE){
				xQueueSend(rtp_ctx->done_queue, (void *)&rtp_buf, 1000);
				printf("no space for cache_buf\n\r");
				continue;
			}
		}else{
			//printf("\n\r.");
			//insert silence here
		}
	}
exit:
	close(rtp_ctx->connect_ctx.socket_id);
	rtp_ctx->rtp_socket_closed = 1;
	return ret;
}

void rtp_client_init(void *param)
{
	rtp_client_t *rtp_ctx = (rtp_client_t *)param;
	rtw_init_sema(&rtp_ctx->rtp_sema, 0);
	rtp_ctx->rtp_socket_closed = 1;
	while(1)
	{
		if(rtw_down_timeout_sema(&rtp_ctx->rtp_sema,100))
		{
			if(rtp_client_start(rtp_ctx) < 0)
				goto exit;
		}
	}
exit:
	rtp_ctx->rtp_shutdown = 0;
	rtw_free_sema(&rtp_ctx->rtp_sema);
	vTaskDelete(NULL);
}

void* rtp_mod_open(void)
{
	rtp_client_t *rtp_ctx = malloc(sizeof(rtp_client_t));
	if(rtp_ctx == NULL)
		return NULL;
	memset(rtp_ctx, 0, sizeof(rtp_client_t));
	/* set default port */
	rtp_ctx->connect_ctx.socket_id = -1;
	rtp_ctx->connect_ctx.server_port = DEFAULT_RTP_PORT;
	rtp_ctx->rtp_shutdown = 1;
	/* create a rtp client to receive audio data */
	if(xTaskCreate(rtp_client_init, ((const char*)"rtp_client_init"), 512, rtp_ctx, 2, &rtp_ctx->task_handle) != pdPASS) {
		//printf("\r\n geo_rtp_client_init: Create Task Error\n");
		free(rtp_ctx);
		return NULL;
	}
	return (void*)rtp_ctx;
}

void rtp_mod_close(void* ctx)
{
	rtp_client_t *rtp_ctx = (rtp_client_t *)ctx;
	if(!rtp_ctx->task_handle && xTaskGetCurrentTaskHandle()!=rtp_ctx->task_handle)
		vTaskDelete(rtp_ctx->task_handle);
	free(rtp_ctx);
}


int rtp_mod_set_param(void* ctx, int cmd, int arg)
{
	int ret = 0;
	rtp_client_t *rtp_ctx = (rtp_client_t *)ctx;
	
	switch(cmd)
	{
		case CMD_SET_STREAMMING:
			if(arg == ON) {	// stream on
				//printf("\n\r\n\r*****CMD_SET_STREAMMING:  up_sema\n\r");
				rtp_ctx->rtp_shutdown = 0;
				rtw_up_sema(&rtp_ctx->rtp_sema);
			}
			else {			// stream off
				//printf("\n\r\n\r*****CMD_SET_STREAMMING:rtp_shutdown\n\r");
				rtp_ctx->rtp_shutdown = 1;
				// sleep until socket is closed
				while(!rtp_ctx->rtp_socket_closed) {
					printf("d");
					vTaskDelay(1);
				}
			}	
			break;
		case CMD_RTP_SET_PRIV_BUF:
			if(arg==1) { //initially set queue
				rtp_ctx->cache_queue = xQueueCreate(AUDIO_BUF_DEPTH, sizeof(rtp_exbuf_t));
				rtp_ctx->done_queue = xQueueCreate(AUDIO_BUF_DEPTH, sizeof(rtp_exbuf_t));
			}
			else if(arg==2) { //reset queue
				xQueueReset(rtp_ctx->cache_queue);
				xQueueReset(rtp_ctx->done_queue);
			}
			else if(arg==0) { //delete queue
				vQueueDelete(rtp_ctx->cache_queue);
				vQueueDelete(rtp_ctx->done_queue);
				break;
			}
			else {
				ret = -EINVAL;
				break;
			}
			for(int i = 0; i < AUDIO_BUF_DEPTH; i++) {
				xQueueSend(rtp_ctx->done_queue, (void*)&rtp_buf, 0xFFFFFFFF);
			}
			break;
        case CMD_RTP_SET_SUPPORTED_PAYLOAD_TYPE:
            if (arg == FMT_A_PCMU) {
                supported_payload_type = RTP_PT_PCMU;
            } else if (arg == FMT_A_PCMA) {
                supported_payload_type = RTP_PT_PCMA;
            } else if (arg == FMT_A_AAC_RAW) {
                supported_payload_type = RTP_PT_DYN_BASE;
            }
            break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

//give audio data here

rtp_exbuf_t tmp_exbuf[3];
static int tmp_order = 0;
static u16 last_seq = 0;
int rtp_mod_handle(void* ctx, void* b)
{
	int ret = 0;
	int q_depth = 0;
	rtp_client_t *rtp_ctx = (rtp_client_t *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;   
	rtp_hdr_t rtp_header;
	int offset;
	
	if(exbuf->state==STAT_USED) {
		//xQueueSend(rtp_ctx->done_queue, (void *)&tmp_exbuf[exbuf->index], 0xFFFFFFFF);
		xQueueSend(rtp_ctx->done_queue, (void *)&tmp_exbuf[exbuf->index], 0);
		exbuf->state = STAT_INIT;
	}
	
	//decide if audio cache is enough for send
	q_depth = uxQueueSpacesAvailable(rtp_ctx->cache_queue);
	//printf("\n\rcacheQ-%d\n\r", q_depth);
	if(q_depth >= AUDIO_BUF_DEPTH/2) {
		return -EAGAIN;
	}
	
	if(xQueueReceive(rtp_ctx->cache_queue, (void *)&tmp_exbuf[tmp_order], 0) != pdTRUE) {
		printf("no_cache ");
		return -EAGAIN;
	}
	offset = rtp_parse_header(tmp_exbuf[tmp_order].buf, &rtp_header, 1);
	//printf("\n\r%d-%dB", offset, tmp_exbuf[tmp_order].len);
/* for data loss debugging */
#if 1
	if(last_seq == 0)
		last_seq = ntohs(rtp_header.seq);
	else
	{
		if((ntohs(rtp_header.seq) - last_seq)>1)
			printf("\n\r(%d-%d)", last_seq, ntohs(rtp_header.seq));
		last_seq = ntohs(rtp_header.seq); 
	}
#endif

#if CONFIG_EXAMPLE_MP3_STREAM_RTP    
	if((rtp_header.pt == RTP_PT_PCMU) || (rtp_header.pt == RTP_PT_DYN_BASE) || (rtp_header.pt == RTP_PT_MPA))
#elif CONFIG_PRO_AUDIO_STREAM_RTP
        if((rtp_header.pt == RTP_PT_PCMU) || (rtp_header.pt == RTP_PT_DYN_BASE) || (rtp_header.pt == RTP_PT_MPA)) 
#else
	if(rtp_header.pt == supported_payload_type)
#endif
	{
		if(exbuf->state!=STAT_READY){
			exbuf->index = tmp_order;
			exbuf->data = tmp_exbuf[tmp_order].buf + offset;
			exbuf->len = tmp_exbuf[tmp_order].len - offset;
			//exbuf->timestamp = ?;
			exbuf->state = STAT_READY;
			
			tmp_order++;
			if(tmp_order == 3)
				tmp_order = 0;
			
			mmf_source_add_exbuf_sending_list_all(exbuf);
		}
	}
	else {
		printf("Error: Unsupported rtp_header payload type: %d",rtp_header.pt);
	}
	return ret;
}

msrc_module_t rtp_src_module =
{
	.create = rtp_mod_open,
	.destroy = rtp_mod_close,
	.set_param = rtp_mod_set_param,
	.handle = rtp_mod_handle,
};
