#include <platform_opts.h>

#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>
#include "wifi_structures.h"
#include "i2s_api.h"
#include "freertos_service.h"
#include "example_competitive_headphones_dongle.h"
#include "resample_library.h"

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM]; 
#endif

xSemaphoreHandle i2s_rx_sema = NULL;
i2s_t i2s_obj;

SRAM_NOCACHE_DATA_SECTION static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
SRAM_NOCACHE_DATA_SECTION static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

u8 recv_buf[I2S_DMA_PAGE_SIZE*RECV_BUF_NUM];

static int count = 0;
gpio_t i2s_intr;
int first_packet = 0;

static void test_tx_complete(void *data, char *pbuf)
{    
	return ;
}

static void test_rx_complete(void *data, char* pbuf)
{
	portBASE_TYPE taskWoken = pdFALSE;
	if(first_packet == 0){
		gpio_write(&i2s_intr, 1);
		gpio_write(&i2s_intr, 0);
		first_packet = 1;
	}

	memcpy((void*)&recv_buf[I2S_DMA_PAGE_SIZE*count], (void*)pbuf, I2S_DMA_PAGE_SIZE);
	count++;
	if(count == RECV_BUF_NUM)
		count = 0;

	i2s_recv_page(&i2s_obj);


    xSemaphoreGiveFromISR(i2s_rx_sema, &taskWoken);
    portEND_SWITCHING_ISR(taskWoken);
}

static void audio_data_recv_init(void *param)
{
	// I2S init
	i2s_obj.channel_num = I2S_CH_STEREO;
	i2s_obj.sampling_rate = SR_48KHZ;
	i2s_obj.word_length = WL_16b;
#ifdef RECORD_SUPPORT
	i2s_obj.direction = I2S_DIR_TXRX;   
#else
	i2s_obj.direction = I2S_DIR_RX; 
#endif

	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_TX_PIN, I2S_SD_RX_PIN, I2S_MCK_PIN);
#ifdef RECORD_SUPPORT
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, \
		I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
#else
	i2s_set_dma_buffer(&i2s_obj, NULL, (char*)i2s_rx_buf, \
		I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
#endif
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)test_rx_complete, (uint32_t)&i2s_obj);
#ifdef RECORD_SUPPORT
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)test_tx_complete, (uint32_t)&i2s_obj);
	i2s_send_page(&i2s_obj, (uint32_t*)i2s_get_tx_page(&i2s_obj));
#endif
	i2s_recv_page(&i2s_obj);
}

char send_buf[SEND_PKT_SIZE];
char headphone_ip_addr[20];
static void udp_client_for_dongle_thread(void *param)
{
	static int out_idx = 0;
	int i;
	int num = 0;
	u8 tos_value = 96;
	int i2s_packet_num;
	int last_err;
	int sockfd;
	int ret;
	u32 dataseq;
	u32 burststart;
	struct sockaddr_in ser_addr;

	u32 first_send = 0;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(DONGLE_CLINET_PORT);
	ser_addr.sin_addr.s_addr = inet_addr((char const*)headphone_ip_addr);  //should changed

	lwip_setsockopt(sockfd,IPPROTO_IP,IP_TOS,&tos_value,sizeof(tos_value));

	printf("connect to the server addr: 0x%x, port: %d\n\r", ser_addr.sin_addr.s_addr, ser_addr.sin_port);

	while(1){	
		if(xSemaphoreTake(i2s_rx_sema, portMAX_DELAY) != pdTRUE) continue;
		
		if(count >= out_idx){
			i2s_packet_num = count - out_idx; 
		}else{
			i2s_packet_num = count + RECV_BUF_NUM - out_idx;
		}

		if(i2s_packet_num >= RECV_PAGE_NUM){
			dataseq = 0;
			burststart= wifi_get_tsf_low(0);
			if(first_send == 0){
				printf("first %d\n", burststart);
				first_send = 1;
			}
			for(i=0; i<RECV_PAGE_NUM; i++){
				memcpy(send_buf, &recv_buf[I2S_DMA_PAGE_SIZE * out_idx], I2S_DMA_PAGE_SIZE);
				
				memcpy(&send_buf[I2S_DMA_PAGE_SIZE], &dataseq, 4);
				memcpy(&send_buf[I2S_DMA_PAGE_SIZE + 4], &burststart, 4);
				if((ret = sendto(sockfd, send_buf, SEND_PKT_SIZE,0,
									(struct sockaddr*)&ser_addr, sizeof(struct sockaddr_in))) < 0) {
					last_err = lwip_getsocklasterr((long)sockfd);	
				}

				out_idx++;
				out_idx = out_idx % RECV_BUF_NUM;
				dataseq++;
			}	
		}
	}

	close(sockfd);
	vTaskDelete(NULL);
}

