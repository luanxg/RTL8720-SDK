#include "mmf_sink.h"
#include "mmf_sink_pro_audio.h"
#include "platform_opts.h"
//#include "audio_api.h"

//======================== Codec =================================
#define CODEC_ANALOG       1
#define CODEC_AUDIO_INV_SCLK 0
#define CODEC_AUDIO_LOOPBACK 0
#define CODEC_DMIC         0
#define CODEC_LOOPBACK     0 //
#define CODEC_ASRC_OFF     1
#define CODEC_DC_COMP      0 
#define CODEC_DAC_LCH_EQ   0 
#define CODEC_DAC_RCH_EQ   0 
#define CODEC_ADC_LCH_EQ   0 
#define CODEC_ADC_RCH_EQ   0 
//==============================================================


//======================== Audio =================================

//#define TX_PAGE_SIZE    128 //64*N bytes, max: 4095. 128, 4032  
//#define RX_PAGE_SIZE    128 //64*N bytes, max: 4095. 128, 4032
#define TX_PAGE_SIZE    640 //64*N bytes, max: 4095. 128, 4032  
#define RX_PAGE_SIZE    640 //64*N bytes, max: 4095. 128, 4032  

#define WORD_LENGTH     2 // 0:8bits, 1:24bits, 2:16bits  // 2
#define SAMPLE_RATE     6 //0:96K, 1:88.2K, 2:48K, 3:44.1K, 4:32K, 5:16K, 6:8K // 2
#define SPORT_LOOPBACK  0

#define RX_PAGE_CNT_LI  4
#define LOG_RUN_TEST    1
#define RX_IRQ_CHECK    0 // 1
#define DATA_INPUT_DUMP 1
#define TX_PATTERN_SINE 0 // 0

#define BUF_BLK 3 //MUST BE GREATER OR EQUAL TO 2
#define AUDIO_DMA_PAGE_NUM 2
#define AUDIO_DMA_PAGE_SIZE (640)

static audio_buf_context audio_buf;
static audio_buf_context *buf_ctx=NULL;

#if CONFIG_EXAMPLE_MEDIA_AUDIO_FROM_RTP == 1
static audio_t *pro_audio_obj;
#elif CONFIG_EXAMPLE_MEDIA_BISTREAM == 1
extern audio_t *pro_audio_obj;
#else
static audio_t *pro_audio_obj;
#endif

static u8 audio_dec_buf[320]; //store decoded data
static u8 audio_chl_buf[640*3]; //store mono2stereo data
//static u8 audio_tx_buf[AUDIO_DMA_PAGE_SIZE*AUDIO_DMA_PAGE_NUM];
//static u8 audio_rx_buf[AUDIO_DMA_PAGE_SIZE*AUDIO_DMA_PAGE_NUM];
//=================================================================


volatile u32 rx_pattern;
volatile u32 rx_pattern_mask;
volatile u32 rx_pattern_offset;
volatile u32 rx_pattern_max;
volatile u32 rx_pattern_min;
volatile u32 rx_pattern_cmp_cnt = 0;


static u32 update_flag = 0;
static u32 update_token = 0;
static u32 prev_token = 0;


u8 *play_ptr = audio_chl_buf;
static void audio_tx_complete(u32 arg, u8 *pbuf)
{
    audio_t *obj = (audio_t *)arg;
    u8 *ptx_addre;
    ptx_addre = audio_get_tx_page_adr(obj);
	//if(update_token > prev_token)
	//	update_flag = 1;
	//else
	//	update_flag = 0;
	update_flag = (update_token - prev_token > 0)?1:0;

	if(update_flag)
	{
		memcpy((void*)ptx_addre, (void*)play_ptr, AUDIO_DMA_PAGE_SIZE);
		
		play_ptr += AUDIO_DMA_PAGE_SIZE;
		if(play_ptr >= audio_chl_buf + 2*AUDIO_DMA_PAGE_SIZE)
		{
			play_ptr = audio_chl_buf;
		}
		update_token--;
	}
	else
		memset((void*)ptx_addre, 0, AUDIO_DMA_PAGE_SIZE);
	audio_set_tx_page(obj, ptx_addre);    
	//printf("\n\r%d", update_token);
}

static void audio_rx_complete(u32 arg, u8 *pbuf){
	return;
}

void audio_sink_mod_close(void* ctx)
{
    printf("sink_audio_mod_close");
    audio_trx_stop(pro_audio_obj);
    (void)ctx;
    return;
}

