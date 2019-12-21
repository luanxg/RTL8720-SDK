#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>
#include "example_competitive_headphones.h"
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include "wifi_structures.h"
#include "lwip_netconf.h"

#include "rl6548.h"
#include "freertos_service.h"
#include "gpio_api.h"
//#include "voice_resample.h"
#include "resample/resample_library.h"

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];
_sema packet_buffer_sema = NULL;


static char udp_recv_buf[RECV_BUF_SIZE];

#define HEADPHONE_MIC	1
#ifdef HEADPHONE_MIC
static SP_RX_INFO sp_rx_info;
static _sema audio_rx_sema = NULL;
static char* audio_rx_pbuf;
SRAM_NOCACHE_DATA_SECTION 
static u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_full_buf[SP_FULL_BUF_SIZE];

#define RECV_BUF_NUM	2
u8 mic_recv_pkt_buff[SP_DMA_PAGE_SIZE*RECV_BUF_NUM];
u8 mic_resample_buf[RECORD_RESAMPLE_SIZE];
_sema mic_rx_sema = NULL;
static xQueueHandle mic_pkt_queue = NULL;
#define MIC_PKT_QUEUE_LENGTH	8


static u8 *sp_get_ready_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	if (prx_block->rx_gdma_own)
		return NULL;
	else{
		return prx_block->rx_addr;
	}
}

static void sp_read_rx_page(u8 *dst, u32 length)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	memcpy(dst, prx_block->rx_addr, length);
	prx_block->rx_gdma_own = 1;
	sp_rx_info.rx_usr_cnt++;
	if (sp_rx_info.rx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_rx_info.rx_usr_cnt = 0;
	}
}

static void sp_release_rx_page(void)
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

static u8 *sp_get_free_rx_page(void)
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

static u32 sp_get_free_rx_length(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

	if (sp_rx_info.rx_full_flag){
		return sp_rx_info.rx_full_block.rx_length;
	}
	else{
		return prx_block->rx_length;
	}
}

static void sp_rx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 rx_addr;
	u32 rx_length;
	portBASE_TYPE taskWoken = pdFALSE;
	static int count = 0;
	
	GDMA_InitStruct = &(gs->SpRxGdmaInitStruct);
	
	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

	sp_release_rx_page();

	//printf("a\n");
	if(sp_get_ready_rx_page()!= NULL){
		sp_read_rx_page((u8 *)&mic_recv_pkt_buff[SP_DMA_PAGE_SIZE*count], SP_DMA_PAGE_SIZE);
		count++;
		count = count%2;
		//printf("b\n");
		xSemaphoreGiveFromISR(mic_rx_sema, &taskWoken);
	    portEND_SWITCHING_ISR(taskWoken);
	}
	
	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	GDMA_SetDstAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_length>>2);	
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
}
#endif
u8 *sp_get_free_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	if (ptx_block->tx_gdma_own)
		return NULL;
	else {
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
	if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM) {
		sp_tx_info.tx_usr_cnt = 0;
	}
}

void sp_release_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (sp_tx_info.tx_empty_flag){
	} else {
		ptx_block->tx_gdma_own = 0;
		sp_tx_info.tx_gdma_cnt++;
		if (sp_tx_info.tx_gdma_cnt == SP_DMA_PAGE_NUM) {
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
	} else {
		sp_tx_info.tx_empty_flag = 1;
		return sp_tx_info.tx_zero_block.tx_addr;	//for audio buffer empty case
	}
}

