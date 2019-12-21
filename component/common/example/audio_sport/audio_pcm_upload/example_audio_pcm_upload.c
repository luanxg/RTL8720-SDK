#include "FreeRTOS.h"
#include "task.h"
#include <platform_stdlib.h>

#include "device.h"
#include "diag.h"
#include "osdep_service.h"
#include "wifi_conf.h"

#include "tftp/tftp.h"

#include "rl6548.h"
#include "example_audio_pcm_upload.h"

//gpio
#include "gpio_api.h"   // mbed
#include "gpio_irq_api.h"   // mbed
#define GPIO_IRQ_PIN            PA_30

//static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
SRAM_NOCACHE_DATA_SECTION static u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];

//#define RECORD_LEN (1024*1024)  //for record
#define RECORD_LEN (1024*190)//for check data
unsigned int record_length = 0;
unsigned char record_data[RECORD_LEN];

#define WRITE_FREE      0
#define WRITE_LOCK      1

static unsigned char audio_status = 0;
static unsigned char write_status = 0;
static unsigned char wifi_state = 0;
static unsigned char audio_start = 0;

#define RECORD_WAV_NAME "AMEBA"
#define TFTP_HOST_IP_ADDR "192.168.3.12"
#define TFTP_HOST_PORT  69
#define TFTP_MODE       "octet"

_sema       VOICE_TRIGGER_SEMA;//INFORM audio TO RECORD DATA
_sema       VOICE_UPLOAD_SEMA;//INFORM TFTP TO UPLOAD DATA

#define INTERNAL_TEST 0

#if INTERNAL_TEST
extern char upgrade_ip[64];
#endif

static u8 sp_full_buf[SP_FULL_BUF_SIZE];
static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_RX_INFO sp_rx_info;

u8 *sp_get_ready_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	if (prx_block->rx_gdma_own)
		return NULL;
	else{
		return prx_block->rx_addr;
	}
}

void sp_read_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);

	if(prx_block->rx_gdma_own){
		DBG_8195A("No Idle Rx Page\r\n");
		return;
	}
	
	prx_block->rx_gdma_own = 1;
	sp_rx_info.rx_usr_cnt++;
	if (sp_rx_info.rx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_rx_info.rx_usr_cnt = 0;
	}
}

void sp_release_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (sp_rx_info.rx_full_flag){
	}
	else{
		prx_block->rx_gdma_own = 0;
		sp_rx_info.rx_gdma_cnt++;
		if (sp_rx_info.rx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_rx_info.rx_gdma_cnt = 0;
		}
	}
}

u8 *sp_get_free_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (prx_block->rx_gdma_own){
		sp_rx_info.rx_full_flag = 0;
		return prx_block->rx_addr;
	}
	else{
		sp_rx_info.rx_full_flag = 1;
		return sp_rx_info.rx_full_block.rx_addr;	//for audio buffer full case
	}
}

u32 sp_get_free_rx_length(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

	if (sp_rx_info.rx_full_flag){
		return sp_rx_info.rx_full_block.rx_length;
	}
	else{
		return prx_block->rx_length;
	}
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	//PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k

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
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
	Pinmux_Config(_PB_24, PINMUX_FUNCTION_DMIC);
	Pinmux_Config(_PB_25, PINMUX_FUNCTION_DMIC);		
}

static void sp_init_rx_variables(void)
{
	int i;

	sp_rx_info.rx_full_block.rx_addr = (u32)sp_full_buf;
	sp_rx_info.rx_full_block.rx_length = (u32)SP_FULL_BUF_SIZE;
	
	sp_rx_info.rx_gdma_cnt = 0;
	sp_rx_info.rx_usr_cnt = 0;
	sp_rx_info.rx_full_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_rx_info.rx_block[i].rx_gdma_own = 1;
		sp_rx_info.rx_block[i].rx_addr = sp_rx_buf+i*SP_DMA_PAGE_SIZE;
		sp_rx_info.rx_block[i].rx_length = SP_DMA_PAGE_SIZE;
	}
}

static void check_wifi_connection()
{
    while(1){
        vTaskDelay(1000);
#if INTERNAL_TEST
        if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS && upgrade_ip[0]!=0) {
            printf("wifi is connected ip = %s\r\n",upgrade_ip);
            wifi_state = 1;
            break;
        }
#else
        if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS ){
            printf("wifi is connected\r\n");
            wifi_state = 1;
            break;
        }
#endif
    }
}

void voice_demo_irq_handler (uint32_t id, gpio_irq_event event)
{
	if(wifi_state == 1 && write_status == WRITE_FREE) {
		audio_start = 1;
		rtw_up_sema_from_isr(&VOICE_TRIGGER_SEMA);
	}

	printf("voice irq = %d\n",audio_start);
}

static void test_rx_complete(void *data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 rx_addr;
	u32 rx_length;
	char *pbuf;
	
	GDMA_InitStruct = &(gs->SpRxGdmaInitStruct);
	
	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

	sp_release_rx_page();
	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	GDMA_SetDstAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_length>>2);	
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);

	if((audio_start) == 1 && (write_status == WRITE_FREE)) {     
		if((record_length+SP_DMA_PAGE_SIZE) < RECORD_LEN) {
			pbuf = sp_get_ready_rx_page();
			if(record_length == 0) {
				_memcpy((void*)(record_data+record_length+sizeof(WAVE_HEADER)),(void*)pbuf,SP_DMA_PAGE_SIZE);
				record_length+=(SP_DMA_PAGE_SIZE+sizeof(WAVE_HEADER));
			} else {
				_memcpy((void*)(record_data+record_length),(void*)pbuf,SP_DMA_PAGE_SIZE);
				record_length+=SP_DMA_PAGE_SIZE;
			}
			sp_read_rx_page();
		} else {
			write_status = WRITE_LOCK;
			printf("write length = %x\r\n",record_length);
			rtw_up_sema_from_isr(&VOICE_UPLOAD_SEMA);
		}
	}   
}

