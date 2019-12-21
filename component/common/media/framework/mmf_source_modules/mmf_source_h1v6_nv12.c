#include "mmf_source.h"
#include "mmf_source_h1v6_nv12.h"

extern struct stream_manage_handle *isp_stm;

#include "hantro_modify_parm.h"

extern int uvc_format;

#define FORMAT_TYPE_YUY2        0
#define FORMAT_TYPE_NV12        1
#define FORMAT_TYPE_MJPEG       2
#define FORMAT_TYPE_H264        3

struct uvc_format *uvc_format_ptr = NULL;

void *hantro_nv12_open()
{
        struct h1v6_nv12_context *encoder = NULL;
        encoder = (struct h1v6_nv12_context *)malloc(sizeof(struct h1v6_nv12_context));
        memset(encoder, 0, sizeof(struct h1v6_nv12_context));
        uvc_format_ptr = (struct uvc_format *)malloc(sizeof(struct uvc_format));
        memset(uvc_format_ptr, 0, sizeof(struct uvc_format));
	if (encoder){
		return encoder;
        }else{
                return NULL;
        }
}


void* h1v6_nv12_open(void)
{
	struct h1v6_nv12_context *nv12_ctx = hantro_nv12_open();
	int i = 0;
        int ret = 0;
	if (nv12_ctx == NULL) {
		return NULL;
	}
	
	return nv12_ctx;
}

void h1v6_nv12_close(void* ctx)
{
	//hantro_h264enc_release(ctx);
}

int h1v6_nv12_set_param(void* ctx, int cmd, int arg)
{
	struct h1v6_nv12_context *nv12_ctx = (struct h1v6_nv12_context *)ctx;
	int ret = 0;
	int count = 0;
	
	switch(cmd){
		case CMD_ISP_STREAMID:
			nv12_ctx->isp_info_value.streamid = arg;
			break;
		case CMD_ISP_HW_SLOT:
			nv12_ctx->isp_info_value.hw_slot = arg;
			break;
		case CMD_ISP_SW_SLOT:
			nv12_ctx->isp_info_value.sw_slot = arg;
			break;
                case CMD_ISP_FORMAT:
                        nv12_ctx->format = arg;
                        break;
		case CMD_ISP_HEIGHT:
			nv12_ctx->height = arg;
                        uvc_format_ptr->height = arg;
			break;		
		case CMD_ISP_WIDTH:
			nv12_ctx->width = arg;
                        uvc_format_ptr->width = arg;
			break;
		case CMD_ISP_FPS:
			nv12_ctx->fps = arg;
			break;
		case CMD_ISP_APPLY:
			isp_config(nv12_ctx->isp_info_value.streamid,nv12_ctx->isp_info_value.hw_slot,nv12_ctx->isp_info_value.sw_slot,nv12_ctx->fps,nv12_ctx->width ,nv12_ctx->height,nv12_ctx->format);
			vTaskDelay(1000);
			isp_start(nv12_ctx->isp_info_value.streamid);
                        //nv12_ctx->format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        //mmf_video_nv12_change_parm_cb(nv12_ctx);
			printf("start frame\r\n");
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return 0;
}

int h1v6_nv12_handle(void* ctx, void* b)
{
	struct encoder_data *ptr_encoder = NULL;
        struct enc_buf_data *enc_data = NULL;
	struct h1v6_nv12_context *nv12_ctx = (struct h1v6_nv12_context *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	_irqL 	irqL;
        static int format = 0;//Default is 1080p raw data
        //printf("h1v6_nv12_handle\r\n");
	if(exbuf->state==STAT_USED){
                exbuf->state = STAT_INIT;
                //printf("FREE DATA\r\n");
                rtw_enter_critical(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
                enc_set_idle_buffer(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list,exbuf->priv);
                rtw_exit_critical(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
	//memory_free(h264_ctx->encoder_lh.memory_ctx,exbuf->data);
	}else{
	
                printf("exbuf->state = %d\r\n",exbuf->state);
	}

        if((nv12_ctx->uvc_format != uvc_format_ptr->format) || (nv12_ctx->width != uvc_format_ptr->width) || (nv12_ctx->height != uvc_format_ptr->height)){
                if(uvc_format_ptr->format == FORMAT_TYPE_YUY2){
                  nv12_ctx->format = ISP_FORMAT_BAYER_PATTERN;
                  nv12_ctx->fps = 5;
                }else if(uvc_format_ptr->format == FORMAT_TYPE_NV12){
                  nv12_ctx->format = ISP_FORMAT_YUV420_SEMIPLANAR;
                  nv12_ctx->fps = 15;
                }
                //format = uvc_format_ptr->format;
                nv12_ctx->uvc_format = uvc_format_ptr->format;
                nv12_ctx->width = uvc_format_ptr->width;
                nv12_ctx->height = uvc_format_ptr->height;
                mmf_video_nv12_change_parm_cb(nv12_ctx);
        }
        
	if(exbuf->state!=STAT_READY){
		while(1){
			if(rtw_is_list_empty(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list.enc_acti_list)){
				vTaskDelay(1);
			}else{
                                //printf("GET THE DATA\r\n");
				rtw_enter_critical(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
				enc_data = enc_get_acti_buffer(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list);
                                enc_data->uv_addr = enc_data->y_addr+nv12_ctx->width*nv12_ctx->height;
				exbuf->priv = enc_data;
				rtw_exit_critical(&isp_stm->isp_stream[nv12_ctx->isp_info_value.streamid]->enc_list.enc_lock,&irqL);
                                break;
			}
		}
         
                if(nv12_ctx->format == ISP_FORMAT_YUV420_SEMIPLANAR)
                  dcache_clean_by_addr_ual32((uint32_t*)enc_data->y_addr,nv12_ctx->height*nv12_ctx->width*1.5);
                else
                  dcache_clean_by_addr_ual32((uint32_t*)enc_data->y_addr,nv12_ctx->height*nv12_ctx->width*2);
		
		enc_data->timestamp;
		exbuf->codec_fmt = 0;//ptr_encoder->type;
                if(nv12_ctx->format == ISP_FORMAT_YUV420_SEMIPLANAR)
                  exbuf->len = nv12_ctx->height*nv12_ctx->width*1.5;//enc_data->size;//ptr_encoder->size;
                else
                  exbuf->len = nv12_ctx->height*nv12_ctx->width*2;
		exbuf->index = 0;
		exbuf->data = (uint8_t *)enc_data->y_addr;//ptr_encoder->addr;
		exbuf->timestamp = enc_data->timestamp;//ptr_encoder->timestamp;
		exbuf->state = STAT_READY;
                
		mmf_source_add_exbuf_sending_list_all(exbuf);

	}
	return 0;
}

msrc_module_t h1v6_nv12_module =
{
	.create = h1v6_nv12_open,
	.destroy = h1v6_nv12_close,
	.set_param = h1v6_nv12_set_param,
	.handle = h1v6_nv12_handle,
};