u32 sp_get_ready_tx_length(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

	if (sp_tx_info.tx_empty_flag) {
		return sp_tx_info.tx_zero_block.tx_length;
	} else {
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
	CODEC_SetVolume(0x70, 0x70);
	PAD_CMD(_PB_28, DISABLE);
	PAD_CMD(_PB_29, DISABLE);
	PAD_CMD(_PB_30, DISABLE);
	PAD_CMD(_PB_31, DISABLE);
	if ((psp_obj->direction&APP_DMIC_IN) == APP_DMIC_IN){	
		PAD_CMD(_PB_24, DISABLE);
		PAD_CMD(_PB_25, DISABLE);
		Pinmux_Config(_PB_24, PINMUX_FUNCTION_DMIC);
		Pinmux_Config(_PB_25, PINMUX_FUNCTION_DMIC);	
	}

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

#ifdef HEADPHONE_MIC
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
#endif

static int wifi_connect_dongle()
{
		static rtw_network_info_t wifi = {0};
		static unsigned char password[65]={0};
		int mode, ret;
		unsigned long tick1 = xTaskGetTickCount();
		unsigned long tick2, tick3;
		strcpy((char *)wifi.ssid.val, "headphone_dongle");
		wifi.ssid.len = strlen("headphone_dongle");
		wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
		strcpy((char *)password, "12345678");
		wifi.password = password;
		wifi.password_len = strlen("12345678");

		printf("\n\rJoining BSS by SSID %s...\n\r", (char*)wifi.ssid.val);
		ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
						wifi.password_len, wifi.key_id, NULL);

		if(ret!= RTW_SUCCESS){
			if(ret == RTW_INVALID_KEY)
				printf("\n\rERROR:Invalid Key ");
			
			printf("\n\rERROR: Can't connect to AP");
			return -1;
		}
		tick2 = xTaskGetTickCount();
		printf("\r\nConnected after %dms.\n", (tick2-tick1));
#if CONFIG_LWIP_LAYER
			/* Start DHCPClient */
		if(LwIP_DHCP(0, DHCP_START)!= DHCP_ADDRESS_ASSIGNED)
			return -1;
		tick3 = xTaskGetTickCount();
		printf("\r\n\nGot IP after %dms.\n", (tick3-tick1));
#endif
		printf("\n\r");
		return 0;

}


static xSemaphoreHandle recv_qlen_lock;
uint32_t recv_queue_len_test = 0;
static xQueueHandle recv_pkt_queue = NULL;

u8 play_buf[SP_DMA_PAGE_SIZE];
u8 phone_test[256];
static int play_start = 0;
static int play_index = 0;
gpio_t i2s_intr;

int first_play_packet = 0;
int first_receive_packet = 0;


void example_audio_dac_thread(void* param)
{
	u32 tx_addr;
	u32 tx_length;
	u32 rx_addr;
	u32 rx_length;
	pSP_OBJ psp_obj = (pSP_OBJ)param;
	recv_pkt_t recv_pkt;
	
	DBG_8195A("Audio dac demo begin......\n");

	sp_init_hal(psp_obj);
	
	sp_init_tx_variables();
#ifdef HEADPHONE_MIC
	sp_init_rx_variables();
#endif
	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);
#ifdef HEADPHONE_MIC
	AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_RxStart(AUDIO_SPORT_DEV, ENABLE);	
#endif
	DBG_8195A("\nWait to play.\n");

#ifdef HEADPHONE_MIC
	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	AUDIO_SP_RXGDMA_Init(0, &SPGdmaStruct.SpRxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_rx_complete, rx_addr, rx_length);	
#endif
	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	AUDIO_SP_TXGDMA_Init(0, &SPGdmaStruct.SpTxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_tx_complete, tx_addr, tx_length);

	while(1){
		if(play_start == 0){
			if(xSemaphoreTake(packet_buffer_sema, 0xFFFFFFFF) != pdTRUE) continue;
		}
	
	    if (xQueueReceive( recv_pkt_queue, &recv_pkt, portMAX_DELAY ) != pdTRUE) {
            continue;
        }
		
		memcpy(&play_buf[RECV_SP_BUF_SIZE*play_index], recv_pkt.data, RECV_SP_BUF_SIZE);
		play_index++;

		if(play_index == PLAY_PACKET_NUM)
			play_index = 0;

		if(play_index == 0){
			while(sp_get_free_tx_page() == 0){
				vTaskDelay(1);
			}

			if(first_play_packet==0){
				gpio_write(&i2s_intr, 1);
				gpio_write(&i2s_intr, 0);
				first_play_packet= 1;
			}
		
			sp_write_tx_page((u8 *)play_buf, SP_DMA_PAGE_SIZE);
		}
		free(recv_pkt.data);
	}
exit:
	vTaskDelete(NULL);
}

