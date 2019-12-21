#include <platform_opts.h>

#if (CONFIG_EXAMPLE_AUDIO_HELIX_AAC)
#include "example_audio_helix_aac.h"
#include "platform_stdlib.h"
#include "section_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "wifi_conf.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "aacdec.h"

#include "rl6548.h"

#ifndef SDRAM_BSS_SECTION
#define SDRAM_BSS_SECTION                        
 //       SECTION(".psram.bss")
#endif

#define AUDIO_SOURCE_BINARY_ARRAY 1
#define ADUIO_SOURCE_HTTP_FILE    0

#if AUDIO_SOURCE_BINARY_ARRAY
#include "sr48000_br320_stereo.c"
#endif

#if ADUIO_SOURCE_HTTP_FILE
#define AUDIO_SOURCE_HTTP_FILE_HOST         "192.168.1.219"
#define AUDIO_SOURCE_HTTP_FILE_PORT         (80)
#define AUDIO_SOURCE_HTTP_FILE_NAME         "/audio.aac"
#define BUFFER_SIZE                         (1500)

#define AAC_MAX_FRAME_SIZE (1600)
#define AAC_DATA_CACHE_SIZE ( BUFFER_SIZE + 2 * AAC_MAX_FRAME_SIZE )
SDRAM_BSS_SECTION static uint8_t aac_data_cache[AAC_DATA_CACHE_SIZE];
static uint32_t aac_data_cache_len = 0;
#endif

#if ADUIO_SOURCE_HTTP_FILE
#define AUDIO_PKT_BEGIN (1)
#define AUDIO_PKT_DATA  (2)
#define AUDIO_PKT_END   (3)
typedef struct _audio_pkt_t {
    uint8_t type;
    uint8_t *data;
    uint32_t data_len;
} audio_pkt_t;

#define AUDIO_PKT_QUEUE_LENGTH (50)
static xQueueHandle audio_pkt_queue;
#endif


SDRAM_BSS_SECTION static uint8_t sp_tx_buf[ SP_DMA_PAGE_SIZE * SP_DMA_PAGE_NUM ];
//SDRAM_BSS_SECTION static uint8_t sp_rx_buf[ SP_DMA_PAGE_SIZE * SP_DMA_PAGE_NUM ];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

SDRAM_BSS_SECTION static uint8_t decodebuf[ AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int16_t) ];

#define SP_TX_PCM_QUEUE_LENGTH (10)
static xQueueHandle sp_tx_pcm_queue = NULL;

static uint32_t sp_tx_pcm_cache_len = 0;
static int16_t sp_tx_pcm_cache[SP_DMA_PAGE_SIZE/2];

static xSemaphoreHandle sp_tx_done_sema = NULL;


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

