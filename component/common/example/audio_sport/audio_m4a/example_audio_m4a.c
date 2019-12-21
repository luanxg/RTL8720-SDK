#include "example_audio_m4a.h"
#include "platform/platform_stdlib.h"
#include "wifi_conf.h"
#include "lwip/arch.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"
#include "libAACdec/include/aacdecoder_lib.h"
#include <libavformat/avformat.h>
#include <libavformat/url.h>
#include "rl6548.h"

#if CONFIG_EXAMPLE_AUDIO_M4A

#define CONFIG_EXAMPLE_M4A_FROM_HTTP	1
#define WRITE_RAW_DATA_SD		0

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
#define NUM_CHANNELS CH_STEREO   //Use m4a file properties to determine number of channels
//Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ SR_44P1K	//Use m4a file properties to identify frequency and use appropriate macro
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

#define BITS_PER_CHANNEL	WL_16

#define FILE_NAME "AudioSDTest.m4a"    //Specify the file name you wish to play that is present in the SDCARD

#define URL_M4A "http://audio.xmcdn.com/group30/M02/83/76/wKgJXll66BnwsJmKADP_jqgbnBs147.m4a"  //128kbps LC-AAC
//#define URL_M4A "http://audio.xmcdn.com/group24/M04/01/87/wKgJMFhg7P_yLGdqACtj0yUya6Q213.m4a"  //24Kbps HE-AAC V2
//#define URL_M4A "http://audio.xmcdn.com/group24/M04/01/8A/wKgJNVhg7Qrx7V4YAHF3ZOCo1Q8958.m4a" //64Kbps HE-AAC V1

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
#ifdef __GNUC__
#undef SDRAM_DATA_SECTION
#define SDRAM_DATA_SECTION 
#endif

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

SDRAM_DATA_SECTION int16_t decode_buf[M4A_DECODE_SIZE];

SDRAM_DATA_SECTION static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];

static void my_printf(void* avc, int level, const char* fmt, va_list list)
{
	if(level <= av_log_get_level())
	{
		printf(fmt, list);
	}
}

u8 *sp_get_free_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	if (ptx_block->tx_gdma_own)
		return NULL;
	else{
		return ptx_block->tx_addr;
	}	
}

void sp_write_tx_page(u8 *src, u32 length)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	memcpy(ptx_block->tx_addr, src, length);
	ptx_block->tx_length = length;	
	ptx_block->tx_gdma_own = 1;
	sp_tx_info.tx_usr_cnt++;
	if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_tx_info.tx_usr_cnt = 0;
	}
}

void sp_release_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (sp_tx_info.tx_empty_flag){
	}
	else{
		ptx_block->tx_gdma_own = 0;
		sp_tx_info.tx_gdma_cnt++;
		if (sp_tx_info.tx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_tx_info.tx_gdma_cnt = 0;
		}
	}
}

u8 *sp_get_ready_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (ptx_block->tx_gdma_own){
		sp_tx_info.tx_empty_flag = 0;
		return ptx_block->tx_addr;
	}
	else{
		sp_tx_info.tx_empty_flag = 1;
		return sp_tx_info.tx_zero_block.tx_addr;	//for audio buffer empty case
	}
}

u32 sp_get_ready_tx_length(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

	if (sp_tx_info.tx_empty_flag){
		return sp_tx_info.tx_zero_block.tx_length;
	}
	else{
		return ptx_block->tx_length;
	}
}

void sp_tx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 tx_addr;
	u32 tx_length;
	
	GDMA_InitStruct = &(gs->SpTxGdmaInitStruct);

	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
	
	sp_release_tx_page();
	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	GDMA_SetSrcAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_length>>2);
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
}

void sp_rx_complete(void *data, char* pbuf)
{
	//
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;

	PLL_Sel(PLL_PCM);
	
	//PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/88.2k

	RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);	

	//set clock divider to gen clock sample_rate*256 from 98.304M.
	switch(psp_obj->sample_rate){
		case SR_48K:
			div = 8;
			break;
		case SR_96K:
			div = 4;
			break;
		case SR_32K:
			div = 12;
			break;
		case SR_16K:
			div = 24;
			break;
		case SR_8K:
			div = 48;
			break;
		case SR_44P1K:
			div = 4;
			break;
		case SR_88P2K:
			div = 2;
			break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
}

