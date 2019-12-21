#include "mmf_source.h"
#include "mmf_source_h264_file.h"
#include <platform_opts.h>

TaskHandle_t h264f_thread_id = NULL;
gtimer_t h264f_frame_timer;
struct h264f_enc_buf_handle *h264f_hdl = NULL;

void h264f_buffer_setting(int size)
{
	unsigned char *ptr = NULL;
	enc_list_initial(&h264f_hdl->enc_list);
	for (int i=0; i<H264F_MAX_BUFFER_COUNT; i++) {
		ptr = (unsigned char*)malloc(size);
		h264f_hdl->enc_data[i].sample_addr= (u32)ptr;
		h264f_hdl->enc_data[i].index = i;
		h264f_hdl->enc_data[i].size = size;
		enc_set_idle_buffer(&h264f_hdl->enc_list,&h264f_hdl->enc_data[i]);
		printf("[h264f_buffer_setting] buffer index = %d, size=%d, addr = %x\r\n",i,size,ptr);
	}
}

static unsigned char *sample = (unsigned char*)h264_sample;
static unsigned char *next_sample = (unsigned char *)h264_sample;

static unsigned char* seek_to_next(unsigned char* ptr, unsigned char* ptr_end)
{
	if (ptr != h264_sample)
		ptr+=3;
	int skip_flag = 0;
	
	while( ptr < ptr_end ){
		if(ptr[0]==0 && ptr[1]==0){
			if(ptr[2]==0 && ptr[3]==1 ){
				if((ptr[4]&0x1f)!=0x07 && (ptr[4]&0x1f)!=0x08){	// not SPS or PPS
					if(skip_flag==0){
						//printf("r \n\r");
						return ptr;
					}else{
						skip_flag = 0;
					}
				}
				else if((ptr[4]&0x1f)==0x08){	// PPS, get next (one more) frame before return
					skip_flag=1;
				}
			}
		}
		ptr++;
	}

	return NULL;
}

static void h264f_frame_timer_handler(uint32_t id)
{
	u32 timestamp = xTaskGetTickCountFromISR();
	unsigned char * source=(unsigned char*)h264_sample;
	struct enc_buf_data *enc_data = NULL;

#if H264F_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int drop_frame = 0;
	static unsigned int send_frame = 0;
	static unsigned int drop_frame_total = 0;
	static unsigned int send_frame_total = 0;
	
	if(timer_2 == 0){
		timer_1 = xTaskGetTickCountFromISR();
		timer_2 = timer_1;
	}else{
		timer_2 = xTaskGetTickCountFromISR();
		if((timer_2 - timer_1)>=1000){
			printf("[h264f_frame_timer_handler] send:%d drop:%d drop_total:%d send_total:%d\r\n",send_frame,drop_frame,drop_frame_total,send_frame_total);
			timer_2 = 0;
			drop_frame = 0;
			send_frame = 0;
		}
	}
#endif

	sample = next_sample;
	next_sample = seek_to_next(sample, (unsigned char*)h264_sample+h264_sample_len);
	if(next_sample == NULL) { // replay
		next_sample = source;
	}
	
	enc_data = enc_get_idle_buffer(&h264f_hdl->enc_list);
	
	if(enc_data == NULL){ // drop current frame
#if H264F_DBG_SHOW
		drop_frame++;
		drop_frame_total++;
#endif
		return; // NOT SEND
	}
	else{
		if(next_sample == (unsigned char*)h264_sample) { // replay
			enc_data->size = h264_sample + h264_sample_len - sample;
		}
		else {
			enc_data->size = next_sample - sample;
		}
		memcpy((void*)enc_data->sample_addr,(void*)sample, enc_data->size);
		
		enc_data->timestamp = timestamp;
		enc_set_acti_buffer(&h264f_hdl->enc_list,enc_data);
		rtw_up_sema_from_isr(&h264f_hdl->enc_list.enc_sema);
#if H264F_DBG_SHOW
		send_frame++;
		send_frame_total++;
#endif
	}
}

static void h264f_handler(void *param)
{
	u8 *ptr;
	int frame_size = 0;
	u32 timestamp = 0;
	
	struct encoder_data *ptr_encoder = NULL;
	struct h264f_context *h264f_ctx = (struct h264f_context *)param;
	struct enc_buf_data *enc_data = NULL;
	_irqL 	irqL;
	
	printf("h264f_handler start\r\n");
	while(1){
		rtw_down_sema(&h264f_hdl->enc_list.enc_sema);
		do {
			rtw_enter_critical(&h264f_hdl->enc_list.enc_lock, &irqL);
			enc_data = enc_get_acti_buffer(&h264f_hdl->enc_list);
			rtw_exit_critical(&h264f_hdl->enc_list.enc_lock, &irqL);
			if(enc_data == NULL)
				break;
RETRY:
			ptr = memory_alloc(h264f_ctx->encoder_lh.memory_ctx,h264f_ctx->mem_info_value.mem_frame_size);
			if(ptr == NULL){
				vTaskDelay(10);
				//printf("Not enough buffer\r\n");
				goto RETRY;
			}
			timestamp = enc_data->timestamp;
			h264f_ctx->source_addr = (void*)enc_data->sample_addr;
			h264f_ctx->outputBuffer = (unsigned char *)ptr;
			frame_size = enc_data->size;
			// directly copy h264 raw data from file
			memcpy(h264f_ctx->outputBuffer,h264f_ctx->source_addr,frame_size);
			
			rtw_enter_critical(&h264f_hdl->enc_list.enc_lock, &irqL);
			enc_set_idle_buffer(&h264f_hdl->enc_list,enc_data);
			rtw_exit_critical(&h264f_hdl->enc_list.enc_lock, &irqL);
			
			if (frame_size <= 0) {
				printf("h264 frame_size = %d, drop\r\n",frame_size);
				memory_free(h264f_ctx->encoder_lh.memory_ctx,ptr);
			}
			else{
				memory_realloc(h264f_ctx->encoder_lh.memory_ctx,ptr,frame_size);
				ptr_encoder = (struct encoder_data *)malloc(sizeof(struct encoder_data));
				if(ptr_encoder == NULL){
					printf("ptr_encoder null\r\n");
					while(1);
				}
				ptr_encoder->type = FMT_V_H264;
				ptr_encoder->timestamp= timestamp;
				ptr_encoder->addr = ptr;
				ptr_encoder->size = frame_size;
				rtw_mutex_get(&h264f_ctx->encoder_lh.list_lock);
				rtw_list_insert_tail(&ptr_encoder->list_data,&h264f_ctx->encoder_lh.list);
				rtw_mutex_put(&h264f_ctx->encoder_lh.list_lock);
				rtw_up_sema(&h264f_ctx->encoder_output_sema);
			}
		}while(1);
	}
}

