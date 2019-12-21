#include "example_audio_hls.h"
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


#define ____GLOBAL_PARAM________________________________


// 仅支持LC-AAC直播

#define BITS_PER_CHANNEL		WL_16

//SAL_PSRAM_MNGT_ADPT  PSRAMTestMngtAdpt[1];

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

//uint16_t	decode_buffer[SP_DMA_PAGE_SIZE];
//static u8	sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];

uint16_t *decode_buffer = (uint16_t *)(0x02000000);
u8 *sp_tx_buf = (u8 *)(0x02080000);

static u8	sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];

//#define URL_HLS "http://106.75.145.173/m3u/test/index.m3u8"				// 猫王电台,已停播...
//#define URL_HLS "http://rthkaudio2-lh.akamaihd.net/i/radio2_1@355865/index_56_a-p.m3u8"	// 香港电台2
#define URL_HLS "http://cdn.can.cibntv.net/12/201702161000/rexuechangan01/rexuechangan01.m3u8"	
//#define URL_HLS "http://60.199.188.61/HLS/WG_ETTV-N/01.m3u8"

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
static xSemaphoreHandle avpkts_get_format_sema = NULL;


#if 0
VOID
PSRAMISRHandle(
    IN  VOID *Data
)
{
         u32 intr_status_cal_fail = 0; 
         u32 intr_status_timeout = 0; 
         
         intr_status_cal_fail = (PSRAM_PHY_REG_Read(0x0) & BIT16) >> 16;
         PSRAM_PHY_REG_Write(0x0, PSRAM_PHY_REG_Read(0x0)) ;
         intr_status_timeout = (PSRAM_PHY_REG_Read(0x1C) & BIT31) >> 31;
         PSRAM_PHY_REG_Write(0x1C, PSRAM_PHY_REG_Read(0x1C)) ;
         DBG_8195A("cal_fail = %x timeout = %x\n",intr_status_cal_fail, intr_status_timeout);
}


void PSRAMInit(void){
         u32 temp;
         PSAL_PSRAM_MNGT_ADPT  pSalPSRAMMngtAdpt = NULL;
         pSalPSRAMMngtAdpt = &PSRAMTestMngtAdpt[0];
         InterruptRegister((IRQ_FUN) PSRAMISRHandle, PSRAMC_IRQ, (u32) (pSalPSRAMMngtAdpt), 3);
         InterruptEn(PSRAMC_IRQ, 3);

         /*set rwds pull down*/
         temp = HAL_READ32(PINMUX_REG_BASE, 0x104);
         temp &= ~(PAD_BIT_PULL_UP_RESISTOR_EN | PAD_BIT_PULL_DOWN_RESISTOR_EN);
         temp |= PAD_BIT_PULL_DOWN_RESISTOR_EN;
         HAL_WRITE32(PINMUX_REG_BASE, 0x104, temp);

         PSRAM_CTRL_StructInit(&pSalPSRAMMngtAdpt->PCTL_InitStruct);
         PSRAM_CTRL_Init(&pSalPSRAMMngtAdpt->PCTL_InitStruct); 
         
         PSRAM_PHY_REG_Write(0x0, (PSRAM_PHY_REG_Read(0x0)& (~0x1)));
         
         /*set N/J initial value HW calibration*/
         PSRAM_PHY_REG_Write(0x4, 0x2030316);

         /*start HW calibration*/
         PSRAM_PHY_REG_Write(0x0, 0x111);
}
#endif

static void hls_my_printf(void* avc, int level, const char* fmt, va_list list)
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
	u32 sel = 0;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k

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
			sel = 1;
			break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Sel(sel);
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
		vTaskDelay(500);
		fATW0((void *)"hw_w550");
		fATW1((void *)"012345679");
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

static void hls_initialize_audio(uint8_t ch_num, int sample_rate)
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

