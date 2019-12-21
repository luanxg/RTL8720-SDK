#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "wifi_conf.h"
#include "lwip/arch.h"
#include "i2s_api.h"
#include "analogin_api.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"
#include "libAACdec/include/aacdecoder_lib.h"
#include <libavformat/avformat.h>
#include <libavformat/url.h>

#define CONFIG_EXAMPLE_M4A_STREAM_SGTL5000      0
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5651       0
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5680       1

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
#include "sgtl5000.h"
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5651
#include "alc5651.h"
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5680
#include "alc5680.h"
#endif

//-----------------Frequency Mapping Table--------------------//
/*+-------------+-------------------------+--------------------+
| Frequency(hz) | Number of Channels      | Decoded Bytes      |
|(CH_MONO:1 CH_STEREO:2)  |(I2S_DMA_PAGE_SIZE) |
+---------------+-------------------------+--------------------+
|          8000 |                       1 |               1152 |
|          8000 |                       2 |               2304 |
|         16000 |                       1 |               1152 |
|         16000 |                       2 |               2304 |
|         22050 |                       1 |               1152 |
|         22050 |                       2 |               2304 |
|         24000 |                       1 |               1152 |
|         24000 |                       2 |               2304 |
|         32000 |                       1 |               2304 |
|         32000 |                       2 |               4608 |
|         44100 |                       1 |               2304 |
|         44100 |                       2 |               4608 |
|         48000 |                       1 |               2304 |
|         48000 |                       2 |               4608 |
+---------------+-------------------------+------------------+*/


//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
#define I2S_DMA_PAGE_SIZE_M4A 8192//4096//8192//   //Use frequency mapping table and set this value to number of decoded bytes 
//Options:- 1152, 2304, 4608

#define NUM_CHANNELS CH_STEREO   //Use m4a file properties to determine number of channels
//Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ SR_44p1KHZ   //Use m4a file properties to identify frequency and use appropriate macro
//Options:- SR_8KHZ     =>8000hz  - PASS       
//          SR_16KHZ    =>16000hz - PASS
//          SR_24KHZ    =>24000hz - PASS
//          SR_32KHZ    =>32000hz - PASS
//          SR_48KHZ    =>48000hz - PASS
//          SR_96KHZ    =>96000hz ~ NOT SUPPORTED
//          SR_7p35KHZ  =>7350hz  ~ NOT SUPPORTED
//          SR_14p7KHZ  =>14700hz ~ NOT SUPPORTED
//          SR_22p05KHZ =>22050hz - PASS
//          SR_29p4KHZ  =>29400hz ~ NOT SUPPORTED
//          SR_44p1KHZ  =>44100hz - PASS
//          SR_88p2KHZ  =>88200hz ~ NOT SUPPORTED

#define BITS_PER_CHANNEL	WL_16b

#define FILE_NAME "sound.m4a"    //Specify the file name you wish to play that is present in the SDCARD

#define URL_M4A "http://audio.xmcdn.com/group30/M02/83/76/wKgJXll66BnwsJmKADP_jqgbnBs147.m4a"  //128kbps LC-AAC
//#define URL_M4A "http://audio.xmcdn.com/group24/M04/01/87/wKgJMFhg7P_yLGdqACtj0yUya6Q213.m4a"  //24Kbps HE-AAC V2
//#define URL_M4A "http://audio.xmcdn.com/group24/M04/01/8A/wKgJNVhg7Qrx7V4YAHF3ZOCo1Q8958.m4a" //64Kbps HE-AAC V1

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//

SDRAM_DATA_SECTION int16_t decode_buf[I2S_DMA_PAGE_SIZE_M4A];
#define I2S_DMA_PAGE_NUM    2   // Vaild number is 2~4

SDRAM_DATA_SECTION static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE_M4A*I2S_DMA_PAGE_NUM];
SDRAM_DATA_SECTION static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE_M4A*I2S_DMA_PAGE_NUM];

#define I2S_SCLK_PIN		PC_1
#define I2S_WS_PIN		PC_0
#define I2S_SD_PIN		PC_2

static i2s_t i2s_obj;

static float volume_ratio = 0.5; // codec volume ration. 0.0 ~1.0
static void i2s_tx_complete(void *data, char *pbuf)
{   
	//
}

static void i2s_rx_complete(void *data, char* pbuf)
{
	//
}

static void my_printf(void* avc, int level, const char* fmt, va_list list)
{
	if(level <= av_log_get_level())
	{
		rtl_vprintf(fmt, list);
	}
}