void* h264f_mod_open(void)
{
	struct h264f_context *h264f_ctx = (struct h264f_context *)malloc(sizeof(struct h264f_context));
	if (!h264f_ctx)
		return NULL;
	
	h264f_hdl = malloc(sizeof(struct h264f_enc_buf_handle));
	if (!h264f_hdl) {
		free(h264f_ctx);
		return NULL;
	}
	
	memset(h264f_ctx,0,sizeof(struct h264f_context));	
	rtw_init_listhead(&h264f_ctx->encoder_lh.list);
	rtw_mutex_init(&h264f_ctx->encoder_lh.list_lock);
	rtw_init_sema(&h264f_ctx->encoder_output_sema,0);
	
	memset(h264f_hdl,0,sizeof(struct h264f_enc_buf_handle));
	
	return (void*)h264f_ctx;
	
}

void h264f_mod_close(void* ctx)
{
	struct h264f_context *h264f_ctx = (struct h264f_context *)ctx;
	
	if (h264f_hdl) {
		for (int i=0; i<H264F_MAX_BUFFER_COUNT; i++) {	
			free((void*)h264f_hdl->enc_data[i].sample_addr);
		}
		free(h264f_hdl);
	}
	
	if (h264f_ctx) {
		rtw_free_sema(&h264f_ctx->encoder_output_sema);
		free(h264f_ctx);
	}
	return;
}

int h264f_mod_set_param(void* ctx, int cmd, int arg)
{
	struct h264f_context *h264f_ctx = (struct h264f_context *)ctx;
	switch(cmd){
		case CMD_H264F_SET_FPS:
			h264f_ctx->fps = arg;
			break;
		case CMD_H264F_SET_APPLY:
			h264f_ctx->encoder_lh.memory_ctx = memory_init(h264f_ctx->mem_info_value.mem_total_size,h264f_ctx->mem_info_value.mem_block_size);
			if(h264f_ctx->encoder_lh.memory_ctx == NULL){
				printf("Can't allocate H264F buffer\r\n");
				while(1);
			}
			h264f_buffer_setting(h264f_ctx->mem_info_value.mem_frame_size);
			if(xTaskCreate(h264f_handler, ((const char*)"h264f_handler"), 1024, (void *)h264f_ctx, tskIDLE_PRIORITY + 2, &h264f_thread_id) != pdPASS)
				printf("\n\r%s xTaskCreate failed", __FUNCTION__);
			gtimer_init(&h264f_frame_timer, TIMER0);
			gtimer_start_periodical(&h264f_frame_timer, 1000000/h264f_ctx->fps, (void*)h264f_frame_timer_handler, NULL);
			break;
		case CMD_H264F_MEMORY_SIZE:
			h264f_ctx->mem_info_value.mem_total_size = arg;
			break;
		case CMD_H264F_BLOCK_SIZE:
			h264f_ctx->mem_info_value.mem_block_size = arg;
			break;
		case CMD_H264F_MAX_FRAME_SIZE:
			h264f_ctx->mem_info_value.mem_frame_size = arg;
			break;
	}
	return 0;
}

int frame_idx = 0;

int h264f_mod_handle(void* ctx, void* b)
{
	struct encoder_data *ptr_encoder = NULL;
	struct h264f_context *h264f_ctx = (struct h264f_context *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	
	if(exbuf->state==STAT_USED){
		exbuf->state = STAT_INIT;
		memory_free(h264f_ctx->encoder_lh.memory_ctx,exbuf->data);
	}else{
		printf("exbuf->state = %d\r\n",exbuf->state);
	}
	
	if(exbuf->state!=STAT_READY){
		while(1){
			rtw_down_sema(&h264f_ctx->encoder_output_sema);
			if(!rtw_is_list_empty(&h264f_ctx->encoder_lh.list)) {
				rtw_mutex_get(&h264f_ctx->encoder_lh.list_lock);
				ptr_encoder = list_first_entry(&h264f_ctx->encoder_lh.list, struct encoder_data, list_data);
				rtw_list_delete(&ptr_encoder->list_data);
				rtw_mutex_put(&h264f_ctx->encoder_lh.list_lock);
				break;
			}
		}
		exbuf->codec_fmt = ptr_encoder->type;
		exbuf->len = ptr_encoder->size;
		exbuf->index = 0;
		exbuf->data = ptr_encoder->addr;
		exbuf->timestamp = ptr_encoder->timestamp;
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

msrc_module_t h264f_module =
{
	.create = h264f_mod_open,
	.destroy = h264f_mod_close,
	.set_param = h264f_mod_set_param,
	.handle = h264f_mod_handle,
};