static void hls_audio_play_pcm(char *buf, uint32_t len)
{
	u32 offset = 0;
	u32 tx_len;

	//DiagPrintf("play pcm len = %x\n", len);
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
	avpkts_get_format_sema = xSemaphoreCreateCounting(1, 0);


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
	vSemaphoreDelete(avpkts_get_format_sema);
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
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] avformat_open_input (url: %s) OK !\n", __func__, URL_HLS);
	}

	for (i = 0; i < in->nb_streams && !avpkts->st; i++) {
		//DiagPrintf("=========  codec_id = %x  ==========\n" ,in->streams[i]->codec->codec_id);
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC)
			avpkts->st = in->streams[i];
	}
	if (!avpkts->st) {
		av_log(NULL, AV_LOG_FATAL, "\r\n[%s] no AAC stream found !", __func__);
		goto exit;
	}
	xSemaphoreGive(avpkts_get_format_sema);	

	while(1) {
		//DiagPrintf("start dwnld while\n");
		if(avpkts->pkt_num < HLS_PKTS_POOL_SIZE) {
			DiagPrintf("b\n");
			DiagPrintf("%x\n",xPortGetFreeHeapSize());
			ret = av_read_frame(in, &avpkts->pkts[avpkts->pkt_wt]);
			DiagPrintf("%x\n",xPortGetFreeHeapSize());
			DiagPrintf("e\n");
			
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "\r\n[%s] read frame fail ! ret=%d ", __func__, ret);			
				if (ret == AVERROR(EAGAIN))
					continue;
				vTaskDelay(100);
				break;
			}
			if (avpkts->pkts[avpkts->pkt_wt].stream_index != avpkts->st->index) {	
				DiagPrintf("n\n");
				av_free_packet(&avpkts->pkts[avpkts->pkt_wt]);
				continue;
			}

			avpkts->pkt_wt = (++avpkts->pkt_wt) % HLS_PKTS_POOL_SIZE;
			xSemaphoreTake(avpkts_pkt_num_mutex, portMAX_DELAY);
			++avpkts->pkt_num;
			xSemaphoreGive(avpkts_pkt_num_mutex);
			xSemaphoreGive(avpkts_ready_dec_sema);	
		}else{
			vTaskDelay(1);
		}
			//while(xSemaphoreTake(avpkts_ready_download_sema, portMAX_DELAY) != pdTRUE);
			
			//DiagPrintf("end dwnld while\n");
	}
	
exit:
	printf("start_time = %x, start_heap = %x\n", start_time, start_heap);
	printf("end_time = %x, end_heap = %x\n", rtw_get_current_time(), xPortGetFreeHeapSize());	
	
	av_log(NULL, AV_LOG_FATAL, "[%s] Exit !!!(Heap used: 0x%x, Time passed: %dms)\n", __func__, start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	av_log(NULL, AV_LOG_FATAL, "Exit !!!(Heap used: 0x%x, Time passed: %dms)\n", start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
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

	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS);

	av_log(NULL, AV_LOG_INFO, "\r\nHLS decode begin......\n", __func__);
	handle = aacDecoder_Open(TT_MP4_ADTS, 1);
	
	while(xSemaphoreTake(avpkts_get_format_sema, portMAX_DELAY) != pdTRUE);
	//DBG_8195A("%x\n", (u32)avpkts);
	
	//DBG_8195A("%x\n", (u32)avpkts->st);
	//DBG_8195A("%x\n", (u32)avpkts->st->codec);

	//DBG_8195A("%x\n", avpkts->st->codec->extradata_size);	

	err = aacDecoder_ConfigRaw(handle, &avpkts->st->codec->extradata, &avpkts->st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		av_log(NULL, AV_LOG_FATAL, "\r\n[%s] unable to decode the ASC (err=%d) !", __func__, err);
		goto exit;
	}

	while(1) {
		
		//DiagPrintf("start dec while\n");
		while(xSemaphoreTake(avpkts_ready_dec_sema, portMAX_DELAY) != pdTRUE);
		valid = avpkts->pkts[avpkts->pkt_rd].size;
		//DiagPrintf("===========  pkt size = %x ============\n", valid);
refill:
		err = aacDecoder_Fill(handle, &avpkts->pkts[avpkts->pkt_rd].data, &avpkts->pkts[avpkts->pkt_rd].size, &valid);
		if (err != AAC_DEC_OK) {
			av_log(NULL, AV_LOG_FATAL, "\r\n[%s] aacDecoder_Fill fail ! err=0x%x", __func__, err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, decode_buffer, SP_DMA_PAGE_SIZE, 0);
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
			pcm_size = info->frameSize * info->numChannels * ((BITS_PER_CHANNEL == WL_16)?2:3);
		//	hls_dump_aac_stream_info(info);
		}
		if(first_frame){
			hls_initialize_audio(info->numChannels, info->sampleRate);
			first_frame = 0;
		}

		hls_audio_play_pcm(decode_buffer, pcm_size);
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
				//DiagPrintf("end dec while\n");
	}

exit:
	av_log(NULL, AV_LOG_FATAL, "\r\n[%s] Exit !!!", __func__);
	aacDecoder_Close(handle);
	hls_deinit();
}

void example_audio_hls(void)
{	
	//PSRAMInit();

	if(hls_init() < 0)
		goto err;
	DBG_8195A("Audio codec hls demo begin......\n");
	if(xTaskCreate(audio_hls_dwn_tsk, ((const char*)"hls_dwn"), 10000, (void *)&avPackets, tskIDLE_PRIORITY + 3, &hls_dwn_tsk_hdl) != pdPASS) {
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] Create dwn_tsk failed !", __func__);		
		goto err;
	}
	#if 1
	if(xTaskCreate(audio_hls_dec_tsk, ((const char*)"hls_dec"), 10000, (void *)&avPackets, tskIDLE_PRIORITY + 3, &hls_dec_tsk_hdl) != pdPASS) {
		av_log(NULL, AV_LOG_PANIC, "\r\n[%s] Create dec_tsk failed !", __func__);
		goto err;
	}
	#endif
	return;
err:
	hls_deinit();
}