static void sp_init_tx_variables(void)
{
	int i;

	for(i=0; i<SP_ZERO_BUF_SIZE; i++){
		sp_zero_buf[i] = 0;
	}
	sp_tx_info.tx_zero_block.tx_addr = (u32)sp_zero_buf;
	sp_tx_info.tx_zero_block.tx_length = (u32)SP_ZERO_BUF_SIZE;
	
	sp_tx_info.tx_gdma_cnt = 0;
	sp_tx_info.tx_usr_cnt = 0;
	sp_tx_info.tx_empty_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_tx_info.tx_block[i].tx_gdma_own = 0;
		sp_tx_info.tx_block[i].tx_addr = sp_tx_buf+i*SP_DMA_PAGE_SIZE;
		sp_tx_info.tx_block[i].tx_length = SP_DMA_PAGE_SIZE;
	}
}

static void initialize_audio(uint8_t ch_num, int sample_rate)
{
    printf("ch:%d sr:%d\r\n", ch_num, sample_rate);

	uint8_t smpl_rate_idx = SR_16K;
	uint8_t ch_num_idx = CH_STEREO;
	pSP_OBJ psp_obj = &sp_obj;
	u32 tx_addr;
	u32 tx_length;

	switch(ch_num){
    	case 1: ch_num_idx = CH_MONO;   break;
    	case 2: ch_num_idx = CH_STEREO; break;
    	default: break;
	}
	
	switch(sample_rate){
    	case 8000:  smpl_rate_idx = SR_8K;     break;
    	case 16000: smpl_rate_idx = SR_16K;    break;
    	//case 22050: smpl_rate_idx = SR_22p05K; break;
    	//case 24000: smpl_rate_idx = SR_24K;    break;		
    	case 32000: smpl_rate_idx = SR_32K;    break;
    	case 44100: smpl_rate_idx = SR_44P1K;  break;
    	case 48000: smpl_rate_idx = SR_48K;    break;
    	default: break;
	}

	psp_obj->mono_stereo= ch_num_idx;
	psp_obj->sample_rate = smpl_rate_idx;
	psp_obj->word_len = WL_16;
	psp_obj->direction = APP_LINE_OUT;	 
	sp_init_hal(psp_obj);
	
	sp_init_tx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);

	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	AUDIO_SP_TXGDMA_Init(0, &SPGdmaStruct.SpTxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_tx_complete, tx_addr, tx_length);

}

static void audio_play_pcm(char *buf, uint32_t len)
{
	u32 offset = 0;
	u32 tx_len;
	while(len){
		if (sp_get_free_tx_page()){
			tx_len = (len >= SP_DMA_PAGE_SIZE)?SP_DMA_PAGE_SIZE:len;
			sp_write_tx_page(((u8 *)buf+offset), tx_len);
			offset += tx_len;
			len -= tx_len;
		}
		else{
			vTaskDelay(1);
		}
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
	u8 sample_rate = SR_44P1K;
	int m4a_decode_size = M4A_DECODE_SIZE;
	start_time = rtw_get_current_time();
	start_heap = xPortGetFreeHeapSize();
	printf("Remaing heap size: %x\n", start_heap);
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
		err = aacDecoder_DecodeFrame(handle, decode_buf, M4A_DECODE_SIZE, 0);
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
			m4a_decode_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16)?2:3);
			pcm_size = m4a_decode_size;
		}
		if(first_frame){
			initialize_audio(info->numChannels, info->sampleRate);
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
		audio_play_pcm(decode_buf, pcm_size);
	}
exit:
#if WRITE_RAW_DATA_SD
	res = f_close(&m_file);
	if(res){
		printf("close file (%s) fail.\n", outfile);
	}
#endif
	printf("decoding finished (Heap ever free minimum: 0x%x, Time passed: %dms)\n", xPortGetMinimumEverFreeHeapSize(), rtw_get_current_time() - start_time);
	avformat_close_input(&in);
	aacDecoder_Close(handle);
	return;
}

#if CONFIG_EXAMPLE_M4A_FROM_HTTP
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
		err = aacDecoder_DecodeFrame(handle, decode_buf, M4A_DECODE_SIZE, 0);
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
			pcm_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16)?2:3);
			dma_page_sz = pcm_size;
		}
		
		if(first_frame){
			initialize_audio(info->numChannels, info->sampleRate);
			first_frame = 0;
		}
		audio_play_pcm(decode_buf, pcm_size);
	}
exit:
	printf("decoding finished (Heap used: 0x%x, Time passed: %dms)\n", start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	aacDecoder_Close(handle);
	avformat_close_input(&in);
	return;
}
#endif

void example_audio_m4a_thread(void)
{

	printf("Audio codec m4a demo begin......\n");

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
	if(xTaskCreate(example_audio_m4a_thread, ((const char*)"example_audio_m4a_thread"), 10000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_m4a_thread) failed", __FUNCTION__);
}

#endif