void audio_play_init(void)
{
	sp_obj.sample_rate = SR_48K;
	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo = CH_STEREO;
#ifdef HEADPHONE_MIC
	sp_obj.direction = APP_LINE_OUT | APP_DMIC_IN;
#else
	sp_obj.direction = APP_LINE_OUT;
#endif
	if(xTaskCreate(example_audio_dac_thread, ((const char*)"example_audio_dac_thread"), 512, (void *)(&sp_obj), tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_dac_thread) failed", __FUNCTION__);
}



static u32 udp_server_stop = 0;
static xSemaphoreHandle udp_server_rfoff_sema = NULL;
static u32 rf_status = 1;
static u32 burststart_g = 0;
static u32 burstperiod_g = 0;

TimerHandle_t		recv_timeout_timer;

static void udp_server_timeout_handler(void)
{
	portBASE_TYPE taskWoken = pdFALSE;

#ifdef RFOFF_USER_TIMER
	if (rf_status == 1) {
		//u32 current_tsf = HAL_READ32(WIFI_REG_BASE, 0x560);
		u32 current_tsf = wifi_get_tsf_low(0);
		int offtime = burstperiod_g - (current_tsf - burststart_g) / 1000; //us->ms

		if (offtime > MIN_RFOFF_TIME) {
			wifi_rf_contrl(OFF);
			rf_status = 0;
			udp_server_timeout_start(offtime);
		}
	} else {
		wifi_rf_contrl(ON);
		rf_status = 1;
	}
#else
	xSemaphoreGiveFromISR(udp_server_rfoff_sema, &taskWoken);
	portEND_SWITCHING_ISR(taskWoken);
#endif
}

int udp_server_timeout_start(size_t delay)
{
	int rs = FALSE;

	do {
		if (xTimerChangePeriod(recv_timeout_timer, delay / portTICK_PERIOD_MS, 500) == pdFALSE) {
			DBG_8195A("Change the period failed");
			break;
		}

		// Start the timer.  No block time is specified, and even if one was
		// it would be ignored because the scheduler has not yet been
		// started.
		if (xTimerStart(recv_timeout_timer, 500) != pdPASS) {
			// The timer could not be set into the Active state.
			DBG_8195A("Failed to start timer");
			break;
		}

		rs = TRUE;
	} while (0);

	return rs;
}

void udp_server_rfoff_thread(void)
{
	u32 current_tsf = 0;
	int offtime = 0; //us->ms

    do{
        xSemaphoreTake(udp_server_rfoff_sema, 0xFFFFFFFF);

		if (udp_server_stop) {
			break;
		}
		
		//current_tsf = HAL_READ32(WIFI_REG_BASE, 0x560);
		current_tsf = wifi_get_tsf_low(0);
		offtime = burstperiod_g - (current_tsf - burststart_g) / 1000; //us->ms

		if (offtime > MIN_RFOFF_TIME) {
			wifi_rf_contrl(OFF);
			vTaskDelay(offtime);
			wifi_rf_contrl(ON);
		}

    }while(1);    
}

int udp_server_timeout_init(void)
{
	recv_timeout_timer = xTimerCreate("recv_timeout_timer",   	// Just a text name, not used by the kernel.
		(0x7fffffff),		// The timer period in ticks.
		recv_timeout_timer,	// The timers will auto-reload themselves when they expire.
		NULL,  			// Assign each timer a unique id equal to its array index.
		udp_server_timeout_handler	// Each timer calls the same callback when it expires.
		);
	
	if (recv_timeout_timer == NULL) {
		DBG_8195A("Timer Create Failed!!!");
		//assert_param(0);
	}

	udp_server_stop = 0;
	vSemaphoreCreateBinary(udp_server_rfoff_sema);
	xSemaphoreTake(udp_server_rfoff_sema, 1/portTICK_RATE_MS);

#ifndef RFOFF_USER_TIMER
	if (pdTRUE != xTaskCreate(udp_server_rfoff_thread, (const char * const)"udp_server_rfoff", 64, 
		NULL, tskIDLE_PRIORITY + 6, NULL))
	{
		DBG_8195A("Create udp_server_rfoff Err!!\n");
	}
#endif
}

int udp_server_timeout_deinit(void)
{
	xTimerStop(recv_timeout_timer, 5000);
	xTimerDelete(recv_timeout_timer, 5000);

	udp_server_stop = 1;
	xSemaphoreGive(udp_server_rfoff_sema);
	vSemaphoreDelete(udp_server_rfoff_sema);
}

static void udp_server_burststop(u32 dataseq, u32 burststart)
{
	u32 burstsize = 0;
	int offtime = 0;
	u32 current_time = 0;
	u32 burstperiod = 0;
	u32 lastpktcheck = 0;
	
	burstsize = BURST_SIZE;
	burstperiod = BURST_PERIOD;

	/* burst stop */
	lastpktcheck = burstsize - dataseq;

	/* last 1 packet recvd */
	if (lastpktcheck == 1) {
		current_time = wifi_get_tsf_low(0);
		if(current_time < burststart) {
			//DBG_8195A("timer_sync ing no tdma\n");
			return -1;
		}
		offtime = burstperiod - (current_time - burststart) / 1000; //us->ms
		//DBG_8195A("R:%d (%d)\n", offtime, client_hdr.burstindex);
		if (offtime > MIN_RFOFF_TIME) {			
			xTimerStop(recv_timeout_timer, 5000);
			wifi_rf_contrl(OFF);
			vTaskDelay(offtime);
			wifi_rf_contrl(ON);
		}
	}

	/* last 2 packet, if last packet lost */
	if ((lastpktcheck == 2) || (lastpktcheck == 3)) {
		burststart_g = burststart;
		burstperiod_g = burstperiod;
		udp_server_timeout_start(5);
	}
	
	return 0;

}

_sema udp_burststop_sema = NULL;
u32 burst_dataseq;
u32 burst_burststart;

static void udp_burststop_thread(void *param)
{
	while(1){
		if(xSemaphoreTake(udp_burststop_sema, 0xFFFFFFFF) != pdTRUE) continue;
		udp_server_burststop(burst_dataseq, burst_burststart);
	}
	vTaskDelete(NULL);
}


static char udp_recv_buf_bk[RECV_SP_BUF_SIZE];
static void udp_server_for_headphones_thread(void *param)
{
	struct sockaddr_in	 ser_addr, client_addr;
	int	addrlen = sizeof(struct sockaddr_in);
	int server_fd;
	int n = 1;
	int recv_size = 0;
	recv_pkt_t recv_pkt;
#ifdef HEADPHONE_MIC
	recv_pkt_t mic_pkt;
#endif
	//wait for first packet to start
	static int play_start_num = 0;
	u32	dataseq;
	u32 burststart;
	u32 last_burst_time = 0;
	u32 last_recv_pkt_num = 0;
	u32 recv_pkt_num = 0;
	int i;

#ifdef HEADPHONE_MIC
	u8 tos_value = 96;
	int mic_client_fd;
	struct sockaddr_in	 mic_client_addr;
#endif
	udp_server_timeout_init();

	if((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}

	//Set the reuse the addr when close socket
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &n, sizeof( n ) );

	//initialize structure dest
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(HEADPHONES_SERVER_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// binding the TCP socket to the TCP server address
	if( bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0) {
		printf("\n\r[ERROR] %s: Bind socket failed",__func__);
		close(server_fd);
		return;
	}
	printf("\n\r%s: Bind socket successfully",__func__);
#ifdef HEADPHONE_MIC
//create socket for mic data send
	if((mic_client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}
	memset(&mic_client_addr, 0, sizeof(mic_client_addr));
	mic_client_addr.sin_family = AF_INET;
	mic_client_addr.sin_port = htons(HEADPHONES_CLIENT_PORT);
	mic_client_addr.sin_addr.s_addr = inet_addr((char const*)"192.168.1.1");  //should changed
	lwip_setsockopt(mic_client_fd,IPPROTO_IP,IP_TOS,&tos_value,sizeof(tos_value));
#endif
	while(1){
			recv_size = recvfrom(server_fd, udp_recv_buf, RECV_BUF_SIZE,0,(struct sockaddr *) &client_addr,(u32_t*)&addrlen);

			//Add wifi ps control

			//end of the wifi ps control
			if(recv_size == RECV_BUF_SIZE ){					
				memcpy(&dataseq, &udp_recv_buf[RECV_SP_BUF_SIZE], 4);
				memcpy(&burststart, &udp_recv_buf[RECV_SP_BUF_SIZE + 4], 4);
				if(first_receive_packet==0)
				{
					gpio_write(&i2s_intr, 1);
					gpio_write(&i2s_intr, 0);
					first_receive_packet= 1;
				}
				//udp_server_burststop(dataseq, burststart);
				{
					burst_dataseq = dataseq;
					burst_burststart = burststart;
					xSemaphoreGive(udp_burststop_sema);
				}
				
				recv_pkt.data = (uint8_t *) malloc (RECV_SP_BUF_SIZE);
				recv_pkt.data_len = RECV_SP_BUF_SIZE;
			
				while(recv_pkt.data==NULL){
					vTaskDelay(100);
					printf("Memory Alloc Failed\n\r");
					recv_pkt.data = (uint8_t*)malloc(RECV_SP_BUF_SIZE);
				}
				
				memcpy(recv_pkt.data, udp_recv_buf, RECV_SP_BUF_SIZE);				
				xQueueSend( recv_pkt_queue, ( void * ) &recv_pkt, portMAX_DELAY );
		
				 if((last_burst_time != 0) && (burststart != last_burst_time)){
					if(last_recv_pkt_num != BURST_SIZE){
						last_recv_pkt_num = recv_pkt_num;
						for(i = last_recv_pkt_num; i < BURST_SIZE; i++){
							recv_pkt.data = (uint8_t *) malloc (RECV_SP_BUF_SIZE);
							recv_pkt.data_len = RECV_SP_BUF_SIZE;
							memcpy(recv_pkt.data,udp_recv_buf, RECV_SP_BUF_SIZE);
							xQueueSend( recv_pkt_queue, ( void * ) &recv_pkt, portMAX_DELAY );
							if(play_start == 0){
								play_start_num++;
								
							}
							//printf("%d %d %d\n", play_start_num, last_burst_time, burststart);
						}
					}
					recv_pkt_num = 0;
					last_recv_pkt_num = 0;
				}

				recv_pkt_num++;

				if(dataseq == (BURST_SIZE - 1)){
					if(recv_pkt_num != BURST_SIZE){
						for(i = recv_pkt_num; i < BURST_SIZE; i++){
							recv_pkt.data = (uint8_t *) malloc (RECV_SP_BUF_SIZE);
							recv_pkt.data_len = RECV_SP_BUF_SIZE;
							memcpy(recv_pkt.data,udp_recv_buf, RECV_SP_BUF_SIZE);
							xQueueSend( recv_pkt_queue, ( void * ) &recv_pkt, portMAX_DELAY );
							if(play_start == 0){
								play_start_num++;
								
							}
							//printf("%d, %d %d\n",play_start_num, i, burststart);
						}
					}
					last_recv_pkt_num = BURST_SIZE;
				}

				last_burst_time = burststart;

				if(play_start == 0){
					play_start_num++;
					if(play_start_num >= PACKET_BUFFER_SIZE){
						xSemaphoreGive(packet_buffer_sema);
						play_start = 1;
					}
					//printf("%d, %d %d\n", play_start_num, dataseq, burststart);
				}

#ifdef HEADPHONE_MIC
				//Send mic packet to dongle

#if 1
				if (xQueueReceive( mic_pkt_queue, &mic_pkt, 0 ) == pdTRUE) {
					#if 1	
					if((sendto(mic_client_fd, mic_pkt.data, mic_pkt.data_len,0,
										(struct sockaddr*)&mic_client_addr, sizeof(struct sockaddr_in))) < 0) {
						printf("error %d\n", lwip_getsocklasterr((long)mic_client_fd));	
					}
					#endif
					free(mic_pkt.data);
        		}
#endif
#endif
			}
	}
	
	if(recv_pkt.data!= NULL)
		free(recv_pkt.data);
	close(server_fd);
	vTaskDelete(NULL);
}
#ifdef HEADPHONE_MIC
#if 0
#else
#define MIC_RESAMPLE	1
RESAMPLE_STAT48TO16 state;
//RESAMPLE_STAT	state;

int resample_temp[SP_DMA_PAGE_SIZE/2 + 16];
static void mic_receive_thread(void* param)
{
	int ret;
	recv_pkt_t mic_pkt;
	int static count = 0;
	
	ResampleReset48khzTo16khz(&state);
	while(1){
		if(xSemaphoreTake(mic_rx_sema, 0xFFFFFFFF) != pdTRUE) continue;

#ifdef MIC_RESAMPLE
			memset(mic_resample_buf, 0, RECORD_RESAMPLE_SIZE);
//			Resample48khzTo16khz(&mic_recv_pkt_buff[count*SP_DMA_PAGE_SIZE], SP_DMA_PAGE_SIZE/2, mic_resample_buf);
			Resample48khzTo16khz(&mic_recv_pkt_buff[count*SP_DMA_PAGE_SIZE], SP_DMA_PAGE_SIZE/2, mic_resample_buf, &state, resample_temp);
			count++;
			count = count % 2;
			mic_pkt.data = (uint8_t *) malloc (RECORD_SEND_PKT_SIZE);
			mic_pkt.data_len = RECORD_SEND_PKT_SIZE;
	
			while(mic_pkt.data==NULL){
				vTaskDelay(100);
				printf("Memory Alloc Failed\n\r");
				mic_pkt.data = (uint8_t*)malloc(RECORD_SEND_PKT_SIZE);
			}
	
			memcpy(mic_pkt.data, mic_resample_buf, RECORD_SEND_PKT_SIZE);
#else
			mic_pkt.data = (uint8_t *) malloc (SP_DMA_PAGE_SIZE);
			mic_pkt.data_len = SP_DMA_PAGE_SIZE;
	
			while(mic_pkt.data==NULL){
				vTaskDelay(100);
				printf("Memory Alloc Failed\n\r");
				mic_pkt.data = (uint8_t*)malloc(SP_DMA_PAGE_SIZE);
			}
	
			memcpy(mic_pkt.data, &mic_recv_pkt_buff[count*SP_DMA_PAGE_SIZE], SP_DMA_PAGE_SIZE);
			count++;
			count = count % 2;

#endif
			xQueueSend(mic_pkt_queue, ( void * ) &mic_pkt, portMAX_DELAY );
	}
	
	vTaskDelete(NULL);
}
#endif
#endif
void example_competitive_headphones_init(void *param)
{
//TODO
#if 0
	int timeout = 20;
	while(timeout > 0) {
		if(rltk_wlan_running(0)) {
			break;
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
		timeout --;
	}

	wifi_connect_dongle();

	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS){ 
		printf("\r\n\r\n\r\n>>>>>>>>>>>>>>Wifi is disconnected!!Please connect!!<<<<<<<<<<<<<<<<<\r\n\r\n\r\n");
		vTaskDelay(10000);
	}
#endif		

	//Create udp_server_thread
	//init should in one function
	rtw_init_sema(&packet_buffer_sema, 0);
	recv_pkt_queue = xQueueCreate(RECV_PKT_QUEUE_LENGTH, sizeof(recv_pkt_t));
#ifdef HEADPHONE_MIC
	mic_rx_sema = xSemaphoreCreateCounting(0xffffffff, 0);	
	mic_pkt_queue = xQueueCreate(MIC_PKT_QUEUE_LENGTH, sizeof(recv_pkt_t));
#endif
	gpio_init(&i2s_intr, PA_10);
    gpio_dir(&i2s_intr, PIN_OUTPUT);
    gpio_mode(&i2s_intr, PullNone);


	if(xTaskCreate(udp_server_for_headphones_thread, ((const char*)"udp_server_for_headphones_thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(udp_server_for_headphones_thread) failed", __FUNCTION__);

#ifdef HEADPHONE_MIC
	if(xTaskCreate(mic_receive_thread, ((const char*)"mic_receive_thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(mic_receive_thread) failed", __FUNCTION__);
#endif
	audio_play_init();
	

	rtw_init_sema(&udp_burststop_sema, 0);
	if(xTaskCreate(udp_burststop_thread, ((const char*)"udp_burststop_thread"), 
				1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(udp_burststop_thread) failed", __FUNCTION__);


	

	vTaskDelete(NULL);
}

void example_competitive_headphones()
{
	if(xTaskCreate(example_competitive_headphones_init, ((const char*)"headphones example init"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(headphones example thread) failed", __FUNCTION__);

}
#endif

