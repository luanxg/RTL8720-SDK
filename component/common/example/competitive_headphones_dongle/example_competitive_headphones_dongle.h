#if CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE
#ifndef __EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE_H__
#define __EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE_H__

#define I2S_DMA_PAGE_SIZE	480   // 2 ~ 4096
#define I2S_DMA_PAGE_NUM    4   // Vaild number is 2~4

#define DONGLE_CLINET_PORT	5002
#define DONGLE_SERVER_PORT	5001
#define RECV_PAGE_NUM 	4
#define RECV_BUF_NUM	16
#define BURST_HEADER 8
#define SEND_PKT_SIZE (I2S_DMA_PAGE_SIZE + BURST_HEADER)

#define BURST_SIZE	4

#define MIC_RESAMPLE	0
#ifdef MIC_RESAMPLE
#define RESAMPLE_RATIO	3
#else
#define RESAMPLE_RATIO	1
#endif
#define DONGLE_RECV_BUF_SIZE	(I2S_DMA_PAGE_SIZE * BURST_SIZE / RESAMPLE_RATIO)


#define RECORD_SUPPORT	1

/* QFN68 Pinmux */  
// S0
#define I2S_SCLK_PIN			PA_2
#define I2S_WS_PIN			PA_4
#define I2S_SD_TX_PIN			PB_26
#define I2S_SD_RX_PIN		PA_0
#define I2S_MCK_PIN			PA_12

extern void example_competitive_headphones_dongle();
#endif
#endif