static void initialize_audio(u8 ch_num, int sample_rate, int dma_page_sz)
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

	//printf("\n\r@@@@@@@@@@ch_num_idx=%d smpl_rate_idx=0x%x dma_page_sz=%d", ch_num_idx, smpl_rate_idx, dma_page_sz);

	i2s_obj.channel_num = ch_num_idx;
	i2s_obj.sampling_rate = smpl_rate_idx;
	i2s_obj.word_length = BITS_PER_CHANNEL;
	i2s_obj.direction = I2S_DIR_TXRX;	 
	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, \
		I2S_DMA_PAGE_NUM, dma_page_sz);
	i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_tx_complete, (uint32_t)&i2s_obj);
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_rx_complete, (uint32_t)&i2s_obj);

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
	sgtl5000_enable();
	sgtl5000_setVolume(volume_ratio);
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5680
	alc5680_i2c_init();
	set_alc5680_volume(0x74, 0x74);
#endif

}
static void audio_play_pcm(char *buf, int len, int dma_page_sz)
{
	int *ptx_buf;

retry:
	ptx_buf = i2s_get_tx_page(&i2s_obj);
	if(ptx_buf){
		if(len < dma_page_sz){
			/* if valid audio data short than a page, make sure the rest of the page set to 0*/
			memset((void*)ptx_buf, 0x00, dma_page_sz);
			memcpy((void*)ptx_buf, (void*)buf, len);
		}else
			memcpy((void*)ptx_buf, (void*)buf, len);
		i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
	}else{
		vTaskDelay(1);
		goto retry;
	}
}

void audio_play_sd_m4a(u8* filename){       
	AVFormatContext *in = NULL;
	AVStream *st = NULL;
	void *wav = NULL;
	int ret, i;
	HANDLE_AACDECODER handle;
	AAC_DECODER_ERROR err;
	int pcm_size = 0;
	CStreamInfo *info = NULL;
	int *ptx_buf;
	int first_frame = 1;
	u32 start_time = 0;
	u32 start_heap = 0;
	u8 sample_rate = SR_44p1KHZ;
	int i2s_dma_page_size = I2S_DMA_PAGE_SIZE_M4A;
	start_time = rtw_get_current_time();
	start_heap = xPortGetFreeHeapSize();
	av_log_set_level(AV_LOG_INFO);
	av_log_set_callback(my_printf);
	av_register_all();
	avformat_network_init();
	ret = avformat_open_input(&in, filename, NULL, NULL);
	if (ret < 0) {
		printf("open input failed\n");
		return;
	}
	for (i = 0; i < in->nb_streams && !st; i++) {
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC)
			st = in->streams[i];
	}
	if (!st) {
		printf("No AAC stream found\n");
		return;
	}
	if (!st->codec->extradata_size) {
		printf("No AAC ASC found\n");
		return;
	}
	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	err = aacDecoder_ConfigRaw(handle, &st->codec->extradata, &st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		printf("Unable to decode the ASC(err=%d)\n", err);
		return;
	}

#if WRITE_RAW_DATA_SD
	char abs_path_op[16]; //Path to output file
	FIL m_file;
	int bw = 0;
	const char *outfile = "AudioSDTest.pcm";
	extern char file_logical_drv[4]; //root diretor
	memset(abs_path_op, 0x00, sizeof(abs_path_op));
	strcpy(abs_path_op, file_logical_drv);
	sprintf(&abs_path_op[strlen(abs_path_op)],"%s", outfile);
	FRESULT res = f_open(&m_file, abs_path_op, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);  // open read only file
	if(res != FR_OK){
		printf("Open source file %(s) fail.\n", outfile);
		goto exit;
	}
#endif
	while (1){
		int i;
		UINT valid;
		AVPacket pkt = { 0 };
		int ret = av_read_frame(in, &pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN))
				continue;
			break;
		}
		if (pkt.stream_index != st->index) {
			av_free_packet(&pkt);
			continue;
		}

		valid = pkt.size;
		err = aacDecoder_Fill(handle, &pkt.data, &pkt.size, &valid);
		if (err != AAC_DEC_OK) {
			printf("Fill failed: %x\n", err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buf, I2S_DMA_PAGE_SIZE_M4A, 0);
		av_free_packet(&pkt);
		if (err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;
		if (err != AAC_DEC_OK) {
			printf("Decode failed: %x\n", err);
			continue;
		}
		if(!info){
			info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				printf("No stream info\n");
				break;
			}
			printf("sampleRate: %d, frameSize: %d, numChannels: %d\n", info->sampleRate, info->frameSize, info->numChannels);
			i2s_dma_page_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16b)?2:3);
			pcm_size = i2s_dma_page_size;
		}
		if(first_frame){
			initialize_audio(info->numChannels, info->sampleRate, i2s_dma_page_size);
			first_frame = 0;
		}
