#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "i2s_api.h"
#include "section_config.h"
#include "wifi_conf.h"
#include "FDK_audio.h"
#include "aacdecoder_lib.h"
#include "libavcodec/version.h"
#include "libavformat/avformat.h"
#include"platform/platform_stdlib.h"

#define ____GLOBAL_PARAM________________________________
#define CONFIG_EXAMPLE_M4A_STREAM_SGTL5000	0
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5651	0
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5680	0

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
#include "sgtl5000.h"
#endif
#if CONFIG_EXAMPLE_M4A_STREAM_ALC5651
#include "alc5651.h"
#endif
#if CONFIG_EXAMPLE_M4A_STREAM_ALC5680
#include "alc5680.h"
#endif

// 仅支持LC-AAC直播
#define HLS_I2S_DMA_PAGE_SIZE		4096//8192   //Use frequency mapping table and set this value to number of decoded bytes 

#define I2S_SCLK_PIN	PC_1
#define I2S_WS_PIN		PC_0
#define I2S_SD_PIN		PC_2
static i2s_t i2s_obj;
#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
static float vol_ratio = 0.5; // codec volume ration. 0.0 ~1.0
#endif

#define BITS_PER_CHANNEL		WL_16b
#define I2S_DMA_PAGE_NUM		2   // Vaild number is 2~4
SDRAM_DATA_SECTION uint16_t	decode_buffer[HLS_I2S_DMA_PAGE_SIZE];
SDRAM_DATA_SECTION static u8	i2s_tx_buffer[HLS_I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
SDRAM_DATA_SECTION static u8	i2s_rx_buffer[HLS_I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

//#define URL_HLS "http://106.75.145.173/m3u/test/index.m3u8"				// 猫王电台,已停播...
#define URL_HLS "http://rthkaudio2-lh.akamaihd.net/i/radio2_1@355865/index_56_a-p.m3u8"	// 香港电台2

#define ____SUB_FUN________________________________
extern void fATW0(void *arg);
extern void fATW1(void *arg);
extern void fATWC(void *arg);

// pkts pool for http dwn_tsk to input and aac dec_tsk to sink
#define HLS_PKTS_POOL_SIZE	5
typedef struct{
	uint8_t pkt_num;
	uint8_t pkt_wt;	
	uint8_t pkt_rd;		
	AVStream *st;
	AVPacket pkts[HLS_PKTS_POOL_SIZE];
}avPackets_t;
static avPackets_t avPackets = {0};
static xTaskHandle hls_dwn_tsk_hdl = NULL;
static xTaskHandle hls_dec_tsk_hdl = NULL;
static xSemaphoreHandle avpkts_pkt_num_mutex = NULL;
static xSemaphoreHandle avpkts_ready_dec_sema = NULL;
static xSemaphoreHandle avpkts_ready_download_sema = NULL;

static void hls_my_printf(void* avc, int level, const char* fmt, va_list list)
{
	if(level <= av_log_get_level())
	{
		rtl_vprintf(fmt, list);
	}
}

static void hls_i2s_tx_complete(void *data, char *pbuf)
{   
	//
}

static void hls_i2s_rx_complete(void *data, char* pbuf)
{
	//
}

void hls_initialize_wifi(void)
{
#if 0
	do{
		fATW0((void *)"ADSL_WLAN");
		fATW1((void *)"wlan123987");
		fATWC(NULL);
	}while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS);
#else
	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS)
	{
		vTaskDelay(1000);
	}
#endif
	av_log(NULL, AV_LOG_INFO, "\r\n[%s] wifi ok\n", __func__);
}

static void hls_initialize_audio(u8 ch_num, int sample_rate, int dma_page_sz)
{
	u8 smpl_rate_idx = SR_16KHZ;
	u8 ch_num_idx = I2S_CH_STEREO;

	switch(ch_num){
	case 1:
		ch_num_idx = I2S_CH_MONO;
		break;
	case 2:
		ch_num_idx = I2S_CH_STEREO;
		break;
	default:
		break;
	}
	
	switch(sample_rate){
	case 8000:
		smpl_rate_idx = SR_8KHZ;
		break;	
	case 11025:
		smpl_rate_idx = SR_16KHZ;
		break;
	case 12000:
		smpl_rate_idx = SR_16KHZ;
		break;	
	case 16000:
		smpl_rate_idx = SR_16KHZ;
		break;
	case 22050:
		smpl_rate_idx = SR_22p05KHZ;
		break;
	case 24000:
		smpl_rate_idx = SR_24KHZ;
		break;		
	case 32000:
		smpl_rate_idx = SR_32KHZ;
		break;
	case 44100:
		smpl_rate_idx = SR_44p1KHZ;
		break;
	case 48000:
		smpl_rate_idx = SR_48KHZ;
		break;
	default:
		break;
	}

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5651
	alc5651_init();
	alc5651_init_interface2();
#endif

	i2s_obj.channel_num = ch_num_idx;
	i2s_obj.sampling_rate = smpl_rate_idx;
	i2s_obj.word_length = BITS_PER_CHANNEL;
	i2s_obj.direction = I2S_DIR_TXRX;	 
	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buffer, (char*)i2s_rx_buffer, \
		I2S_DMA_PAGE_NUM, dma_page_sz);
	i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)hls_i2s_tx_complete, (uint32_t)&i2s_obj);
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)hls_i2s_rx_complete, (uint32_t)&i2s_obj);

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
	sgtl5000_enable();
	sgtl5000_setVolume(volume_ratio);
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5680
	alc5680_i2c_init();
	set_alc5680_volume(0x74, 0x74);
