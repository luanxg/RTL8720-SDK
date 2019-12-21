#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>

#include "queue.h"
#include "duerapp.h"
#include "duerapp_config.h"
#include "duerapp_audio.h"
#include "FreeRTOS.h"
#include "task.h"


QueueHandle_t sdcard_queue;

struct sdcard_message
{
    char type;
    void* pbuf;
    size_t len;
};

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

static FIL	 sdcard_m_file;
static int  sdcard_file_index = 0;
static int  sdcard_process = 0;

#if SDCARD_RECORD
int duer_sdcard_send(int type, const void *data, size_t size)
{
    int rs = 0;
    void *buff = NULL;
    do {

        if(size >0 && type == 1){
            buff = (void*)malloc(size);
            if (!buff) {
                DUER_LOGE("Alloc temp memory failed!");
                break;
            }
            memcpy(buff, data, size);
        }

        struct sdcard_message message;
        message.type = type;
        message.pbuf = buff;
        message.len = size;
        if(pdPASS != xQueueSendFromISR(sdcard_queue, &message, 0)){
            DUER_LOGE("Could not send to type %d scard queue", type);
        }
    } while (0);
    return rs;
}

void sdcard_record_thread(void *param)
{
    FRESULT res;
    int bw;
    char path[64];
    WAVE_HEADER w_header;
    char wav_header[sizeof(WAVE_HEADER)];
    struct sdcard_message message;

    sdcard_queue = xQueueCreate(150, sizeof(struct sdcard_message));
    if (sdcard_queue == NULL) {
        DUER_LOGE("Create the sdcard queue failed!");
        goto Exit;
    }

    memset(&w_header, 0, sizeof(WAVE_HEADER));
    strcpy(w_header.w_header.fccID, "RIFF");
    strcpy(w_header.w_header.fccType, "WAVE");
    strcpy(w_header.w_fmt.fccID, "fmt ");
    w_header.w_fmt.dwSize=16;
    w_header.w_fmt.wFormatTag=1;
    w_header.w_fmt.wChannels=1;
    w_header.w_fmt.dwSamplesPerSec=16000;
    w_header.w_fmt.dwAvgBytesPerSec=w_header.w_fmt.dwSamplesPerSec*2;
    w_header.w_fmt.wBlockAlign=2;
    w_header.w_fmt.uiBitsPerSample=16;
    strcpy(w_header.w_data.fccID, "data");

    memcpy(wav_header, &w_header, sizeof(WAVE_HEADER));

    do {
        if (pdTRUE == xQueueReceive(sdcard_queue, &message, portMAX_DELAY)) {
            if(message.type == 0 && sdcard_process ==0)
            {
                initialize_sdcard();
                memset(path, 0, 64);
                printf("\r\ntype 0, len %d, buf 0x%x\r\n", message.len, message.pbuf);
                strcpy(path, SDCARD_PATH);
                sprintf(&path[strlen(path)],"record%d.wav",sdcard_file_index);
                sdcard_file_index++;

                res = f_open(&sdcard_m_file, path, FA_OPEN_ALWAYS | FA_WRITE);
                if(res){
                    DUER_LOGE("\r\nopen file (%s) fail.\r\n", path);
                }
                //f_lseek(&sdcard_m_file, 0);
                DUER_LOGI("\r\nRecord file name:%s\r\n",path);
                sdcard_process = 1;

                res = f_write(&sdcard_m_file, wav_header, sizeof(WAVE_HEADER), (u32*)&bw);
                if(res){
                    f_lseek(&sdcard_m_file, 0);
                    DUER_LOGE("\r\nSDCard Header Write error.\n");
                }

            }
            else if(message.type == 1 && sdcard_process == 1)
            {
                res = f_write(&sdcard_m_file, message.pbuf, message.len, (u32*)&bw);
                if(res){
                    f_lseek(&sdcard_m_file, 0);
                    DUER_LOGE("\r\nSDCard Data Write error.\n");
                }
                free(message.pbuf);
            }
            else if(message.type == 2 && sdcard_process == 1)
            {
               // w_header.w_data.dwSize=36+message.len;
                w_header.w_header.dwSize = message.len + sizeof(WAVE_HEADER) - 8;
                w_header.w_data.dwSize = message.len; 
                f_lseek(&sdcard_m_file, 0);
                res = f_write(&sdcard_m_file, &w_header, sizeof(WAVE_HEADER), (u32*)&bw);
                if(res){
                    f_lseek(&sdcard_m_file, 0);
                    DUER_LOGE("\r\nSDCard End Write error.\n");
                }

                res = f_close(&sdcard_m_file);
                if(res){
                    DUER_LOGE("close file (%s) fail.\n", path);
                    sdcard_process = 0;
                }else{
                    DUER_LOGD("close file (%s) success.\n", path);
                    sdcard_process = 0;
                }
                finalize_sdcard();
            }

        } else {
            DUER_LOGE("Queue receive failed!");
        }
    } while (1);

Exit:
    vTaskDelete(NULL);
}