static void audio_play_pcm(int16_t *buf, uint32_t len)
{
	u32 offset = 0;
	u32 tx_len;

	len = len*2;		//for int16_t
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

#if AUDIO_SOURCE_BINARY_ARRAY
void audio_play_binary_array(uint8_t *srcbuf, uint32_t len) {
    uint8_t *inbuf;
    int bytesLeft;

    int ret;
    HAACDecoder	hAACDecoder;
    AACFrameInfo frameInfo;

    uint8_t first_frame = 1;
    hAACDecoder = AACInitDecoder();

    inbuf = srcbuf;
    bytesLeft = len;

    sp_tx_done_sema = xSemaphoreCreateBinary();

    while (1) {
        ret = AACDecode(hAACDecoder, &inbuf, &bytesLeft, decodebuf);
        if (!ret) {
            AACGetLastFrameInfo(hAACDecoder, &frameInfo);
            if (first_frame) {
                initialize_audio(frameInfo.nChans, frameInfo.sampRateOut);
                first_frame = 0;
            }
            audio_play_pcm(decodebuf, frameInfo.outputSamps);
        } else {
            printf("error: %d\r\n", ret);
            break;
        }
    }

    printf("decoding finished\r\n");
}
#endif

#if ADUIO_SOURCE_HTTP_FILE
void file_download_thread(void* param)
{
    int n, server_fd = -1;
    struct sockaddr_in server_addr;
    struct hostent *server_host;
    char *buf = NULL;

    audio_pkt_t pkt_data;

    while (wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) vTaskDelay(1000);

    do {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if ( server_fd < 0 ) {
            printf("ERROR: socket\r\n");
            break;
        }

        server_host = gethostbyname(AUDIO_SOURCE_HTTP_FILE_HOST);
        server_addr.sin_port = htons(AUDIO_SOURCE_HTTP_FILE_PORT);
        server_addr.sin_family = AF_INET;
        memcpy((void *) &server_addr.sin_addr, (void *) server_host->h_addr, server_host->h_length);
        if ( connect(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0 ) {
            printf("ERROR: connect\r\n");
            break;
        }

        setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &(int){ 5000 }, sizeof(int));

        buf = (char *) malloc (BUFFER_SIZE);
        if (buf == NULL) {
            printf("ERROR: malloc\r\n");
            break;
        }

		sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", AUDIO_SOURCE_HTTP_FILE_NAME, AUDIO_SOURCE_HTTP_FILE_HOST);
		write(server_fd, buf, strlen(buf));

        audio_pkt_t pkt_begin = { .type = AUDIO_PKT_BEGIN, .data = NULL, .data_len = 0 };
        xQueueSend( audio_pkt_queue, ( void * ) &pkt_begin, portMAX_DELAY );

        n = read(server_fd, buf, BUFFER_SIZE);

        while( (n = read(server_fd, buf, BUFFER_SIZE)) > 0 ) {
            pkt_data.type = AUDIO_PKT_DATA;
            pkt_data.data_len = n;
            pkt_data.data = (uint8_t *) malloc (n);
            while ( pkt_data.data == NULL) {
                vTaskDelay(100);
                pkt_data.data = (uint8_t *) malloc (n);
            }
            memcpy( pkt_data.data, buf, n );
            xQueueSend( audio_pkt_queue, ( void * ) &pkt_data, portMAX_DELAY );
        }

        printf("exit download\r\n");
    } while (0);

    if (buf != NULL) free(buf);

	if(server_fd >= 0) close(server_fd);

    audio_pkt_t pkt_end = { .type = AUDIO_PKT_END, .data = NULL, .data_len = 0 };
    xQueueSend( audio_pkt_queue, ( void * ) &pkt_end, portMAX_DELAY );

    vTaskDelete(NULL);
}

void audio_play_http_file()
{
    audio_pkt_t pkt;

    uint8_t *inbuf;
    int bytesLeft;

    int ret;
    HAACDecoder	hAACDecoder;
    AACFrameInfo frameInfo;

    uint8_t first_frame = 1;

    hAACDecoder = AACInitDecoder();

    while (1) {
        if (xQueueReceive( audio_pkt_queue, &pkt, portMAX_DELAY ) != pdTRUE) {
            continue;
        }

        if (pkt.type == AUDIO_PKT_BEGIN) {
            vTaskDelay(5000); // wait 5 seconds for buffering
        }

        if (pkt.type == AUDIO_PKT_DATA) {
            if (aac_data_cache_len + pkt.data_len >= AAC_DATA_CACHE_SIZE) {
                printf("aac data cache overflow %d %d\r\n", aac_data_cache_len, pkt.data_len);
                free(pkt.data);
                break;
            }

            memcpy( aac_data_cache + aac_data_cache_len, pkt.data, pkt.data_len );
            aac_data_cache_len += pkt.data_len;
            free(pkt.data);

            inbuf = aac_data_cache;
            bytesLeft = aac_data_cache_len;

            ret = 0;
            while (ret == 0) {
                ret = AACDecode(hAACDecoder, &inbuf, &bytesLeft, decodebuf);
                if (ret == 0) {
                    AACGetLastFrameInfo(hAACDecoder, &frameInfo);
                    if (first_frame) {
                        initialize_audio(frameInfo.nChans, frameInfo.sampRateOut);
                        first_frame = 0;
                    }
                    audio_play_pcm(decodebuf, frameInfo.outputSamps);
                } else {
                    if (ret != ERR_AAC_INDATA_UNDERFLOW) {
                        printf("ret:%d\r\n", ret);
                    }
                    break;
                }
            }

            if (bytesLeft > 0) {
                memmove(aac_data_cache, aac_data_cache + aac_data_cache_len - bytesLeft, bytesLeft);
                aac_data_cache_len = bytesLeft;
            } else {
                aac_data_cache_len = 0;
            }
        }

        if (pkt.type == AUDIO_PKT_END) {
        }
    }

    vTaskDelete(NULL);
}
#endif

void example_audio_helix_aac_thread(void* param)
{

	printf("Audio codec haac demo begin......\n");
    //sp_tx_pcm_queue = xQueueCreate(SP_TX_PCM_QUEUE_LENGTH, SP_DMA_PAGE_SIZE);

#if AUDIO_SOURCE_BINARY_ARRAY
    audio_play_binary_array(sr48000_br320_stereo_aac, sr48000_br320_stereo_aac_len);
#endif

#if ADUIO_SOURCE_HTTP_FILE
    audio_pkt_queue = xQueueCreate(AUDIO_PKT_QUEUE_LENGTH, sizeof(audio_pkt_t));

	if(xTaskCreate(file_download_thread, ((const char*)"file_download_thread"), 768, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(file_download_thread) failed", __FUNCTION__);

    audio_play_http_file();
#endif

	vTaskDelete(NULL);
}

#define EXAMPLE_AUDIO_HELIX_AAC_HEAP_SIZE (768)
static uint8_t example_audio_helix_aac_heap[EXAMPLE_AUDIO_HELIX_AAC_HEAP_SIZE * sizeof( StackType_t )];

void example_audio_helix_aac(void)
{
    if ( xTaskGenericCreate( example_audio_helix_aac_thread, "example_audio_helix_aac_thread", EXAMPLE_AUDIO_HELIX_AAC_HEAP_SIZE, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL, (void *)example_audio_helix_aac_heap, NULL) != pdPASS )
        printf("\n\r%s xTaskCreate(example_audio_helix_aac_thread) failed", __FUNCTION__);
}

#endif // CONFIG_EXAMPLE_AUDIO_HELIX_AAC
