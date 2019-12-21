#include "mmf_source.h"
#include "mmf_source_aac_enc.h"
#include "timer_api.h"
#include "audio_api.h"
#include <platform_opts.h>

#define AAC_ENC_DBG_SHOW 1
TaskHandle_t aac_thread_id = NULL;

#define FRAME_PERIOD_US 128000  // 128 ms // 8KHz, 1024 a frame
gtimer_t aac_frame_timer;

#define AAC_ENC_SIZE 2048 // 1024 samples, 16bit

#define AUDIO_LOOPBACK 0
#define SRC_FILE 0
#define SRC_MIC 1

#define TX_PAGE_SIZE 2048 //64*N bytes, max: 4095. 128, 4032  
#define RX_PAGE_SIZE 2048 //64*N bytes, max: 4095. 128, 4032

u8 dma_txdata[TX_PAGE_SIZE*2]__attribute__ ((aligned (0x20))); 
u8 dma_rxdata[RX_PAGE_SIZE*2]__attribute__ ((aligned (0x20))); 

audio_t audio_obj;

#if SRC_FILE
static unsigned char *sample = (unsigned char*)aacenc_sample;
static unsigned char *next_sample = (unsigned char *)aacenc_sample;
#endif

struct aac_enc_buf_handle *aac_hdl = NULL;

void audio_in_buffer_setting(int bit_length, int samplesInput)
{
	unsigned char *ptr = NULL;
	int size = 0;
	if (bit_length == 8)
		size = samplesInput;
	else if (bit_length == 16)
		size = samplesInput*2;
	
	enc_list_initial(&aac_hdl->enc_list);
	
	for (int i=0; i<AAC_MAX_BUFFER_COUNT; i++) {
		ptr = (unsigned char*)malloc(size);
		if(ptr == NULL){
			printf("malloc audio buffer fail\r\n");
			while(1);
		}
		aac_hdl->enc_data[i].audio_addr = (u32)ptr;
		aac_hdl->enc_data[i].index = i;
		aac_hdl->enc_data[i].size = size;
		enc_set_idle_buffer(&aac_hdl->enc_list,&aac_hdl->enc_data[i]);
		printf("[Audio_in] buffer index = %d, size=%d, addr = %x\r\n",i,size,ptr);
	}
}

#if SRC_FILE
static void aac_frame_timer_handler(uint32_t id)
{
	u32 timestamp = xTaskGetTickCountFromISR();
	unsigned char * source=(unsigned char*)aacenc_sample;
	struct enc_buf_data *enc_data = NULL;

#if AAC_ENC_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int drop_frame = 0;
	static unsigned int aac_frame = 0;
	static unsigned int drop_frame_total = 0;
	static unsigned int aac_frame_total = 0;
	
	if(timer_2 == 0){
		timer_1 = xTaskGetTickCountFromISR();
		timer_2 = timer_1;
	}else{
		timer_2 = xTaskGetTickCountFromISR();
		if((timer_2 - timer_1)>=1000){
			printf("[aac_frame_timer_handler] drop:%d aac:%d drop_total:%d aac_total:%d\r\n",drop_frame,aac_frame,drop_frame_total,aac_frame_total);
			timer_2 = 0;
			drop_frame = 0;
			aac_frame = 0;
		}
	}
#endif
	
	enc_data = enc_get_idle_buffer(&aac_hdl->enc_list);
	if(enc_data == NULL){
		// drop current frame
		sample += AAC_ENC_SIZE;
		next_sample += AAC_ENC_SIZE;
#if AAC_ENC_DBG_SHOW
		drop_frame++;
		drop_frame_total++;
#endif
		return; // NOT SEND
	}else{
		sample=next_sample;
		if((sample-source) >= aacenc_sample_len){//replay
			sample = source; 
			next_sample = source + AAC_ENC_SIZE;
		}else{
			next_sample += AAC_ENC_SIZE;
			if(next_sample - source >= aacenc_sample_len){ //replay
				sample = source;
				next_sample = source + AAC_ENC_SIZE;
			}
		}
		memcpy((void*)enc_data->audio_addr,(void*)sample ,next_sample-sample);
		enc_data->size = next_sample - sample;
		enc_data->timestamp = timestamp;
		enc_set_acti_buffer(&aac_hdl->enc_list,enc_data);
		rtw_up_sema_from_isr(&aac_hdl->enc_list.enc_sema);
#if AAC_ENC_DBG_SHOW
		aac_frame++;
		aac_frame_total++;
#endif
	}
}
#endif