void* audio_sink_mod_open(void)
{
#if CONFIG_EXAMPLE_MEDIA_AUDIO_FROM_RTP == 1
    pro_audio_obj = (audio_t *)malloc(sizeof(audio_t));
    if(!pro_audio_obj)
        return NULL;
    memset(pro_audio_obj,0,sizeof(audio_t));

    u8 sample_rate_tp;
    //Select sample rate
    if (SAMPLE_RATE == 0) {
        sample_rate_tp = ASR_96KHZ;
    } else if (SAMPLE_RATE == 1) {
        sample_rate_tp = ASR_88p2KHZ;
    } else if (SAMPLE_RATE == 2) {
        sample_rate_tp = ASR_48KHZ;
    } else if (SAMPLE_RATE == 3) {
        sample_rate_tp = ASR_44p1KHZ;
    } else if (SAMPLE_RATE == 4) {
        sample_rate_tp = ASR_32KHZ;
    } else if (SAMPLE_RATE == 5) {
        sample_rate_tp = ASR_16KHZ;
    } else if (SAMPLE_RATE == 6) {
        sample_rate_tp = ASR_8KHZ;
    }

    //Setting word length
    u8 word_len;
    if (WORD_LENGTH == 0){
        rx_pattern_mask = 0x0ff;
        word_len = WL_8BIT;
        rx_pattern_max = TX_PAGE_SIZE;
    } else if (WORD_LENGTH == 1) {
        rx_pattern_mask = 0x0ffffff;
        word_len = WL_24BIT;
        rx_pattern_max = TX_PAGE_SIZE >> 2;
    } else if (WORD_LENGTH == 2) {
        rx_pattern_mask = 0x0ffff;
        word_len = WL_16BIT;
        rx_pattern_max = TX_PAGE_SIZE >> 1;
    }

    audio_init(pro_audio_obj, OUTPUT_SINGLE_EDNED, INPUT_DISABLE);
    audio_set_param(pro_audio_obj, sample_rate_tp, word_len);

    //Init RX dma
    //audio_set_rx_dma_buffer(pro_audio_obj, sink_dma_rxdata, RX_PAGE_SIZE);    
    //Init TX dma
    //audio_set_tx_dma_buffer(pro_audio_obj, sink_dma_txdata, TX_PAGE_SIZE);   

    audio_mic_analog_gain(pro_audio_obj, 1, MIC_40DB); // default 0DB
#endif
    
//#if CONFIG_EXAMPLE_MEDIA_AUDIO_FROM_RTP == 1
    audio_tx_irq_handler(pro_audio_obj, (audio_irq_handler)audio_tx_complete, (uint32_t)pro_audio_obj);
#if CONFIG_EXAMPLE_MEDIA_AUDIO_FROM_RTP == 1
    audio_rx_irq_handler(pro_audio_obj, (audio_irq_handler)audio_rx_complete, (uint32_t)pro_audio_obj);
#endif
    //Audio Start
    audio_trx_start(pro_audio_obj);
    audio_set_rx_page(pro_audio_obj);
}

int audio_sink_mod_set_param(void* ctx, int cmd, int arg)
{
    audio_t* pro_audio_obj = (audio_t*) ctx;
    switch(cmd){
        case CMD_SET_STREAMMING:
            if(arg == ON){
		audio_set_tx_page(pro_audio_obj, audio_get_tx_page_adr(pro_audio_obj));
            }else{
            }
	break;
	default:
        break;
    }
    return 0;
}

//send audio data here
static int cache_byte = 0;
u8 *cache_ptr = audio_chl_buf;
int audio_sink_mod_handle(void* ctx, void* b)
{
    audio_t* pro_audio_obj = (audio_t*) ctx; 
    exch_buf_t *exbuf = (exch_buf_t*)b; 

    if(exbuf->state != STAT_READY)
        return -EAGAIN;   

    G711_decoder(exbuf->data, (short*)audio_dec_buf, AUDIO_MODE_G711U, exbuf->len);

    // exbuf->len == TX page size
#ifdef AUDIO_RECORDING_LEN
    if (AUDIO_DUMP) {        
        if (audio_byte_count < AUDIO_RECORDING_LEN) {
            memcpy(audio_dump_ptr, (void*)audio_dec_buf, (exbuf->len*2));
            audio_byte_count+=(exbuf->len*2);
            audio_dump_ptr+=(exbuf->len*2);
        }    
        if (audio_byte_count >= AUDIO_RECORDING_LEN) {
            AUDIO_DUMP = 0;
            audio_dump_ptr=tmp_buf;
            printf("---------- [swimglass] audio_rx_loop_irq_fun: record ----------\n\r");
            for(int i=0; i<AUDIO_RECORDING_LEN; i++) {
                if(i%16==0) printf("\n\r");
                printf("0x%02x,",tmp_buf[i]);   
            }
            printf("\n\r---------- [swimglass] audio_rx_loop_irq_fun: dump done ----------\n\r");
        }            
    }
#endif
	int i;
    for(i=0;i<(exbuf->len)*2;i++)
    {
        cache_ptr[i] = audio_dec_buf[i%(sizeof(audio_dec_buf)/sizeof(u8))];
    }

    cache_byte += exbuf->len*2;
    cache_ptr += exbuf->len*2;
    if(cache_ptr >= audio_chl_buf + 2*AUDIO_DMA_PAGE_SIZE)
    {
        memcpy(audio_chl_buf, audio_chl_buf + 2*AUDIO_DMA_PAGE_SIZE, cache_ptr - (audio_chl_buf + 2*AUDIO_DMA_PAGE_SIZE));
        cache_ptr -= 2*AUDIO_DMA_PAGE_SIZE;
    }
    if(cache_byte >= AUDIO_DMA_PAGE_SIZE)
    {
        cache_byte %= AUDIO_DMA_PAGE_SIZE;
        //printf("\n\rcache:%dB", cache_byte);
        update_token++;
    }
    //printf("\n\r%d", update_token);
    while(update_token >= prev_token + 2 - 1)//MINUS 1 TO AVOID RING BUFFER OUT RACE
        vTaskDelay(1);
    exbuf->state = STAT_USED;    
    return 0;
}

msink_module_t audio_sink_module = 
{
	.create = audio_sink_mod_open,
	.destroy = audio_sink_mod_close,
	.set_param = audio_sink_mod_set_param,
	.handle = audio_sink_mod_handle,
};