#if WRITE_RAW_DATA_SD
		do{
			res = f_write(&m_file, decode_buf, pcm_size, (u32*)&bw);
			if(res){
				f_lseek(&m_file, 0); 
				printf("Write error.\n");
			}
			//printf("Write %d bytes.\n", bw);
		}while(bw < pcm_size);
		bw = 0;
#endif
		audio_play_pcm(decode_buf, pcm_size, i2s_dma_page_size);
	}
exit:
#if WRITE_RAW_DATA_SD
	res = f_close(&m_file);
	if(res){
		printf("close file (%s) fail.\n", outfile);
	}
#endif
	printf("decoding finished (Heap used: 0x%x, Time passed: %dms)\n", start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	avformat_close_input(&in);
	aacDecoder_Close(handle);
	return;
}

void audio_play_http_m4a(char* filename){       
	AVFormatContext *in = NULL;
	AVDictionary *opts = NULL;
	AVStream *st = NULL;
	int ret, i;
	HANDLE_AACDECODER handle;
	AAC_DECODER_ERROR err;
	int dma_page_sz = 0;
	int pcm_size = 0;
	CStreamInfo *info = NULL;
	int *ptx_buf;
	int first_frame = 1;
	u32 start_time = 0;
	u32 start_heap = 0;
	start_time = rtw_get_current_time();
	start_heap = xPortGetFreeHeapSize();
	
	av_log_set_level(AV_LOG_INFO);
	av_log_set_callback(my_printf);
	av_register_all();
	avformat_network_init();
	av_dict_set(&opts, "protocol_whitelist", "http,tcp", 0);
	ret = avformat_open_input(&in, filename, NULL, &opts);
	if (ret < 0) {
		printf("open input failed\n");
		return;
	}

	for (i = 0; i < in->nb_streams && !st; i++) {
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC){
			st = in->streams[i];
		}
	}
	if (!st) {
		printf("No AAC or MP3 stream found\n");
		goto exit;
	}

	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	err = aacDecoder_ConfigRaw(handle, &st->codec->extradata, &st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		printf("Unable to decode the ASC(err=%d)\n", err);
		goto exit;
	}
	while (1){
		int i;
		UINT valid;
		AVPacket pkt = { 0 };
		int current_sample = 0;
		int ret = av_read_frame(in, &pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN))
				continue;
			vTaskDelay(100);
			break;
		}
		if (pkt.stream_index != st->index) {
			av_free_packet(&pkt);
			continue;
		}

		valid = pkt.size;
		
		err = aacDecoder_Fill(handle, &pkt.data, &pkt.size, &valid);
		if (err != AAC_DEC_OK) {
			printf("Fill failed: %x\n", err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buf, I2S_DMA_PAGE_SIZE_M4A, 0);
		av_free_packet(&pkt);
		if (err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;
		if (err != AAC_DEC_OK) {
			printf("Decode failed: %x\n", err);
			continue;
		}
		if(!info){
			info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				printf("No stream info\n");
				break;
			}
			printf("sampleRate: %d, frameSize: %d, numChannels: %d\n", info->sampleRate, info->frameSize, info->numChannels);
			pcm_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16b)?2:3);
			dma_page_sz = pcm_size;
		}
		
		if(first_frame){
			initialize_audio(info->numChannels, info->sampleRate, dma_page_sz);
			first_frame = 0;
		}
		audio_play_pcm(decode_buf, pcm_size, dma_page_sz);
	}
exit:
	printf("decoding finished (Heap used: 0x%x, Time passed: %dms)\n", start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	aacDecoder_Close(handle);
	avformat_close_input(&in);
	return;
}

void example_audio_m4a_thread(void* param){

	printf("Audio codec demo begin......\n");

#if CONFIG_EXAMPLE_M4A_FROM_HTTP
	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS)
	{
		vTaskDelay(1000);
	}
	audio_play_http_m4a(URL_M4A);
#else
	audio_play_sd_m4a(FILE_NAME);
#endif	
exit:
	vTaskDelete(NULL);
}

void example_audio_m4a(void)
{
	if(xTaskCreate(example_audio_m4a_thread, ((const char*)"example_audio_m4a_thread"), 4000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_m4a_thread) failed", __FUNCTION__);
}

