#include "FreeRTOS.h"
#include "task.h"
#include <platform_stdlib.h>

#include "device.h"
#include "diag.h"
#include "main.h"
#include "section_config.h"
#include "osdep_service.h"
#include "dma_api.h"
#include "wifi_conf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"

#include "i2s_api.h"
#include "tftp/tftp.h"

//gpio
#include "gpio_api.h"   // mbed
#include "gpio_irq_api.h"   // mbed
#define GPIO_IRQ_PIN            PC_5
//

static i2s_t i2s_obj;

#define I2S_DMA_PAGE_SIZE	768   // 2 ~ 4096
#define I2S_DMA_PAGE_NUM    4   // Vaild number is 2~4

static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

#define I2S_SCLK_PIN            PC_1
#define I2S_WS_PIN              PC_0
#define I2S_SD_PIN              PC_2

typedef   struct  
{
    char    fccID[4];     
    unsigned   long  dwSize;       
    unsigned   short    wFormatTag;    
    unsigned   short    wChannels;    
    unsigned   long     dwSamplesPerSec;  
    unsigned   long     dwAvgBytesPerSec;  
    unsigned   short    wBlockAlign;   
    unsigned   short    uiBitsPerSample;  
}WAVE_FMT; //Format Chunk

typedef   struct 
{
    char    fccID[4];    
    unsigned   long     dwSize;     
}WAVE_DATA; //Data Chunk

typedef   struct  
{
    char     fccID[4];    
    unsigned   long      dwSize;    
    char     fccType[4];   
}WAVE_HEAD; //RIFF WAVE Chunk

typedef struct{
    WAVE_HEAD w_header;
    WAVE_FMT w_fmt;
    WAVE_DATA w_data;
}WAVE_HEADER;



#define SDRAM_BSS_SECTION                        \
        SECTION(".sdram.bss")

#define RECORD_LEN (1024*1024)  //for record
//#define RECORD_LEN (1024*200)//for check data
unsigned int record_length = 0;
//SDRAM_DATA_SECTION unsigned char record_data[RECORD_LEN]={0};
SDRAM_BSS_SECTION unsigned char record_data[RECORD_LEN];

#define WRITE_FREE      0
#define WRITE_LOCK      1

static unsigned char audio_status = 0;
static unsigned char write_status = 0;
static unsigned char wifi_state = 0;
static unsigned char audio_start = 0;

#define RECORD_WAV_NAME "AMEBA"
#define TFTP_HOST_IP_ADDR "192.168.1.100"
#define TFTP_HOST_PORT  69
#define TFTP_MODE       "octet"

_sema       VOICE_TRIGGER_SEMA;//INFORM I2S TO RECORD DATA
_sema       VOICE_UPLOAD_SEMA;//INFORM TFTP TO UPLOAD DATA

#define INTERNAL_TEST 0

#if INTERNAL_TEST
extern char upgrade_ip[64];
#endif

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
    if(wifi_state == 1 && write_status == WRITE_FREE){
        audio_start = 1;
        rtw_up_sema_from_isr(&VOICE_TRIGGER_SEMA);
    }
    
    printf("voice irq = %d\n",audio_start);
}

static void test_tx_complete(void *data, char *pbuf)
{    
    return ;
}

static void test_rx_complete(void *data, char* pbuf)
{
    i2s_t *obj = (i2s_t *)data;
    int *ptx_buf;

    if((audio_start) == 1 && (write_status == WRITE_FREE)){     
         if((record_length+I2S_DMA_PAGE_SIZE)<RECORD_LEN){
           if(record_length == 0){
              _memcpy((void*)(record_data+record_length+sizeof(WAVE_HEADER)),(void*)pbuf,I2S_DMA_PAGE_SIZE);
              record_length+=(I2S_DMA_PAGE_SIZE+sizeof(WAVE_HEADER));
           }else{
              _memcpy((void*)(record_data+record_length),(void*)pbuf,I2S_DMA_PAGE_SIZE);
              record_length+=I2S_DMA_PAGE_SIZE;
           }
           i2s_recv_page(obj);
         }else{
           write_status = WRITE_LOCK;
           printf("write length = %x\r\n",record_length);
           rtw_up_sema_from_isr(&VOICE_UPLOAD_SEMA);
         }
    }   
}

void example_audio_init()
{
    printf("I2S initial\r\n");
    i2s_obj.channel_num = I2S_CH_MONO;
    i2s_obj.sampling_rate = SR_48KHZ;
    i2s_obj.word_length = WL_16b;
    i2s_obj.direction = I2S_DIR_TXRX;    
    i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
    i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, \
    I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)test_tx_complete, (uint32_t)&i2s_obj);
    i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)test_rx_complete, (uint32_t)&i2s_obj);
    
    printf("i2s init finish\r\n");

    /* rx need clock, let tx out first */
    i2s_send_page(&i2s_obj, (uint32_t*)i2s_get_tx_page(&i2s_obj));
    //i2s_recv_page(&i2s_obj);
}

void tftp_audio_send_handler(unsigned char *buffer,int *len,unsigned int index)
{
    //static unsigned int total_size = record_length;
    int remain_len = record_length -(512*(index-1));
    if(remain_len/512){
        //memset(buffer,index,512);
        memcpy(buffer,record_data+512*(index-1),512);
        *len = 512;
    }else{
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

void example_audio_transfer_thread(void* param){
    while(1){
        rtw_down_sema(&VOICE_TRIGGER_SEMA);
        i2s_send_page(&i2s_obj, (uint32_t*)i2s_get_tx_page(&i2s_obj));
        i2s_recv_page(&i2s_obj);
        printf("start_record_data\r\n");
    }
}

void example_audio_pcm_upload_thread(void* param){
	
    WAVE_HEADER w_header;
    tftp tftp_info;
    static int tftp_count = 0;
    memset(&tftp_info,0,sizeof(tftp));
    char tftp_filename[64]={0};
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
    //gpio
    gpio_irq_t voice_irq;
    gpio_t voice_pinr;
        
    printf("GPIO_IRQ\r\n");
    
    check_wifi_connection();
        
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

    while(1){
        //vTaskDelay(1000);
        rtw_down_sema(&VOICE_UPLOAD_SEMA);
        if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS ) {
                    wifi_state = 1;
            if((write_status == WRITE_LOCK) && audio_start){
                    printf("wifi connect\r\n");
                    w_header.w_data.dwSize=36+record_length-sizeof(WAVE_HEADER); 
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
        }else{
                    wifi_state = 0;
        }
    }
}

void example_audio_pcm_upload(void)
{
    if(xTaskCreate(example_audio_pcm_upload_thread, ((const char*)"example_audio_pcm_upload_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
            printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}