#endif
}

static void hls_audio_play_pcm(char *buf, int len, int dma_page_sz)
{
	int *ptx_buf;

retry:
	ptx_buf = i2s_get_tx_page(&i2s_obj);
	if(ptx_buf){
		if(len < dma_page_sz){
			/* if valid audio data short than a page, make sure the rest of the page set to 0*/
			memset((void*)ptx_buf, 0x00, dma_page_sz);
			memcpy((void*)ptx_buf, (void*)buf, len);
		}else {
			memcpy((void*)ptx_buf, (void*)buf, len);
		}
		i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
	}else{
		vTaskDelay(1);
		goto retry;
	}
}

void hls_dump_aac_stream_info(CStreamInfo *info)
{
	printf("\n\t %d", info->sampleRate); //The samplerate in Hz of the fully decoded PCM audio signal
	printf("\n\t %d", info->frameSize); // The frame size of the decoded PCM audio signal. 1024 or 960 for AAC-LC;   2048 or 1920 for HE-AAC (v2);  512 or 480 for AAC-LD and AAC-ELD
	printf("\n\t %d", info->numChannels); 
}

int hls_init() {
	// log level set
	av_log_set_level(AV_LOG_INFO);
	av_log_set_callback(hls_my_printf);
	
	avpkts_pkt_num_mutex = xSemaphoreCreateMutex();
	avpkts_ready_dec_sema = xSemaphoreCreateCounting(HLS_PKTS_POOL_SIZE, 0);
	vSemaphoreCreateBinary(avpkts_ready_download_sema);

	if(NULL == avpkts_pkt_num_mutex || NULL == avpkts_ready_dec_sema || NULL == avpkts_ready_download_sema) {
		av_log(NULL, AV_LOG_ERROR, "\r\n[%s] Create Semaphore fail !", __func__);
		return -1;
	}
	return 0;
}

int hls_deinit() {
	if(hls_dwn_tsk_hdl)	vTaskDelete(hls_dwn_tsk_hdl);
	if(hls_dec_tsk_hdl)	vTaskDelete(hls_dec_tsk_hdl);
	vSemaphoreDelete(avpkts_pkt_num_mutex);
	vSemaphoreDelete(avpkts_ready_dec_sema);
	vSemaphoreDelete(avpkts_ready_download_sema);
	return 0;
}

#define ____MAIN_FUN________________________________
void audio_hls_dwn_tsk(void* param) {
	int32_t ret, i;
	uint32_t start_time;
	uint32_t start_heap;	
	AVDictionary *opts = NULL;
	AVFormatContext *in = NULL;
	avPackets_t *avpkts = (avPackets_t *)param;

	// wait for wifi ok
	hls_initialize_wifi();
	// statistics
	start_time = rtw_get_current_time();
	start_heap = xPortGetFreeHeapSize();

	av_log(NULL, AV_LOG_INFO, "\r\n\nHLS download begin......\n");
	
	av_register_all();
	avformat_network_init();
	av_dict_set(&opts, "protocol_whitelist", "hls,http,tcp", 0);
	ret = avformat_open_input(&in, URL_HLS, NULL, &opts);
	if (ret < 0) {
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] avformat_open_input (url: %s) fail ! ret=0x%x\n", __func__, URL_HLS, ret);
		goto exit;
	}else {
		av_log(NULL, AV_LOG_INFO, "\r\n[%s] avformat_open_input (url: %s) OK !\n", __func__, URL_HLS);
	}

	for (i = 0; i < in->nb_streams && !avpkts->st; i++) {
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC)
			avpkts->st = in->streams[i];
	}
	if (!avpkts->st) {
		av_log(NULL, AV_LOG_FATAL, "\r\n[%s] no AAC stream found !", __func__);
		goto exit;
	}

	while(1) {		
		if(avpkts->pkt_num < HLS_PKTS_POOL_SIZE) {
			ret = av_read_frame(in, &avpkts->pkts[avpkts->pkt_wt]);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "\r\n[%s] read frame fail ! ret=%d ", __func__, ret);			
				if (ret == AVERROR(EAGAIN))
					continue;
				vTaskDelay(100);
				break;
			}
			if (avpkts->pkts[avpkts->pkt_wt].stream_index != avpkts->st->index) {			
				av_free_packet(&avpkts->pkts[avpkts->pkt_wt]);
				continue;
			}

			avpkts->pkt_wt = (++avpkts->pkt_wt) % HLS_PKTS_POOL_SIZE;
			xSemaphoreTake(avpkts_pkt_num_mutex, portMAX_DELAY);
			++avpkts->pkt_num;
			xSemaphoreGive(avpkts_pkt_num_mutex);
			xSemaphoreGive(avpkts_ready_dec_sema);	
		}else
			while(xSemaphoreTake(avpkts_ready_download_sema, portMAX_DELAY) != pdTRUE);
	}
	
