#include "mmf_source.h"
#include "mmf_source_h1v6_jpeg.h"
#include <platform_opts.h>

#include "memory_encoder.h"

#include "cmsis.h"
#include "rtl8195bhp_video.h"
#include "hal_encorder.h"

#include "section_config.h"


#include "basic_types.h"
#include "osdep_service.h"

#include "hal_cache.h"

#include "isp_api.h"

extern struct stream_manage_handle *isp_stm;

#define H1V6_JPEG_DBG_SHOW 0

TaskHandle_t jpeg_thread_id = NULL;

static void jpeg_handler(void *param)
{
	u8 *ptr;
	int ret = 0;
	int frame_size = 0;
	u32 timestamp = 0;
	u32 hw_timestamp = 0;
	//int count = 0;
	struct encoder_data *ptr_encoder = NULL;
	struct hantro_jpeg_context *jpeg_ctx = (struct hantro_jpeg_context *)param;
	//static int count_verify =  0;
	
	struct enc_buf_data *enc_data = NULL;
	_irqL 	irqL;
#if H1V6_JPEG_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int enc_frame = 0;
	static unsigned int enc_frame_all = 0;
	static unsigned int enc_size = 0;
#endif
	printf("jpeg start\r\n");
	while(1){
		rtw_down_sema(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list.enc_sema);
		do{
			rtw_enter_critical(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			enc_data = enc_get_acti_buffer(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list);
			rtw_exit_critical(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			if(enc_data == NULL)
				break;
RETRY:
			ptr = memory_alloc(jpeg_ctx->encoder_jpeg_lh.memory_ctx,jpeg_ctx->mem_info_value.mem_frame_size);
			//dump_info(encoder_jpeg_lh.memory_ctx[ENC_JPEG_STREAM]);
			if(ptr == NULL){
				vTaskDelay(10);
				//printf("Not enough buffer [%d]\r\n",jpeg_ctx->isp_info_value.streamid);
				goto RETRY;
			}
			timestamp = enc_data->timestamp;
			hw_timestamp = enc_data->hw_timestamp;
			jpeg_ctx->source_addr = (u32)enc_data->y_addr;
                        jpeg_ctx->y_addr = (u32)enc_data->y_addr;
                        jpeg_ctx->uv_addr = (u32)enc_data->uv_addr;
			jpeg_ctx->dest_addr = (u32)ptr;
			jpeg_ctx->dest_len = jpeg_ctx->mem_info_value.mem_frame_size;
#if H1V6_JPEG_DBG_SHOW
			unsigned int t1 = rtw_get_current_time();
			ret = hantro_encode_jpeg(jpeg_ctx);
			unsigned int t2 = rtw_get_current_time();
			ENCODER_DBG_INFO("[%d] %d ",jpeg_ctx->isp_info_value.streamid,t2-t1);
			enc_frame++;
			enc_frame_all++;
#else
			ret = hantro_encode_jpeg(jpeg_ctx);
#endif
			if (jpeg_ctx->snapshot_cb) {
				jpeg_ctx->snapshot_cb(jpeg_ctx->dest_addr,jpeg_ctx->dest_actual_len);
			}
			
			rtw_enter_critical(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			enc_set_idle_buffer(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list,enc_data);
			rtw_exit_critical(&isp_stm->isp_stream[jpeg_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
			if(ret < 0){
				ENCODER_DBG_ERROR("[%d] h1v6 jpeg drop frame ret = %d",jpeg_ctx->isp_info_value.streamid,ret);
				memory_free(jpeg_ctx->encoder_jpeg_lh.memory_ctx,ptr);
			}else{
				//printf("h1v6 jpeg size = %d\r\n",jpeg_ctx->dest_actual_len);
				frame_size = jpeg_ctx->dest_actual_len;
				memory_realloc(jpeg_ctx->encoder_jpeg_lh.memory_ctx,ptr,frame_size);
#if H1V6_JPEG_DBG_SHOW
				enc_size += frame_size;
#endif
				ptr_encoder = (struct encoder_data *)malloc(sizeof(struct encoder_data));
				if(ptr_encoder == NULL){
					printf("ptr_encoder null\r\n");
					while(1);
				}
				ptr_encoder->type = FMT_V_MJPG;
				ptr_encoder->timestamp = timestamp;
				ptr_encoder->hw_timestamp = hw_timestamp;
				ptr_encoder->addr = ptr;
				ptr_encoder->size = frame_size;
				rtw_mutex_get(&jpeg_ctx->encoder_jpeg_lh.list_lock);
				rtw_list_insert_tail(&ptr_encoder->list_data,&jpeg_ctx->encoder_jpeg_lh.list);
				rtw_mutex_put(&jpeg_ctx->encoder_jpeg_lh.list_lock);
#if H1V6_JPEG_DBG_SHOW
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

void* h1v6_jpeg_open(void)
{
	struct hantro_jpeg_context *jpeg_ctx = (struct hantro_jpeg_context *)hantro_jpegenc_open();
	if (jpeg_ctx == NULL) {
		return NULL;
	}
	memset(&jpeg_ctx->encoder_jpeg_lh,0,sizeof(struct encoder_list_head));
	rtw_init_listhead(&jpeg_ctx->encoder_jpeg_lh.list);
	
	rtw_mutex_init(&jpeg_ctx->encoder_jpeg_lh.list_lock); 
	
	//hal_video_sys_init_rtl8195bhp();
	//hal_enc_hw_init();
	
	return jpeg_ctx;
}

void h1v6_jpeg_close(void* ctx)
{
	hantro_jpegenc_release(ctx);
}

int h1v6_jpeg_set_param(void* ctx, int cmd, int arg)
{
	struct hantro_jpeg_context *jpeg_ctx = (struct hantro_jpeg_context *)ctx;
	int ret = 0;
	int count = 0;
	
	switch(cmd){
		case CMD_ISP_STREAMID:
			jpeg_ctx->isp_info_value.streamid = arg;
			break;
		case CMD_ISP_HW_SLOT:
			jpeg_ctx->isp_info_value.hw_slot = arg;
			break;
		case CMD_ISP_SW_SLOT:
			jpeg_ctx->isp_info_value.sw_slot = arg;
			break;
		case CMD_HANTRO_JPEG_SET_HEIGHT:
			jpeg_ctx->jpeg_parm.height = arg;
			break;
		case CMD_HANTRO_JPEG_SET_WIDTH:
			jpeg_ctx->jpeg_parm.width = arg;
			break;
		case CMD_HANTRO_JPEG_LEVEL:
			jpeg_ctx->jpeg_parm.qLevel = arg;
			break;
		case CMD_HANTRO_JPEG_FPS:
			jpeg_ctx->jpeg_parm.ratenum = arg;
			break;
		case CMD_H1V6_MEMORY_SIZE:
			jpeg_ctx->mem_info_value.mem_total_size = arg;
			break;
		case CMD_H1V6_BLOCK_SIZE:
			jpeg_ctx->mem_info_value.mem_block_size = arg;
			break;
		case CMD_H1V6_MAX_FRAME_SIZE:
			jpeg_ctx->mem_info_value.mem_frame_size = arg;
			break;
		case CMD_HANTRO_JPEG_HEADER_TYPE:
			jpeg_ctx->jpeg_parm.jpeg_full_header = arg;
			break;
		case CMD_HANTRO_JPEG_APPLY:
			jpeg_ctx->encoder_jpeg_lh.memory_ctx = memory_init(jpeg_ctx->mem_info_value.mem_total_size,jpeg_ctx->mem_info_value.mem_block_size);//(4*1024*1024,512); 
			if(jpeg_ctx->encoder_jpeg_lh.memory_ctx == NULL){
				printf("Can't allocate JPEG buffer\r\n");
				while(1);
			}
			isp_config(jpeg_ctx->isp_info_value.streamid,jpeg_ctx->isp_info_value.hw_slot,jpeg_ctx->isp_info_value.sw_slot,jpeg_ctx->jpeg_parm.ratenum,jpeg_ctx->jpeg_parm.width ,jpeg_ctx->jpeg_parm.height,ISP_FORMAT_YUV420_SEMIPLANAR);
			if ((ret = hantro_jpegenc_initial(jpeg_ctx,&jpeg_ctx->jpeg_parm))< 0) {
				printf("hantro_jpegenc_initial fail\n\r");
				while(1);
			}
			if(xTaskCreate(jpeg_handler, ((const char*)"jpeg_handler"), 1024, (void *)jpeg_ctx, tskIDLE_PRIORITY + 2, &jpeg_thread_id) != pdPASS)
				printf("\n\r%s xTaskCreate failed", __FUNCTION__);
			vTaskDelay(1000);
			isp_start(jpeg_ctx->isp_info_value.streamid);
			printf("start frame\r\n");
			break;
		case CMD_SNAPSHOT_CB:
			jpeg_ctx->snapshot_cb = (void (*)(uint32_t,uint32_t))arg;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return 0;
}

int h1v6_jpeg_handle(void* ctx, void* b)
{
	struct encoder_data *ptr_encoder = NULL;
	struct hantro_jpeg_context *jpeg_ctx = (struct hantro_jpeg_context *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	if(exbuf->state==STAT_USED){
		exbuf->state = STAT_INIT;
		memory_free(jpeg_ctx->encoder_jpeg_lh.memory_ctx,exbuf->data);
	}else{
		printf("exbuf->state = %d\r\n",exbuf->state);
	}
	
	if(exbuf->state!=STAT_READY){
		while(1){
			if(rtw_is_list_empty(&jpeg_ctx->encoder_jpeg_lh.list)){
				vTaskDelay(1);
			}else{
				rtw_mutex_get(&jpeg_ctx->encoder_jpeg_lh.list_lock);
				ptr_encoder = list_first_entry(&jpeg_ctx->encoder_jpeg_lh.list, struct encoder_data, list_data);
				rtw_list_delete(&ptr_encoder->list_data);
				rtw_mutex_put(&jpeg_ctx->encoder_jpeg_lh.list_lock);
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

msrc_module_t h1v6_jpeg_module =
{
	.create = h1v6_jpeg_open,
	.destroy = h1v6_jpeg_close,
	.set_param = h1v6_jpeg_set_param,
	.handle = h1v6_jpeg_handle,
};