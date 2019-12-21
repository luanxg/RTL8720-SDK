#if CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#ifndef __EXAMPLE_COMPETITIVE_HEADPHONES_H__
#define __EXAMPLE_COMPETITIVE_HEADPHONES_H__

#include "ameba_soc.h"

#define BURST_SIZE	4
#define BURST_PERIOD	10
#define MIN_RFOFF_TIME		2 //ms
#define RFOFF_USER_TIMER	1

#define SP_DMA_PAGE_NUM    4
#define SP_ZERO_BUF_SIZE	128
#define PS_USED	0

#define RECV_SP_BUF_SIZE	480
#define BURST_HEADER_SIZE	8
#define PLAY_PACKET_NUM		(BURST_SIZE)
#define SP_DMA_PAGE_SIZE	(RECV_SP_BUF_SIZE*PLAY_PACKET_NUM)// 2 ~ 4096
#define PACKET_BUFFER_SIZE	0
#define RECV_BUF_SIZE	(RECV_SP_BUF_SIZE + BURST_HEADER_SIZE)

#define SP_FULL_BUF_SIZE		128
#define RESAMPLE_RATIO			3
#define RECORD_RESAMPLE_SIZE	(SP_DMA_PAGE_SIZE/RESAMPLE_RATIO)
#define RECORD_SEND_PKT_SIZE	(SP_DMA_PAGE_SIZE/RESAMPLE_RATIO)

#define HEADPHONES_SERVER_PORT	5002
#define HEADPHONES_CLIENT_PORT	5001
#define RECV_PKT_QUEUE_LENGTH (64) //(100)

typedef struct {
	GDMA_InitTypeDef       	SpTxGdmaInitStruct;              //Pointer to GDMA_InitTypeDef
	GDMA_InitTypeDef       	SpRxGdmaInitStruct;              //Pointer to GDMA_InitTypeDef		
}SP_GDMA_STRUCT, *pSP_GDMA_STRUCT;

typedef struct {
	u8 tx_gdma_own;
	u32 tx_addr;
	u32 tx_length;
	
}TX_BLOCK, *pTX_BLOCK;

typedef struct {
	TX_BLOCK tx_block[SP_DMA_PAGE_NUM];
	TX_BLOCK tx_zero_block;
	u8 tx_gdma_cnt;
	u8 tx_usr_cnt;
	u8 tx_empty_flag;
	
}SP_TX_INFO, *pSP_TX_INFO;

typedef struct {
	u8 rx_gdma_own;
	u32 rx_addr;
	u32 rx_length;
	
}RX_BLOCK, *pRX_BLOCK;

typedef struct {
	RX_BLOCK rx_block[SP_DMA_PAGE_NUM];
	RX_BLOCK rx_full_block;
	u8 rx_gdma_cnt;
	u8 rx_usr_cnt;
	u8 rx_full_flag;
	
}SP_RX_INFO, *pSP_RX_INFO;


typedef struct {
	u32 sample_rate;
	u32 word_len;
	u32 mono_stereo;
	u32 direction;
	
}SP_OBJ, *pSP_OBJ;

typedef struct _recv_pkt_t {
    uint8_t *data;
    uint32_t data_len;
} recv_pkt_t;
extern void example_competitive_headphones();
#endif
#endif