exit:
	av_log(NULL, AV_LOG_FATAL, "[%s] Exit !!!(Heap used: 0x%x, Time passed: %dms)\n", __func__, start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	avformat_close_input(&in);
	hls_deinit();
}

void audio_hls_dec_tsk(void* param) {
	UINT valid;
	int32_t pcm_size = 0;
	int32_t dma_page_sz = 0;
	int32_t first_frame = 1;
	CStreamInfo *info = NULL;
	AAC_DECODER_ERROR err;
	HANDLE_AACDECODER handle;
	avPackets_t *avpkts = (avPackets_t *)param;

	av_log(NULL, AV_LOG_INFO, "\r\nHLS decode begin......\n", __func__);
	handle = aacDecoder_Open(TT_MP4_ADTS, 1);
	err = aacDecoder_ConfigRaw(handle, &avpkts->st->codec->extradata, &avpkts->st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		av_log(NULL, AV_LOG_FATAL, "\r\n[%s] unable to decode the ASC (err=%d) !", __func__, err);
		goto exit;
	}

	while(1) {
		while(xSemaphoreTake(avpkts_ready_dec_sema, portMAX_DELAY) != pdTRUE);
		valid = avpkts->pkts[avpkts->pkt_rd].size;
refill:
		err = aacDecoder_Fill(handle, &avpkts->pkts[avpkts->pkt_rd].data, &avpkts->pkts[avpkts->pkt_rd].size, &valid);
		if (err != AAC_DEC_OK) {
			av_log(NULL, AV_LOG_FATAL, "\r\n[%s] aacDecoder_Fill fail ! err=0x%x", __func__, err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buffer, HLS_I2S_DMA_PAGE_SIZE, 0);
		if (err == AAC_DEC_NOT_ENOUGH_BITS) {
			if(valid > 0)
				goto refill;
			else
				goto free;
		}
		if (err != AAC_DEC_OK) {	
			av_log(NULL, AV_LOG_ERROR, "\r\n[%s] aacDecoder_DecodeFrame fail ! err=0x%x", __func__, err);
			goto free;			
		}

		if(!info){
			info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				av_log(NULL, AV_LOG_FATAL, "\r\n[%s] aacDecoder_GetStreamInfo fail !", __func__);
				break;
			}
			pcm_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16b)?2:3);
			dma_page_sz = pcm_size;
		//	hls_dump_aac_stream_info(info);
		}
		if(first_frame){
			hls_initialize_audio(info->numChannels, info->sampleRate, dma_page_sz);
			first_frame = 0;
		}

		hls_audio_play_pcm(decode_buffer, pcm_size, dma_page_sz);
		// subsequent valid bytes of this pkt should be then refill and decode 
		if(valid > 0)
			goto refill;	
free:		
		av_free_packet(&avpkts->pkts[avpkts->pkt_rd]);
		avpkts->pkt_rd = (++avpkts->pkt_rd) % HLS_PKTS_POOL_SIZE;
		xSemaphoreTake(avpkts_pkt_num_mutex, portMAX_DELAY);
		--avpkts->pkt_num;
		xSemaphoreGive(avpkts_pkt_num_mutex);
		xSemaphoreGive(avpkts_ready_download_sema);		
	}

exit:
	av_log(NULL, AV_LOG_FATAL, "\r\n[%s] Exit !!!", __func__);
	aacDecoder_Close(handle);
	hls_deinit();
}

void example_audio_hls(void)
{	
	if(hls_init() < 0)
		goto err;
	
	if(xTaskCreate(audio_hls_dwn_tsk, ((const char*)"hls_dwn"), 7000, (void *)&avPackets, tskIDLE_PRIORITY + 3, &hls_dwn_tsk_hdl) != pdPASS) {
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] Create dwn_tsk failed !", __func__);		
		goto err;
	}
	if(xTaskCreate(audio_hls_dec_tsk, ((const char*)"hls_dec"), 1500, (void *)&avPackets, tskIDLE_PRIORITY + 3, &hls_dec_tsk_hdl) != pdPASS) {
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] Create dec_tsk failed !", __func__);
		goto err;
	}
	return;
err:
	hls_deinit();
}
