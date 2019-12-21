#include "mmf_source.h"
#include "mmf_source_h1v6_h264.h"

extern struct stream_manage_handle *isp_stm;
#define H1V6_H264_DBG_SHOW 0

TaskHandle_t h1v6_thread_id = NULL;

static void h264_handler(void *param)
{
	u8 *ptr;
	int ret = 0;
	int frame_size = 0;
	int count = 0;
	u32 timestamp = 0;
	u32 hw_timestamp = 0;
	struct encoder_data *ptr_encoder = NULL;
	struct hantro_h264_context *h264_ctx = (struct hantro_h264_context *)param;
	int count_verify =  0;
	
	struct enc_buf_data *enc_data = NULL;
	_irqL 	irqL;
#if H1V6_H264_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int enc_frame = 0;
	static unsigned int enc_frame_all = 0;
	static unsigned int enc_size = 0;
#endif
	
	printf("h264 start\r\n");
	while(1){
		rtw_down_sema(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list.enc_sema);
		do{
			if(h264_ctx->change_parm_cb){
				h264_ctx->change_parm_cb(h264_ctx);
			}
			rtw_enter_critical(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			enc_data = enc_get_acti_buffer(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list);
			rtw_exit_critical(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			if(enc_data == NULL)
				break;
RETRY:
			ptr = memory_alloc(h264_ctx->encoder_lh.memory_ctx,h264_ctx->mem_info_value.mem_frame_size);
			if(ptr == NULL){
				vTaskDelay(10);
				//printf("Not enough buffer [%d]\r\n",h264_ctx->isp_info_value.streamid);
				goto RETRY;
			}
			timestamp = enc_data->timestamp;
			hw_timestamp = enc_data->hw_timestamp;
			h264_ctx->source_addr = (u32)enc_data->y_addr;
			h264_ctx->y_addr = (u32)enc_data->y_addr;
			h264_ctx->uv_addr = (u32)enc_data->uv_addr;
			h264_ctx->dest_addr = (u32)ptr;
			h264_ctx->dest_len = h264_ctx->mem_info_value.mem_frame_size;
#if H1V6_H264_DBG_SHOW
			unsigned int t1 = rtw_get_current_time();
			ret = hantro_encode_h264(h264_ctx);
			unsigned int t2 = rtw_get_current_time();
			//ENCODER_DBG_INFO("[%d] enc_time= %d ",h264_ctx->isp_info_value.streamid,t2-t1);
			enc_frame++;
			enc_frame_all++;
#else
			ret = hantro_encode_h264(h264_ctx);
#endif
			if (h264_ctx->snapshot_encode_cb) {
				int snapshot_enc_ret = h264_ctx->snapshot_encode_cb(h264_ctx->source_addr);
			}
			
			rtw_enter_critical(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			enc_set_idle_buffer(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list,enc_data);
			rtw_exit_critical(&isp_stm->isp_stream[h264_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			if(ret < 0){
				ENCODER_DBG_ERROR("[%d] h1v6 h264 drop frame ret = %d",h264_ctx->isp_info_value.streamid,ret);
				memory_free(h264_ctx->encoder_lh.memory_ctx,ptr);
			}else{
				//printf("id = %d size = %d\r\n",h264_ctx->isp_info_value.streamid,frame_size);
				frame_size = h264_ctx->dest_actual_len;
				memory_realloc(h264_ctx->encoder_lh.memory_ctx,ptr,frame_size);
#if H1V6_H264_DBG_SHOW
				enc_size += frame_size;
#endif
				ptr_encoder = (struct encoder_data *)malloc(sizeof(struct encoder_data));
				if(ptr_encoder == NULL){
					printf("ptr_encoder null\r\n");
					while(1);
				}
				
				ptr_encoder->type = FMT_V_H264;
				ptr_encoder->timestamp = timestamp;
				ptr_encoder->hw_timestamp = hw_timestamp;
				ptr_encoder->addr = ptr;
				ptr_encoder->size = frame_size;
				rtw_mutex_get(&h264_ctx->encoder_lh.list_lock);
				rtw_list_insert_tail(&ptr_encoder->list_data,&h264_ctx->encoder_lh.list);
				rtw_mutex_put(&h264_ctx->encoder_lh.list_lock);
#if H1V6_H264_DBG_SHOW
				if(timer_2 == 0){
					timer_1 = rtw_get_current_time();
					timer_2 = timer_1;
				}else{
					timer_2 = rtw_get_current_time();
					if((timer_2 - timer_1)>=1000){
						ENCODER_DBG_INFO("[%d] encode_frame:%d size:%d frame_total:%d\r\n",jpeg_ctx->isp_info_value.streamid,enc_frame,enc_size,enc_frame_all);
						timer_2 = 0;
						enc_frame = 0;
						enc_size = 0;
					}
				}
#endif
			}
		}while(1);
	}
}


void* h1v6_h264_open(void)
{
	struct hantro_h264_context *h264_ctx = hantro_h264enc_open();
	int i = 0;
	int ret = 0;
	if (h264_ctx == NULL) {
		return NULL;
	}
	
	memset(&h264_ctx->encoder_lh,0,sizeof(struct encoder_list_head));
	rtw_init_listhead(&h264_ctx->encoder_lh.list);
	rtw_mutex_init(&h264_ctx->encoder_lh.list_lock);
	
	return h264_ctx;
}

void h1v6_h264_close(void* ctx)
{
	hantro_h264enc_release(ctx);
}

int h1v6_h264_set_param(void* ctx, int cmd, int arg)
{
	struct hantro_h264_context *h264_ctx = (struct hantro_h264_context *)ctx;
	int ret = 0;
	
	switch(cmd){
		case CMD_ISP_STREAMID:
			h264_ctx->isp_info_value.streamid = arg;
			break;
		case CMD_ISP_HW_SLOT:
			h264_ctx->isp_info_value.hw_slot = arg;
			break;
		case CMD_ISP_SW_SLOT:
			h264_ctx->isp_info_value.sw_slot = arg;
			break;
		case CMD_HANTRO_H264_SET_HEIGHT:
			h264_ctx->h264_parm.height = arg;
			break;
		case CMD_HANTRO_H264_SET_WIDTH:
			h264_ctx->h264_parm.width = arg;
			break;
		case CMD_HANTRO_H264_BITRATE:
			h264_ctx->h264_parm.bps = arg;
			break;
		case CMD_HANTRO_H264_FPS:
			h264_ctx->h264_parm.ratenum = arg;
			break;
		case CMD_HANTRO_H264_GOP:
			h264_ctx->h264_parm.gopLen = arg;
			break;
		case CMD_H1V6_MEMORY_SIZE:
			h264_ctx->mem_info_value.mem_total_size = arg;
			break;
		case CMD_H1V6_BLOCK_SIZE:
			h264_ctx->mem_info_value.mem_block_size = arg;
			break;
		case CMD_H1V6_MAX_FRAME_SIZE:
			h264_ctx->mem_info_value.mem_frame_size = arg;
			break;
		case CMD_HANTRO_H264_RCMODE:
			h264_ctx->h264_parm.rcMode = arg;
			break;
		case CMD_HANTRO_H264_SET_RCPARAM:
		if(1){
			struct h1v6_h264_rc_parm *rc_param = (struct h1v6_h264_rc_parm*)arg;
			h264_ctx->h264_parm.rcMode = rc_param->rcMode;
			h264_ctx->h264_parm.maxQp = rc_param->maxQp;
			h264_ctx->h264_parm.minIQp = rc_param->minIQp;
			h264_ctx->h264_parm.minQp = rc_param->minQp;
			h264_ctx->h264_parm.iQp = rc_param->iQp;
			h264_ctx->h264_parm.pQp = rc_param->pQp;
		}
			break;
		case CMD_HANTRO_H264_GET_RCPARAM:
		if(1){
			struct h1v6_h264_rc_parm *rc_param = (struct h1v6_h264_rc_parm*)arg;
			rc_param->rcMode = h264_ctx->h264_parm.rcMode;
			rc_param->maxQp = h264_ctx->h264_parm.maxQp;
			rc_param->minIQp = h264_ctx->h264_parm.minIQp;
			rc_param->minQp = h264_ctx->h264_parm.minQp;
			rc_param->iQp = h264_ctx->h264_parm.iQp;
			rc_param->pQp = h264_ctx->h264_parm.pQp;
		}
			break;
		case CMD_HANTRO_H264_APPLY:
			h264_ctx->encoder_lh.memory_ctx = memory_init(h264_ctx->mem_info_value.mem_total_size,h264_ctx->mem_info_value.mem_block_size);//(4*1024*1024,512); 
			if(h264_ctx->encoder_lh.memory_ctx == NULL){
				printf("Can't allocate H264 buffer\r\n");
				while(1);
			}
			isp_config(h264_ctx->isp_info_value.streamid,h264_ctx->isp_info_value.hw_slot,h264_ctx->isp_info_value.sw_slot,h264_ctx->h264_parm.ratenum,h264_ctx->h264_parm.width ,h264_ctx->h264_parm.height,ISP_FORMAT_YUV420_SEMIPLANAR);
			if ((ret = hantro_h264enc_initial(h264_ctx,&h264_ctx->h264_parm)) < 0) {
				printf("hantro_h264enc_initial fail\n\r");
				while(1);
			}
			if(xTaskCreate(h264_handler, ((const char*)"h264_handler"), 1024, (void *)h264_ctx, tskIDLE_PRIORITY + 2, &h1v6_thread_id) != pdPASS)
				printf("\n\r%s xTaskCreate failed", __FUNCTION__);
			vTaskDelay(1000);
			isp_start(h264_ctx->isp_info_value.streamid);
			printf("start frame\r\n");
			break;
		case CMD_SNAPSHOT_ENCODE_CB:
			h264_ctx->snapshot_encode_cb = (int (*)(uint32_t))arg;
			break;
                case CMD_HANTRO_H264_CHANGE_PARM_CB:
                        h264_ctx->change_parm_cb = (void (*)(void*))arg;
                        break;
		default:
			ret = -EINVAL;
			break;
	}
	return 0;
}

int h1v6_h264_handle(void* ctx, void* b)
{
	struct encoder_data *ptr_encoder = NULL;
	struct hantro_h264_context *h264_ctx = (struct hantro_h264_context *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	
	if(exbuf->state==STAT_USED){
		exbuf->state = STAT_INIT;
		memory_free(h264_ctx->encoder_lh.memory_ctx,exbuf->data);
	}else{
		printf("exbuf->state = %d\r\n",exbuf->state);
	}
	
	if(exbuf->state!=STAT_READY){
		while(1){
			if(rtw_is_list_empty(&h264_ctx->encoder_lh.list)){
				vTaskDelay(1);
			}else{
				rtw_mutex_get(&h264_ctx->encoder_lh.list_lock);
				ptr_encoder = list_first_entry(&h264_ctx->encoder_lh.list, struct encoder_data, list_data);
				rtw_list_delete(&ptr_encoder->list_data);
				rtw_mutex_put(&h264_ctx->encoder_lh.list_lock);
				break;
			}
		}
		exbuf->codec_fmt = ptr_encoder->type;
		exbuf->len = ptr_encoder->size;
		exbuf->index = 0;
		exbuf->data = ptr_encoder->addr;
		exbuf->timestamp = ptr_encoder->timestamp;
		exbuf->hw_timestamp = ptr_encoder->hw_timestamp;
		exbuf->state = STAT_READY;
		if(ptr_encoder!=NULL){
			free(ptr_encoder);
		}else{
			printf("ptr_encoder == NULL\r\n");
			while(1);
		}
		mmf_source_add_exbuf_sending_list_all(exbuf);
	}
	return 0;
}

msrc_module_t h1v6_h264_module =
{
	.create = h1v6_h264_open,
	.destroy = h1v6_h264_close,
	.set_param = h1v6_h264_set_param,
	.handle = h1v6_h264_handle,
};