void audio_tx_irq_fun(u32 arg, u8 *pbuf)
{
	
}

void audio_rx_irq_fun(u32 arg, u8 *pbuf)
{
	u32 audio_rx_ts = xTaskGetTickCountFromISR();
	
	audio_t *obj = (audio_t *)arg;
	struct enc_buf_data *enc_data = NULL;
	
#if AAC_ENC_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int drop_frame = 0;
	static unsigned int aac_frame = 0;
	static unsigned int drop_frame_total = 0;
	static unsigned int aac_frame_total = 0;
	
	if(timer_2 == 0){
		timer_1 = xTaskGetTickCountFromISR();
		timer_2 = timer_1;
	}else{
		timer_2 = xTaskGetTickCountFromISR();
		if((timer_2 - timer_1)>=1000){
			//printf("[audio_rx_irq_fun] drop:%d aac:%d drop_total:%d aac_total:%d\r\n",drop_frame,aac_frame,drop_frame_total,aac_frame_total);
			timer_2 = 0;
			drop_frame = 0;
			aac_frame = 0;
		}
	}
#endif
	
	enc_data = enc_get_idle_buffer(&aac_hdl->enc_list);
	if(enc_data == NULL){
		// drop current frame
#if AAC_ENC_DBG_SHOW
		drop_frame++;
		drop_frame_total++;
#endif
		goto exit; // NOT SEND
	}
	memcpy((void*)enc_data->audio_addr,(void*)pbuf,AAC_ENC_SIZE);
	enc_data->size = AAC_ENC_SIZE;
	enc_data->timestamp = audio_rx_ts;
		
	enc_set_acti_buffer(&aac_hdl->enc_list,enc_data);
	rtw_up_sema_from_isr(&aac_hdl->enc_list.enc_sema);
#if AAC_ENC_DBG_SHOW
	aac_frame++;
	aac_frame_total++;
#endif
	
exit:
#if AUDIO_LOOPBACK
	u8 *ptx_addre;
	ptx_addre = audio_get_tx_page_adr(obj);
	memcpy((void*)ptx_addre, (void*)pbuf, TX_PAGE_SIZE);
	audio_set_tx_page(obj, ptx_addre);
#endif
	audio_set_rx_page(obj);
}

