#include "mmf_source.h"
#include "mmf_source_uvc.h"

void* uvc_mod_open(void)
{
	/*init usb driver prior to init uvc driver*/
	_usb_init();
	if(wait_usb_ready() < 0)
		return NULL;
	
	/*init uvc driver*/
#if UVC_BUF_DYNAMIC
	if(uvc_stream_init(4,60000) < 0)
		return NULL;
#else	
	if(uvc_stream_init() < 0)
		return NULL;
#endif
	
	struct uvc_context *uvc_ctx = malloc(sizeof(struct uvc_context));
	if(!uvc_ctx)	return NULL;
	
	// default value
	uvc_ctx->fmt_type = UVC_FORMAT_MJPEG;
	uvc_ctx->width = 640;
	uvc_ctx->height = 480;
	uvc_ctx->frame_rate = 30;  
	uvc_ctx->compression_ratio = 0; 
	return uvc_ctx;
}

void uvc_mod_close(void* ctx)
{
	uvc_stream_free();
	free((struct uvc_context *)ctx);
	_usb_deinit();
}

int uvc_mod_set_param(void* ctx, int cmd, int arg)
{
	int ret = 0;
	int *parg = NULL;
	struct uvc_context* uvc_ctx = (struct uvc_context*)ctx;

	// format mapping
	//				 FMT_V_MJPG		   FMT_V_H264
	int codec_map[]={UVC_FORMAT_MJPEG, UVC_FORMAT_H264};
	
	switch(cmd){
		case CMD_SET_STREAMMING:
			if(arg == ON){	// stream on
				if(uvc_stream_on() < 0)
					ret = EIO;
			}else{			// stream off
				uvc_stream_off();
			}	
			break;		
		case CMD_UVC_SET_HEIGHT:
			uvc_ctx->height = arg;
			break;
		case CMD_UVC_SET_WIDTH:
			uvc_ctx->width = arg;
			break;
		case CMD_UVC_SET_FRAMERATE:
			uvc_ctx->frame_rate = arg;
			break;
		case CMD_UVC_SET_CPZRATIO:
			uvc_ctx->compression_ratio = arg;
			break;
		case CMD_UVC_SET_FRAMETYPE:
			uvc_ctx->fmt_type = codec_map[arg&0xF];
			break;
		case CMD_UVC_SET_APPLY:
			if(uvc_set_param(uvc_ctx->fmt_type, &uvc_ctx->width, &uvc_ctx->height, &uvc_ctx->frame_rate, &uvc_ctx->compression_ratio) < 0)
				ret = EIO;
			break;
		case CMD_UVC_GET_STREAM_READY:
			parg = (int *)arg;
			*parg = uvc_is_stream_ready();
			break;
		case CMD_UVC_GET_STREAM_STATUS:
			parg = (int *)arg;
			*parg = uvc_is_stream_on();
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

int cnt = 0;
//#include "sample_jpeg.h"
int uvc_mod_handle(void* ctx, void* b)
{
	int ret = 0;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	struct uvc_buf_context buf;
	
	if(exbuf->state==STAT_USED){
		memset(&buf, 0, sizeof(struct uvc_buf_context));
		buf.index = exbuf->index;
		ret = uvc_qbuf(&buf); 
		exbuf->state = STAT_INIT;
	}
	
	/*get uvc buffer for new data*/
	if(exbuf->state!=STAT_READY){
		ret = uvc_dqbuf(&buf); 
		if(buf.index < 0){
			return -EAGAIN;
		}//empty buffer retrieved set payload state as IDLE
		
		if((uvc_buf_check(&buf)<0)||(ret < 0)){
			printf("\n\rbuffer error!");
			uvc_stream_off();
			return -EIO;	// should return error
		}
		
		/*
		if(cnt++&0x1){
			exbuf->data = (unsigned char *)PIC_320x240_2;
			exbuf->len = PIC_LEN_2;
		}else{
			exbuf->data = (unsigned char *)PIC_320x240_1;
			exbuf->len = PIC_LEN_1;
		}
		*/
		
		exbuf->index = buf.index;
		exbuf->data = buf.data;
		exbuf->len = buf.len;
		exbuf->timestamp = 0;
		//vTaskDelay(500);
		exbuf->state = STAT_READY;
		
		mmf_source_add_exbuf_sending_list_all(exbuf);
	}
	return 0;
}

msrc_module_t uvc_module =
{
	.create = uvc_mod_open,
	.destroy = uvc_mod_close,
	.set_param = uvc_mod_set_param,
	.handle = uvc_mod_handle,
};