void example_audio_init()
{
	printf("audio initial\r\n");

	pSP_OBJ psp_obj = &sp_obj;
	u32 rx_addr;
	u32 rx_length;

	psp_obj->mono_stereo= CH_MONO;
	psp_obj->sample_rate = SR_48K;
	psp_obj->word_len = WL_16;
	psp_obj->direction = APP_DMIC_IN;	 
	sp_init_hal(psp_obj);
	
	sp_init_rx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_RxStart(AUDIO_SPORT_DEV, ENABLE);

	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	AUDIO_SP_RXGDMA_Init(0, &SPGdmaStruct.SpRxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)test_rx_complete, rx_addr, rx_length);	

	printf("audio init finish\r\n");
}

void tftp_audio_send_handler(unsigned char *buffer,int *len,unsigned int index)
{
	//static unsigned int total_size = record_length;
	int remain_len = record_length -(512*(index-1));
	if(remain_len/512) {
		//memset(buffer,index,512);
		memcpy(buffer,record_data+512*(index-1),512);
		*len = 512;
	} else {
		//memset(buffer,index,(total_size%512));
		memcpy(buffer,record_data+512*(index-1),remain_len%512);
		*len = (record_length%512);
	}
	//printf("handler = %d size = %d\r\n",total_size,(*len));
}

void tftp_init_audio(tftp *tftp_info)
{
	tftp_info->send_handle = tftp_audio_send_handler;
	tftp_info->tftp_file_name=RECORD_WAV_NAME;
	printf("send file name = %s\r\n",tftp_info->tftp_file_name);
	tftp_info->tftp_mode = TFTP_MODE;
	tftp_info->tftp_port = TFTP_HOST_PORT;
#if INTERNAL_TEST
	tftp_info->tftp_host = upgrade_ip;
#else
	tftp_info->tftp_host = TFTP_HOST_IP_ADDR;
#endif
	tftp_info->tftp_op = WRQ;//FOR READ OPERATION
	tftp_info->tftp_retry_num = 5;
	tftp_info->tftp_timeout = 2;//second
	printf("tftp retry time = %d timeout = %d seconds\r\n",tftp_info->tftp_retry_num,tftp_info->tftp_timeout);
}

void example_audio_transfer_thread(void* param)
{
	while(1) {
		rtw_down_sema(&VOICE_TRIGGER_SEMA);
		sp_read_rx_page();
		printf("start_record_data\r\n");
	}
}

void example_audio_pcm_upload_thread(void* param)
{
	WAVE_HEADER w_header;
	tftp tftp_info;
	static int tftp_count = 0;
	char tftp_filename[64]={0};
	gpio_irq_t voice_irq;
	gpio_t voice_pinr;
	
	memset(&tftp_info,0,sizeof(tftp));
	strcpy(w_header.w_header.fccID, "RIFF");  
	strcpy(w_header.w_header.fccType, "WAVE");
	strcpy(w_header.w_fmt.fccID, "fmt ");
	w_header.w_fmt.dwSize=16;
	w_header.w_fmt.wFormatTag=1;
	w_header.w_fmt.wChannels=1;
	w_header.w_fmt.dwSamplesPerSec=48000;
	w_header.w_fmt.dwAvgBytesPerSec=w_header.w_fmt.dwSamplesPerSec*2;
	w_header.w_fmt.wBlockAlign=2;
	w_header.w_fmt.uiBitsPerSample=16; 
	strcpy(w_header.w_data.fccID, "data");

	rtw_init_sema(&VOICE_TRIGGER_SEMA,0);
	rtw_init_sema(&VOICE_UPLOAD_SEMA,0);

	check_wifi_connection();

	//gpio
	printf("GPIO_IRQ\r\n");
	// Initial Push Button pin as interrupt source
	gpio_irq_init(&voice_irq, GPIO_IRQ_PIN, voice_demo_irq_handler, (uint32_t)(&voice_pinr));
	gpio_irq_set(&voice_irq, IRQ_RISE, 1);   // Falling Edge Trigger
	gpio_irq_enable(&voice_irq);
	//

	//tftp
	tftp_init_audio(&tftp_info);
	//tftp
	example_audio_init();

	if(xTaskCreate(example_audio_transfer_thread, ((const char*)"example_audio_transfer_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);

	while(1) {
		//vTaskDelay(1000);
		rtw_down_sema(&VOICE_UPLOAD_SEMA);
		if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS ) {
			wifi_state = 1;
			if((write_status == WRITE_LOCK) && audio_start){
				printf("wifi connect\r\n");
				w_header.w_data.dwSize = record_length; 
				memcpy(record_data,&w_header,sizeof(WAVE_HEADER));
				memset(tftp_filename,0,sizeof(tftp_filename));
				sprintf(tftp_filename,"%s_%d.wav",RECORD_WAV_NAME,tftp_count);
				//sprintf(tftp_filename,"%s_%d.wav",tftp_info.tftp_file_name,tftp_count);
				tftp_info.tftp_file_name = tftp_filename;
				printf("File name = %s\r\n",tftp_info.tftp_file_name);
				if(tftp_client_start(&tftp_info) == 0)
					printf("Send file successful\r\n");
				else
					printf("Send file fail\r\n");
				audio_start = 0;
				record_length = 0;
				write_status = WRITE_FREE;
				tftp_count++;
			}
		} else {
			wifi_state = 0;
		}
	}
}

void example_audio_pcm_upload(void)
{
	if(xTaskCreate(example_audio_pcm_upload_thread, ((const char*)"example_audio_pcm_upload_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
