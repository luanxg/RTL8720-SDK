#include "mmf_sink.h"
#include "uvc/inc/usbd_uvc_desc.h"

#define num 1
static struct usbd_uvc_buffer uvc_payload[num];

void uvcd_mod_close(void* ctx)
{
	return;
}

void* uvcd_mod_open(void)
{
	struct uvc_dev *uvc_ctx = get_private_usbd_uvc();
	int i = 0;
        int status = 0;
	_usb_init();
	status = wait_usb_ready();
	if(status != USB_INIT_OK){
		if(status == USB_NOT_ATTACHED)
			printf("\r\n NO USB device attached\n");
		else
			printf("\r\n USB init fail\n");
		goto exit;
	}
        if(!uvc_ctx)	return NULL;
	/*open rtsp service task*/
	if(usbd_uvc_init()<0){
		//rtsp_context_free(rtsp_ctx);
		uvc_ctx = NULL;
                printf("usbd uvc init error\r\n");
	}
        for(i=0;i<num;i++)
          uvc_video_put_out_stream_queue(&uvc_payload[i], uvc_ctx); 
	return (void*)uvc_ctx; 
exit:
	return NULL;
}

int uvcd_mod_set_param(void* ctx, int cmd, int arg)
{
	return 0;
}

#if 0
int uvcd_mod_handle(void* ctx, void* b)
{
	struct uvc_dev *uvc_ctx = (struct uvc_dev *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
        int i = 0;
	struct usbd_uvc_buffer *payload = NULL;//&rtp_payload;
        
        //printf("uvcd_mod_handle\r\n");
        
        if(exbuf->state != STAT_READY)
		return -EAGAIN;
        
	// wait output not empty and get one
	// Get payload from rtsp module
	do{
		payload = uvc_video_out_stream_queue(uvc_ctx);
		if(payload==NULL)
                  vTaskDelay(1);
	}while(payload == NULL);
	
        payload->exbuf=exbuf;
	payload->mem = exbuf->data;
	payload->bytesused = exbuf->len;
        //printf("payload->bytesused = %d\r\n", payload->bytesused);
	
	//rtp_object_in_stream_queue(payload, stream_ctx);
	uvc_video_put_in_stream_queue(payload, uvc_ctx);

        if(!list_empty(&uvc_ctx->video.output_queue)){
                exbuf->state = STAT_RESERVED;
                return 0;
        }
	while(list_empty(&uvc_ctx->video.output_queue)){
                //get_frame_index(&uvc_ctx->video);
                printf("EMPTY\r\n");
		vTaskDelay(1);
        }
        
        rtw_mutex_get(&uvc_ctx->video.output_lock);
        payload = list_first_entry(&uvc_ctx->video.output_queue, struct usbd_uvc_buffer, buffer_list);
        //printf("v=%d\r\n",payload->bytesused);
        exbuf=(exch_buf_t*)payload->exbuf;
        rtw_mutex_put(&uvc_ctx->video.output_lock);
        
        
        
end:
        exbuf->state = STAT_USED;
	return 0;
}
#endif

#if 1
int uvcd_mod_handle(void* ctx, void* b)
{
	//printf("[sink in] ");
	struct uvc_dev *uvc_ctx = (struct uvc_dev *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
        int i = 0;
		//printf("exbuf->state:%d\r\n", exbuf->state);
	struct usbd_uvc_buffer *payload = NULL;//&rtp_payload;
        
        //printf("uvcd_mod_handle\r\n");
        if(uvc_ctx->common->running == 0){
            goto end;
        }
        if(exbuf->state != STAT_READY)
		return -EAGAIN;
        
	// wait output not empty and get one
	// Get payload from rtsp module
	do{
		payload = uvc_video_out_stream_queue(uvc_ctx);
		if(payload==NULL){
                  printf("NULL\r\n");
                  vTaskDelay(1);
                }
	}while(payload == NULL);
	
        payload->exbuf=exbuf;
	payload->mem = exbuf->data;
	payload->bytesused = exbuf->len;
        //printf("payload->bytesused = %d\r\n", payload->bytesused);
	
	//rtp_object_in_stream_queue(payload, stream_ctx);
	uvc_video_put_in_stream_queue(payload, uvc_ctx);
#if 0
	while(list_empty(&uvc_ctx->video.output_queue)){
                //printf("EMPTY\r\n");
		vTaskDelay(1);
        }
#endif
        rtw_down_sema(&uvc_ctx->video.output_queue_sema);
        //rtw_mutex_get(&uvc_ctx->video.output_lock);
        //payload = list_first_entry(&uvc_ctx->video.output_queue, struct usbd_uvc_buffer, buffer_list);
        //printf("v=%d\r\n",payload->bytesused);
        //exbuf=(exch_buf_t*)payload->exbuf;
        //rtw_mutex_put(&uvc_ctx->video.output_lock);
        
  	//printf("[sink ret] ");      
        
end:
        exbuf->state = STAT_USED;
	return 0;
}
#endif

msink_module_t uvcd_module = 
{
	.create = uvcd_mod_open,
	.destroy = uvcd_mod_close,
	.set_param = uvcd_mod_set_param,
	.handle = uvcd_mod_handle,
};