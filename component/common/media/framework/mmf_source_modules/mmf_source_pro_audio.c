/**************************************************************************//**
 * @file     main.c
 * @brief    main function example.
 * @version  V1.00
 * @date     2016-06-08
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 ******************************************************************************/

#include "mmf_source_pro_audio.h"
#include <platform/platform_stdlib.h>
#include "platform_opts.h"

void receive_check();
log_buf_type_t debug_log;
char debug_log_buf[DEBUG_LOG_BUF_SIZE];
hal_timer_adapter_t test_timer;

/*
Sine Tone Test
#define RX_PAGE_CNT_LI  2
#define LOG_RUN_TEST    1
#define RX_IRQ_CHECK    0       //Need " 0 " for Sine Tome Test
#define DATA_INPUT_DUMP 1
#define TX_PATTERN_SINE 1
*/

//======================== Codec =================================
#define CODEC_ANALOG       1
#define CODEC_audio_INV_SCLK 0
#define CODEC_audio_LOOPBACK 0
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
#define RECV_PAGE_NUM 2
//=================================================================

//Close dbg_printf
#define SG_Test 0
#if SG_Test
#define dbg_printf(x)   
#endif

#define AUDIO_RECORDING_LEN      8000*2*10 // 8kHz * 16 bit (2 byte) * 20 seconds 
sgpio_t sgpio_obj;

hal_gdma_adaptor_t gdma_adaptor_tx;
hal_gdma_adaptor_t gdma_adaptor_rx;

volatile u32 ser_first = 0;
volatile u32 ser_offset = 0;

volatile u8 get_data_flag;
volatile u32 arg_test;
volatile u32 recv_cnt;

volatile u32 tx_page_cnt;
volatile u32 tx_end = 0;
volatile u32 rx_page_cnt;
volatile u32 rx_end = 0;
volatile u32 long_run_flag;

volatile u32 rx_tx_onece_flag = 0;

volatile u32 tx_error_cnt_ck;
volatile u32 rx_error_cnt_ck;


volatile u32 tx_pattern;
volatile u32 tx_pattern_mask;
volatile u32 tx_pattern_offset;
volatile u32 tx_pattern_max;
volatile u32 tx_pattern_min;
volatile u32 tx_pattern_cmp_cnt = 0;
volatile u32 zero_pattern_cnt = 0;

volatile u8* ptxdata_bf_ck; 
volatile u8* prxdata_bf_ck; 

u32 rx_index_ck;
u8 Sel_rate;

//#define CONFIG_EXAMPLE_MEDIA_SS 1
//#define CONFIG_EXAMPLE_MEDIA_BISTREAM 0
//u8 source_dma_txdata[TX_PAGE_SIZE*2]__attribute__ ((aligned (0x20))); 
//u8 source_dma_rxdata[RX_PAGE_SIZE*2]__attribute__ ((aligned (0x20)));

#if CONFIG_EXAMPLE_MEDIA_BISTREAM == 1
audio_t *pro_audio_obj;
u8 source_dma_txdata[TX_PAGE_SIZE*2]__attribute__ ((aligned (0x20))); 
u8 source_dma_rxdata[RX_PAGE_SIZE*2]__attribute__ ((aligned (0x20)));
#else
static audio_t *pro_audio_obj;
static u8 source_dma_txdata[TX_PAGE_SIZE*2]__attribute__ ((aligned (0x20))); 
static u8 source_dma_rxdata[RX_PAGE_SIZE*2]__attribute__ ((aligned (0x20)));
#endif

static uint8_t encoder_buf[G711_FSIZE];
static audio_buf_context audio_buf;
static audio_buf_context * buf_ctx=NULL;


static void audio_tx_complete(u32 arg, u8 *pbuf)
{
    return;
}

static void audio_rx_complete(u32 arg, u8 *pbuf)
{
    audio_t *obj = (audio_t *)arg; 
    //u8 *ptx_addre;
    //ptx_addre = audio_get_tx_page_adr(obj);
    //memcpy((void*)ptx_addre, (void*)pbuf, TX_PAGE_SIZE);
    //audio_set_tx_page(obj, ptx_addre);
    if(buf_ctx != NULL){
        if(buf_ctx->len > TX_PAGE_SIZE*RECV_PAGE_NUM/2){
            buf_ctx->data_start = (buf_ctx->data_start+TX_PAGE_SIZE) % (TX_PAGE_SIZE*RECV_PAGE_NUM);
            buf_ctx->len -=TX_PAGE_SIZE;
        }
        buf_ctx->data_end = (buf_ctx->data_end+TX_PAGE_SIZE) % (TX_PAGE_SIZE*RECV_PAGE_NUM);
        buf_ctx->len +=TX_PAGE_SIZE;
    }
    audio_set_rx_page(obj);
}