static u8 udp_dongle_recv_buf[DONGLE_RECV_BUF_SIZE];
static void udp_server_for_dongle_thread(void *param)
{
	int sockfd;
	int i;
	int *ptx_buf;
	int recv_size = 0;
	struct sockaddr_in	 ser_addr, client_addr;
	int	addrlen = sizeof(struct sockaddr_in);
	RESAMPLE_STAT16TO48 state;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(DONGLE_SERVER_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //should changed

		// binding the TCP socket to the TCP server address
	if( bind(sockfd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0) {
		printf("\n\r[ERROR] %s: Bind socket failed",__func__);
		close(sockfd);
		return;
	}

	ResampleReset16khzTo48khz(&state);

	u8 voice_resample[I2S_DMA_PAGE_SIZE];
	int temp[DONGLE_RECV_BUF_SIZE/BURST_SIZE + 16];

	memset(voice_resample, 0, I2S_DMA_PAGE_SIZE);
	memset(temp, 0, sizeof(temp));
	
	while(1){	
		#if 1
		recv_size = recvfrom(sockfd, udp_dongle_recv_buf, DONGLE_RECV_BUF_SIZE,0,
							(struct sockaddr *) &client_addr,(u32_t*)&addrlen);
		
		for(i = 0; i < BURST_SIZE; i++){
			ptx_buf = i2s_get_tx_page(&i2s_obj);
			while(ptx_buf == NULL){
				vTaskDelay(10);
				ptx_buf = i2s_get_tx_page(&i2s_obj);
			}
#ifdef MIC_RESAMPLE
			memset(voice_resample, 0, I2S_DMA_PAGE_SIZE);
			Resample16khzTo48khz(&udp_dongle_recv_buf[i*DONGLE_RECV_BUF_SIZE/BURST_SIZE], 
								DONGLE_RECV_BUF_SIZE/BURST_SIZE/2, voice_resample, &state, temp);
			
			_memcpy((void*)ptx_buf, voice_resample, I2S_DMA_PAGE_SIZE);
#else
			_memcpy((void*)ptx_buf, &udp_dongle_recv_buf[i*I2S_DMA_PAGE_SIZE], I2S_DMA_PAGE_SIZE);

#endif
			i2s_send_page(&i2s_obj, ptx_buf);
		}
		#endif
		
	}

	close(sockfd);
	vTaskDelete(NULL);

}
void example_headphones_dongle_thread(void *param)
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

//	audio_data_recv();
	softap_for_dongle();
#endif
	//Set softap Wifi Retry time to 0x4
	u8 RetryLimit = 0x4;
	u16 val16 = (RetryLimit << 8) | (RetryLimit << 0);
	HAL_WRITE16(WIFI_REG_BASE, 0x042A, val16);

    gpio_init(&i2s_intr, PA_13);
    gpio_dir(&i2s_intr, PIN_OUTPUT);
    gpio_mode(&i2s_intr, PullNone);

	i2s_rx_sema = xSemaphoreCreateCounting(0xffffffff, 0);	

	if(xTaskCreate(udp_client_for_dongle_thread, ((const char*)"dongle client"), 
					(256+1024), NULL, tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET, NULL) != pdPASS){
		printf("udp_client_thread_for_dongle thread create fail\n\r");
	}

#ifdef RECORD_SUPPORT
	if(xTaskCreate(udp_server_for_dongle_thread, ((const char*)"dongle server"), 
					(256 + 1024), NULL, tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET, NULL) != pdPASS){
		printf("udp_server_thread_for_dongle thread create fail\n\r");
	}
#endif

	audio_data_recv_init(NULL);
	vTaskDelete(NULL);
}

void example_competitive_headphones_dongle()
{
	if(xTaskCreate(example_headphones_dongle_thread, ((const char*)"headphones example thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(headphones example thread) failed", __FUNCTION__);
}
#endif