static u8 sdcard_record_start = 0;
static u8 sdcard_record_stop = 0;
static u8 sdcard_record_in_process = 0;
int sdcard_record_len = 0;

void sdcard_recording_start()
{
    if(sdcard_record_in_process == 1){
        printf("Recording in process, stop first\n");
        return;
    }else{
        printf("Start to Record...\n");
        sdcard_record_start = 1;
    }
}

void sdcard_recording_stop()
{
    if(sdcard_record_in_process == 0){
        printf("already stopped\n");
        return;
    }else{
        printf("Stop to Record\n");
        sdcard_record_stop = 1;
    }
}


void audio_recoder_sdcard_rx_complete(void* buf, u32 len)
{
	if(sdcard_record_stop){
		duerapp_gpio_led_mode(DUER_LED_OFF);
		duer_sdcard_send(REC_STOP, NULL, sdcard_record_len);
		//printf("\r\nsend stop\r\n");

		sdcard_record_start = 0;
		sdcard_record_stop = 0;
		sdcard_record_in_process = 0;
		sdcard_record_len = 0;
	}

	if(sdcard_record_start){
		duer_sdcard_send(REC_START, NULL, 0);
		//printf("\r\nsend start\r\n");
		duerapp_gpio_led_mode(DUER_LED_ON);
		sdcard_record_in_process = 1;
		sdcard_record_start = 0;
	}

	if(sdcard_record_in_process){
		sdcard_record_len += len;
		//printf("\r\nsend data\r\n");
		duer_sdcard_send(REC_DATA, buf, len);
	}
}
#endif

void initialize_sdcard_record()
{
#if SDCARD_RECORD
    if(xTaskCreate(sdcard_record_thread, ((const char*)"sdcard_record_thread"), 3072, NULL, 6, NULL) != pdPASS)
        DUER_LOGE("Create example sdcard_record_thread failed!");
#endif
}

#ifdef CONFIG_PLATFORM_8195A

extern unsigned char i2s_comm_buf[AUDIO_MAX_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
extern i2s_t i2s_obj;
static void i2s_recoder_sdcard_rx_complete(void *data, char* pbuf)
{
    i2s_t *obj = (i2s_t *)data;

    //printf("ddd\r\n");
    if(sdcard_record_stop){
        duerapp_gpio_led_mode(DUER_LED_OFF);
        duer_sdcard_send(REC_STOP, NULL, sdcard_record_len);
        //printf("\r\nsend stop\r\n");

        sdcard_record_start = 0;
        sdcard_record_stop = 0;
        sdcard_record_in_process = 0;
        sdcard_record_len = 0;
    }

    if(sdcard_record_start){
        duer_sdcard_send(REC_START, NULL, 0);
        //printf("\r\nsend start\r\n");
        duerapp_gpio_led_mode(DUER_LED_ON);
        sdcard_record_in_process = 1;
        sdcard_record_start = 0;
    }


    if(sdcard_record_in_process){
        sdcard_record_len += I2S_RX_PAGE_SIZE;
        //printf("\r\nsend data\r\n");
        i2s_recv_page(obj);

        duer_sdcard_send(REC_DATA, pbuf, I2S_RX_PAGE_SIZE);
    }

}

void example_pcm_record(char* path)
{
#if SDCARD_RECORD
    initialize_sdcard_record();

    DUER_LOGI("Initializing audio as recorder....\n");

    i2s_obj.channel_num = I2S_CH_MONO;
    i2s_obj.sampling_rate = SR_16KHZ;
    i2s_obj.word_length = WL_16b;
    i2s_obj.direction = I2S_DIR_RX;
    i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
    pDuerAppInfo->i2s_has_inited = 1;
    i2s_set_dma_buffer(&i2s_obj, NULL, (char*)i2s_comm_buf, I2S_DMA_PAGE_NUM, I2S_RX_PAGE_SIZE);
    i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_recoder_sdcard_rx_complete, (uint32_t)&i2s_obj);
    //i2s_recv_page(&i2s_obj);

    DUER_LOGI("Audio has initialized as recorder.\n");
#endif
}
#endif