static void aac_handler(void *param)
{
	u8 *ptr;
	int ret = 0;
	int frame_size = 0;
	u32 timestamp = 0;
	
	struct encoder_data *ptr_encoder = NULL;
	struct aacen_context *aac_ctx = (struct aacen_context *)param;
	
	struct enc_buf_data *enc_data = NULL;
	int samplesRead;
	_irqL 	irqL;
#if AAC_ENC_DBG_SHOW
	static unsigned int timer_1 = 0;
	static unsigned int timer_2 = 0;
	static unsigned int enc_frame = 0;
	static unsigned int enc_frame_all = 0;
	static unsigned int enc_size = 0;
#endif
	printf("aac start\r\n");
	while(1){
		rtw_down_sema(&aac_hdl->enc_list.enc_sema);
		do {
			rtw_enter_critical(&aac_hdl->enc_list.enc_lock, &irqL);
			enc_data = enc_get_acti_buffer(&aac_hdl->enc_list);
			rtw_exit_critical(&aac_hdl->enc_list.enc_lock, &irqL);
			if(enc_data == NULL)
				break;
RETRY:
			ptr = memory_alloc(aac_ctx->encoder_lh.memory_ctx,aac_ctx->mem_info_value.mem_frame_size);
			if(ptr == NULL){
				vTaskDelay(10);
				//printf("AAC Not enough buffer\r\n");
				goto RETRY;
			}
			timestamp = enc_data->timestamp;
			aac_ctx->source_addr = (void*)enc_data->audio_addr;
			aac_ctx->outputBuffer = (unsigned char *)ptr;
			
			if (aac_ctx->bit_length==8)
				samplesRead = enc_data->size;
			else if (aac_ctx->bit_length==16)
				samplesRead = enc_data->size/2;
			

#if AAC_ENC_DBG_SHOW
			unsigned int t1 = rtw_get_current_time();
			frame_size = aac_encode_run(aac_ctx->hEncoder, aac_ctx->source_addr, samplesRead, aac_ctx->outputBuffer, aac_ctx->maxBytesOutput);
			unsigned int t2 = rtw_get_current_time();
			//ENCODER_DBG_INFO("enc_time= %d",t2-t1);
			enc_frame++;
			enc_frame_all++;
#else
			frame_size = aac_encode_run(aac_ctx->hEncoder, aac_ctx->source_addr, samplesRead, aac_ctx->outputBuffer, aac_ctx->maxBytesOutput);
#endif

			rtw_enter_critical(&aac_hdl->enc_list.enc_lock, &irqL);
			enc_set_idle_buffer(&aac_hdl->enc_list,enc_data);
			rtw_exit_critical(&aac_hdl->enc_list.enc_lock, &irqL);
			
			// TODO:  aac_encode_run error code handling
			/*
			if(ret < 0){
				printf("aac drop frame ret = %d\r\n",ret);
				memory_free(aac_ctx->encoder_lh.memory_ctx,ptr);
			}
			*/
			if (frame_size <= 0) {
				ENCODER_DBG_ERROR("aacenc frame_size = %d, drop",frame_size);
				memory_free(aac_ctx->encoder_lh.memory_ctx,ptr);
			}
			else{
				memory_realloc(aac_ctx->encoder_lh.memory_ctx,ptr,frame_size);
#if AAC_ENC_DBG_SHOW
				enc_size += frame_size;
#endif
				ptr_encoder = (struct encoder_data *)malloc(sizeof(struct encoder_data));
				if(ptr_encoder == NULL){
					printf("ptr_encoder null\r\n");
					while(1);
				}
				ptr_encoder->type = FMT_A_MP4A_LATM;
				ptr_encoder->timestamp = timestamp;
				ptr_encoder->addr = ptr;
				ptr_encoder->size = frame_size;
				rtw_mutex_get(&aac_ctx->encoder_lh.list_lock);
				rtw_list_insert_tail(&ptr_encoder->list_data,&aac_ctx->encoder_lh.list);
				rtw_mutex_put(&aac_ctx->encoder_lh.list_lock);
				rtw_up_sema(&aac_ctx->encoder_output_sema);
#if AAC_ENC_DBG_SHOW
				if (timer_2 == 0) {
					timer_1 = rtw_get_current_time();
					timer_2 = timer_1;
				}
				else{
					timer_2 = rtw_get_current_time();
					if ((timer_2 - timer_1)>=1000) {
						ENCODER_DBG_INFO("[AAC] encode_frame:%d size:%d frame_total:%d\r\n",enc_frame,enc_size,enc_frame_all);
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

void* aac_encode_mod_open(void)
{
	struct aacen_context *aac_ctx = (struct aacen_context *)malloc(sizeof(struct aacen_context));
	if (!aac_ctx)
		return NULL;

	aac_hdl = malloc(sizeof(struct aac_enc_buf_handle));
	if (!aac_hdl) {
		free(aac_ctx);
		return NULL;
	}
	
	memset(aac_ctx,0,sizeof(struct aacen_context));	
	rtw_init_listhead(&aac_ctx->encoder_lh.list);
	rtw_mutex_init(&aac_ctx->encoder_lh.list_lock);
	rtw_init_sema(&aac_ctx->encoder_output_sema,0);
	
	//set default value 8khz mon channel and bit length 8
	aac_ctx->sample_rate = 8000;
	aac_ctx->channel = 1;
	aac_ctx->bit_length = 8;
	
	memset(aac_hdl,0,sizeof(struct aac_enc_buf_handle));
	
	//Audio Init
	audio_init(&audio_obj, OUTPUT_SINGLE_EDNED, MIC_DIFFERENTIAL, AUDIO_CODEC_2p8V); //LINE_IN_MODE//MIC_DIFFERENTIAL, OUTPUT_SINGLE_EDNED, OUTPUT_CAPLESS, OUTPUT_DIFFERENTIAL
	audio_set_param(&audio_obj, ASR_8KHZ, WL_16BIT);  // ASR_8KHZ, ASR_16KHZ //ASR_48KHZ
	
	//Init RX dma
	audio_set_rx_dma_buffer(&audio_obj, dma_rxdata, RX_PAGE_SIZE);
	audio_rx_irq_handler(&audio_obj, audio_rx_irq_fun,(uint32_t)&audio_obj);
	
	//Init TX dma
	audio_set_tx_dma_buffer(&audio_obj, dma_txdata, TX_PAGE_SIZE);
	audio_tx_irq_handler(&audio_obj, audio_tx_irq_fun,(uint32_t)&audio_obj);
	
	audio_mic_analog_gain(&audio_obj, 1, MIC_40DB); // default 0DB
	return (void*)aac_ctx;
}

void aac_encode_mod_close(void* ctx)
{
	struct aacen_context *aac_ctx = (struct aacen_context *)ctx;
	if (aac_hdl) {
		for (int i=0; i<AAC_MAX_BUFFER_COUNT; i++) {
			free((void*)aac_hdl->enc_data[i].audio_addr);
		}
		free(aac_hdl);
	}
	
	if (aac_ctx) {
		rtw_free_sema(&aac_ctx->encoder_output_sema);
		free(aac_ctx);
	}
	return;	
	free(aac_ctx);
	return;
}

int aac_encode_mod_set_param(void* ctx, int cmd, int arg)
{
	struct aacen_context *aac_ctx = (struct aacen_context *)ctx;
	switch(cmd){
		case CMD_AACEN_SET_BIT_LENGTH:
			aac_ctx->bit_length = arg;
			break;
		case CMD_AACEN_SET_SAMPLE_RATE:
			aac_ctx->sample_rate = arg;
			break;
		case CMD_AACEN_MEMORY_SIZE:
			aac_ctx->mem_info_value.mem_total_size = arg;
			break;
		case CMD_AACEN_BLOCK_SIZE:
			aac_ctx->mem_info_value.mem_block_size = arg;
			break;
		case CMD_AACEN_MAX_FRAME_SIZE:
			aac_ctx->mem_info_value.mem_frame_size = arg;
			break;
		case CMD_AACEN_SET_APPLY:
			aac_ctx->encoder_lh.memory_ctx = memory_init(aac_ctx->mem_info_value.mem_total_size,aac_ctx->mem_info_value.mem_block_size);
			if(aac_ctx->encoder_lh.memory_ctx == NULL){
				printf("Can't allocate AAC buffer\r\n");
				while(1);
			}
			aac_encode_init(&aac_ctx->hEncoder, aac_ctx->bit_length, aac_ctx->sample_rate, aac_ctx->channel, &aac_ctx->samplesInput, &aac_ctx->maxBytesOutput);
			printf("[AAC INIT] bit_length=%d, sample_rate=%d, channel=%d, samplesInput=%d, maxBytesOutput=%d\n\r",aac_ctx->bit_length,aac_ctx->sample_rate,aac_ctx->channel,aac_ctx->samplesInput,aac_ctx->maxBytesOutput);
			audio_in_buffer_setting(aac_ctx->bit_length, aac_ctx->samplesInput);
			if(xTaskCreate(aac_handler, ((const char*)"aac_handler"), 1024, (void *)aac_ctx, tskIDLE_PRIORITY + 2, &aac_thread_id) != pdPASS)
				printf("\n\r%s xTaskCreate failed", __FUNCTION__);
#if SRC_FILE
			printf("AAC_SRC_FILE\n\r");
			gtimer_init(&aac_frame_timer, TIMER0);
			gtimer_start_periodical(&aac_frame_timer, FRAME_PERIOD_US, (void*)aac_frame_timer_handler, NULL);
#elif SRC_MIC
			printf("AAC_SRC_MIC\n\r");
			//Audio Start
			audio_trx_start(&audio_obj);
#endif
			break;
		default:
			break;
	}
	return 0;
}

int aac_encode_mod_handle(void* ctx, void* b)
{
	exch_buf_t *exbuf = (exch_buf_t*)b;
	struct encoder_data *ptr_encoder = NULL;
	struct aacen_context *aac_ctx = (struct aacen_context *)ctx;
	
	if(exbuf->state==STAT_USED){
		exbuf->state = STAT_INIT;
		memory_free(aac_ctx->encoder_lh.memory_ctx,exbuf->data);
	}else{
		printf("exbuf->state = %d\r\n",exbuf->state);
	}
	
	if(exbuf->state!=STAT_READY) {
		while(1){
			rtw_down_sema(&aac_ctx->encoder_output_sema);
			if(!rtw_is_list_empty(&aac_ctx->encoder_lh.list)) {
				rtw_mutex_get(&aac_ctx->encoder_lh.list_lock);
				ptr_encoder = list_first_entry(&aac_ctx->encoder_lh.list, struct encoder_data, list_data);
				rtw_list_delete(&ptr_encoder->list_data);
				rtw_mutex_put(&aac_ctx->encoder_lh.list_lock);
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

msrc_module_t aac_enc_module =
{
	.create = aac_encode_mod_open,
	.destroy = aac_encode_mod_close,
	.set_param = aac_encode_mod_set_param,
	.handle = aac_encode_mod_handle,
};