static void set_audio_buf_context(audio_buf_context * des){
    buf_ctx = des;
//#if CONFIG_EXAMPLE_MEDIA_SS == 1
    buf_ctx -> raw_data = source_dma_rxdata;
//#elif CONFIG_EXAMPLE_MEDIA_BISTREAM == 1
//    buf_ctx -> raw_data = bi_dma_rxdata;
//#endif
}

void* audio_mod_open(void)
{
    rtw_mdelay_os(3000);
    //Audio buffer initalize
    audio_buf.len = audio_buf.data_start = audio_buf.data_end = 0;
    set_audio_buf_context(&audio_buf);

    pro_audio_obj = (audio_t *)malloc(sizeof(audio_t));
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
        tx_pattern_mask = 0x0ff;
        word_len = WL_8BIT;
        tx_pattern_max = TX_PAGE_SIZE;
    } else if (WORD_LENGTH == 1) {
        tx_pattern_mask = 0x0ffffff;
        word_len = WL_24BIT;
        tx_pattern_max = TX_PAGE_SIZE >> 2;
    } else if (WORD_LENGTH == 2) {
        tx_pattern_mask = 0x0ffff;
        word_len = WL_16BIT;
        tx_pattern_max = TX_PAGE_SIZE >> 1;
    }
    
    audio_init(pro_audio_obj, OUTPUT_SINGLE_EDNED, MIC_DIFFERENTIAL, AUDIO_CODEC_2p8V);
    audio_set_param(pro_audio_obj, sample_rate_tp, word_len);
    
    //Init RX dma
    audio_set_rx_dma_buffer(pro_audio_obj, source_dma_rxdata, RX_PAGE_SIZE);    
    //Init TX dma
    audio_set_tx_dma_buffer(pro_audio_obj, source_dma_txdata, TX_PAGE_SIZE);   
    
    audio_mic_analog_gain(pro_audio_obj, 1, MIC_40DB); // default 0DB

//#if CONFIG_EXAMPLE_MEDIA_SS == 1
#if CONFIG_EXAMPLE_MEDIA_SS == 1
    audio_tx_irq_handler(pro_audio_obj, (audio_irq_handler)audio_tx_complete, (uint32_t)pro_audio_obj);
#endif
    audio_rx_irq_handler(pro_audio_obj, (audio_irq_handler)audio_rx_complete, (uint32_t)pro_audio_obj);
#if CONFIG_EXAMPLE_MEDIA_SS == 1
    //Audio Start
    audio_trx_start(pro_audio_obj);
    audio_set_rx_page(pro_audio_obj);
#endif
}

void audio_mod_close(void* ctx)
{
    printf("source_audio_mod_close");
//#if CONFIG_EXAMPLE_MEDIA_SS == 1
    audio_trx_stop(pro_audio_obj);
//#elif CONFIG_EXAMPLE_MEDIA_BISTREAM == 1
//    audio_trx_stop(bi_audio_obj);
//#endif
	(void)ctx;
    return;
}

int audio_mod_set_param(void* ctx, int cmd, int arg)
{
  printf("audio_mod_set_param\n\r");
  int ret = 0;
    
    switch(cmd){
        case CMD_AUDIO_SET_FRAMETYPE:
            if(arg == FMT_A_PCMU)
                audio_buf.mode = AUDIO_MODE_G711U;
            else if(arg == FMT_A_PCMA)
                audio_buf.mode = AUDIO_MODE_G711A;
            else
                audio_buf.mode = AUDIO_MODE_G711A;//default
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}

int audio_mod_handle(void* ctx, void* b)
{
    static unsigned int time_count = 0;
    exch_buf_t *exbuf = (exch_buf_t*)b;
    unsigned char * source = audio_buf.raw_data;
    /*get uvc buffer for new data*/
    if(exbuf->state!=STAT_READY) {  
        G711_encoder((short *)(audio_buf.raw_data+audio_buf.data_start), encoder_buf, audio_buf.mode, G711_FSIZE);
        audio_buf.data_start=(audio_buf.data_start+2*G711_FSIZE)%(TX_PAGE_SIZE*RECV_PAGE_NUM);
        audio_buf.len = (audio_buf.data_end-audio_buf.data_start+(TX_PAGE_SIZE*RECV_PAGE_NUM))%(TX_PAGE_SIZE*RECV_PAGE_NUM);
        exbuf->len =  G711_FSIZE;
        exbuf->index = 0;
        exbuf->data = encoder_buf;
        time_count+=160;
        exbuf->timestamp = time_count;
        exbuf->state = STAT_READY;
		
		mmf_source_add_exbuf_sending_list_all(exbuf);
    }
    return 0;
}

msrc_module_t audio_module =
{
    .create = audio_mod_open,
    .destroy = audio_mod_close,
    .set_param = audio_mod_set_param,
    .handle = audio_mod_handle,
};