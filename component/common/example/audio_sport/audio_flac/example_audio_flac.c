#include "example_audio_flac.h"

#include <platform/platform_stdlib.h>
#include "platform_opts.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"
#include "rl6548.h"
#include "bitstream.h"
#include "decoder.h"
#include "golomb.h"

#if CONFIG_EXAMPLE_AUDIO_FLAC

//-----------------Frequency Mapping Table--------------------//
/*+-------------+-------------------------+--------------------+
| Frequency(hz) | Number of Channels      | Decoded Bytes      |
                |(CH_MONO:1 CH_STEREO:2)  |(I2S_DMA_PAGE_SIZE) |
+---------------+-------------------------+--------------------+
|          8000 |                       1 |               1152 |
|          8000 |                       2 |               2304 |
|         16000 |                       1 |               1152 |
|         16000 |                       2 |               2304 |
|         22050 |                       1 |               1152 |
|         22050 |                       2 |               2304 |
|         24000 |                       1 |               1152 |
|         24000 |                       2 |               2304 |
|         32000 |                       1 |               2304 |
|         32000 |                       2 |               4608 |
|         44100 |                       1 |               2304 |
|         44100 |                       2 |               4608 |
|         48000 |                       1 |               2304 |
|         48000 |                       2 |               4608 |
+---------------+-------------------------+------------------+*/


//------------------------------------- ---CONFIG Parameters-----------------------------------------------//


//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
                                
#ifdef __GNUC__
#undef SDRAM_DATA_SECTION
#define SDRAM_DATA_SECTION 
#endif

#undef printf
#define printf DiagPrintf

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

static u8 *sp_tx_buf;
static u8 *sp_zero_buf;
static u32 sp_zero_buf_size;
static u32 sp_dma_page_size;
static u32 sampling_freq;
static u32 sr_value;
static u32 word_len;
static u8 *wav;

typedef struct _file_info {
	u32 sampling_freq;
	u32 word_len;
	u32 sr_value;
	u8 *wav;
}file_info;


#define FILE_NUM				6
static file_info file[FILE_NUM] = {
	{SR_44P1K, WL_16, 44100, "997Hz_sine_wave_44100Hz_16bit_2ch.flac"},
	{SR_48K, WL_16, 48000, "997Hz_sine_wave_48000Hz_16bit_2ch.flac"},
	{SR_96K, WL_16, 96000, "997Hz_sine_wave_96000Hz_16bit_2ch.flac"},
	{SR_96K, WL_24, 96000, "997Hz_sine_wave_96000Hz_24bit_2ch_level0.flac"},
	{SR_96K, WL_24, 96000, "997Hz_sine_wave_96000Hz_24bit_2ch_level8.flac"},
	{SR_96K, WL_24, 96000, "noise_96000Hz_24bit_2ch_level0.flac"},
	
};

#define decoderScatchSize MAX_FRAMESIZE + MAX_BLOCKSIZE*8
// Buffer for all decoders
static unsigned char g_decoderScratch[decoderScatchSize];


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
		//printf("tx %d\n", sp_tx_info.tx_gdma_cnt);
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
	PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k
	PLL_Sel(PLL_I2S);

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
        	case SR_44P1K:
            		div = 4;
            		PLL_Sel(PLL_PCM);
           		 break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
	CODEC_SetVolume(0x90, 0x90);

}

static void sp_init_tx_variables(void)
{
	int i;

	sp_zero_buf = malloc(sp_zero_buf_size);
	sp_tx_buf = malloc(sp_dma_page_size*SP_DMA_PAGE_NUM);
	
	for(i=0; i<sp_zero_buf_size; i++){
		sp_zero_buf[i] = 0;
	}
	sp_tx_info.tx_zero_block.tx_addr = (u32)sp_zero_buf;
	sp_tx_info.tx_zero_block.tx_length = (u32)sp_zero_buf_size;
	
	sp_tx_info.tx_gdma_cnt = 0;
	sp_tx_info.tx_usr_cnt = 0;
	sp_tx_info.tx_empty_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_tx_info.tx_block[i].tx_gdma_own = 0;
		sp_tx_info.tx_block[i].tx_addr = sp_tx_buf+i*sp_dma_page_size;
		sp_tx_info.tx_block[i].tx_length = sp_dma_page_size;
	}
}

//converts string to all uppercase letters
static void strToUppercase(char * string)
{
	unsigned long i = 0;
	
	while(string[i] != '\0')
	{
		if(string[i] >= 97 && string[i] <= 122)
		{
			string[i] -= 32;
		}
		i++;

	}
}

//This function creates the FLACContext the file at filePath
//Called by main FLAC decoder
//See http://flac.sourceforge.net/format.html for FLAC format details
//0 context is valid; 1 context is not valid
static int Flac_ParceMetadata(const TCHAR filePath[], FLACContext* context)
{
	UINT s1 = 0;
	FIL FLACfile;
	int metaDataFlag = 1;
	char metaDataChunk[128];	
	unsigned long metaDataBlockLength = 0;
	char* tagContents;

	if(f_open(&FLACfile, filePath, FA_READ) != FR_OK) {
		printf("Could not open: %s\r\n", filePath);
		return 1;
	}
	f_read(&FLACfile, metaDataChunk, 4, &s1);
	if(s1 != 4) {
		printf("Read failure\r\n");
		f_close(&FLACfile);
		return 1;
	}

	if(memcmp(metaDataChunk, "fLaC", 4) != 0) {
		printf("Not a FLAC file\r\n");
		f_close(&FLACfile);
		return 1;
	}
	
	// Now we are at the stream block
	// Each block has metadata header of 4 bytes
	do {
		f_read(&FLACfile, metaDataChunk, 4, &s1);
		if(s1 != 4) {
			printf("Read failure\r\n");
			f_close(&FLACfile);
			return 1;
		}

		//Check if last chunk
		if(metaDataChunk[0] & 0x80) metaDataFlag = 0;
		metaDataBlockLength = (metaDataChunk[1] << 16) | (metaDataChunk[2] << 8) | metaDataChunk[3];
		//STREAMINFO block
		if((metaDataChunk[0] & 0x7F) == 0) {						
			if(metaDataBlockLength > 128) {
				printf("Metadata buffer too small\r\n");
				f_close(&FLACfile);
				return 1;
			}

			f_read(&FLACfile, metaDataChunk, metaDataBlockLength, &s1);
			if(s1 != metaDataBlockLength) {
				printf("Read failure\r\n");
				f_close(&FLACfile);
				return 1;
			}
			/* 
			<bits> Field in STEAMINFO
			<16> min block size (samples)
			<16> max block size (samples)
			<24> min frams size (bytes)
			<24> max frams size (bytes)
			<20> Sample rate (Hz)
			<3> (number of channels)-1
			<5> (bits per sample)-1. 
			<36> Total samples in stream. 
			<128> MD5 signature of the unencoded audio data.
			*/		
			context->min_blocksize = (metaDataChunk[0] << 8) | metaDataChunk[1];
			context->max_blocksize = (metaDataChunk[2] << 8) | metaDataChunk[3];
			context->min_framesize = (metaDataChunk[4] << 16) | (metaDataChunk[5] << 8) | metaDataChunk[6];
			context->max_framesize = (metaDataChunk[7] << 16) | (metaDataChunk[8] << 8) | metaDataChunk[9];
			context->samplerate = (metaDataChunk[10] << 12) | (metaDataChunk[11] << 4) | ((metaDataChunk[12] & 0xf0) >> 4);
			context->channels = ((metaDataChunk[12] & 0x0e) >> 1) + 1;
			context->bps = (((metaDataChunk[12] & 0x01) << 4) | ((metaDataChunk[13] & 0xf0)>>4) ) + 1;			
			//This field in FLAC context is limited to 32-bits
			context->totalsamples = (metaDataChunk[14] << 24) | (metaDataChunk[15] << 16) | (metaDataChunk[16] << 8) | metaDataChunk[17];
		} else if((metaDataChunk[0] & 0x7F) == 4) {
			unsigned long fieldLength, commentListLength;			
			unsigned long readCount;
			unsigned long totalReadCount = 0;
			unsigned long currentCommentNumber = 0;
			int readAmount;

			f_read(&FLACfile, &fieldLength, 4, &s1);
			totalReadCount +=s1;
			//Read vendor info
			readCount = 0;
			readAmount = 128;
			while(readCount < fieldLength) {
				if(fieldLength-readCount < readAmount) readAmount = fieldLength-readCount;
				if(readAmount> metaDataBlockLength-totalReadCount) {
					printf("Malformed metadata aborting\r\n");
					f_close(&FLACfile);
					return 1;
				
				}
				f_read(&FLACfile, metaDataChunk, readAmount, &s1);
				readCount += s1;
				totalReadCount +=s1;
				//terminate the string								
				metaDataChunk[s1-1] = '\0';
			}

			f_read(&FLACfile, &commentListLength, 4, &s1);
			totalReadCount +=s1;
			while(currentCommentNumber < commentListLength) {
				f_read(&FLACfile, &fieldLength, 4, &s1);
				totalReadCount +=s1;
				readCount = 0;
				readAmount = 128;
				while(readCount < fieldLength) {
					if(fieldLength-readCount < readAmount) readAmount = fieldLength-readCount;
					if(readAmount> metaDataBlockLength-totalReadCount) {
						printf("Malformed metadata aborting\r\n");
						f_close(&FLACfile);
						return 1;
					
					}
					f_read(&FLACfile, metaDataChunk, readAmount, &s1);
					readCount += s1;
					totalReadCount +=s1;
					//terminate the string
					metaDataChunk[s1-1] = '\0';
					
					//Make another with just contents
					tagContents = strchr(metaDataChunk, '=');
					if (!tagContents) {
						continue;
					}					
					tagContents[0] = '\0';
					tagContents = &tagContents[1];
					strToUppercase(metaDataChunk);
					if(strcmp(metaDataChunk, "ARTIST") == 0) {		
						printf("Artist: %s\r\n", tagContents);
					} else if(strcmp(metaDataChunk, "TITLE") == 0) {	
						printf("Title: %s\r\n", tagContents);
					} else if(strcmp(metaDataChunk, "ALBUM") == 0) {	
						printf("Album: %s\r\n", tagContents);
					}					
				}
				currentCommentNumber++;
			}
			if(f_lseek(&FLACfile, FLACfile.fptr + metaDataBlockLength-totalReadCount) != FR_OK) {
				printf("File Seek Faile\r\n");
				f_close(&FLACfile);
				return 1;
			}
		} else {		
			if(f_lseek(&FLACfile, FLACfile.fptr + metaDataBlockLength) != FR_OK) {
				printf("File Seek Failed\r\n");
				f_close(&FLACfile);
				return 1;
			}
		}		
	} while(metaDataFlag);

	// track length in ms
	context->length = (context->totalsamples / context->samplerate) * 1000; 
	// file size in bytes
	context->filesize = f_size(&FLACfile);					
	// current offset is end of metadata in bytes
	context->metadatalength = FLACfile.fptr;
	// bitrate of file				
	context->bitrate = ((context->filesize - context->metadatalength) * 8) / context->length;
	f_close(&FLACfile);
	return 0;	
}

//Just a dummy function for the flac_decode_frame
static void yield() 
{
	//Do nothing
}

int Flac_Play(u8* filename)
{
	FIL FLACfile;
	UINT bytesLeft, bytesUsed, s1;
	int i;

	FLACContext context;
	int sampleShift;

	char	logical_drv[4]; //root diretor
	char abs_path[64]; //Path to input file
	
	//Pointers to memory chuncks in scratchMemory for decode
	//fileChunk currently can't be in EPI as it needs byte access
	unsigned char* bytePointer;
	unsigned char* fileChunk;
	int32_t* decodedSamplesLeft;
	int32_t* decodedSamplesRight;
	int remain_blk = 0;
	int blk_per_page = 0;
	int tx_len = 0;
	int *ptx_buf;
	
	int drv_num = 0;
	FATFS 	m_fs;
	u32 cnt = 0;
	int *tmp_buf;
	u32 time1, time2, total_time = 0;

	drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
	if(drv_num < 0){
		printf("Rigester disk driver to FATFS fail.\n");
		return;
	}else{
		logical_drv[0] = drv_num + '0';
		logical_drv[1] = ':';
		logical_drv[2] = '/';
		logical_drv[3] = 0;
	}

	if(f_mount(&m_fs, logical_drv, 1)!= FR_OK){
		printf("FATFS mount logical drive fail, please format DISK to FAT16/32.\n");
		goto unreg;
	}

	memset(abs_path, 0x00, sizeof(abs_path));
	strcpy(abs_path, logical_drv);
	sprintf(&abs_path[strlen(abs_path)],"%s", filename);
	
	//Setup the pointers, the defines are in decoder.h
	bytePointer = (unsigned char*)g_decoderScratch;
	fileChunk = bytePointer;
	decodedSamplesLeft = (int32_t*)&bytePointer[MAX_FRAMESIZE];
	decodedSamplesRight = (int32_t*)&bytePointer[MAX_FRAMESIZE+4*MAX_BLOCKSIZE];

	//Get the metadata we need to play the file
	if(Flac_ParceMetadata(abs_path, &context) != 0) {
		printf("Failed to get FLAC context\r\n");
		goto umount;
	}

	if(f_open(&FLACfile, abs_path ,FA_READ) != FR_OK) {
		printf("Cannot open: %s\r\n", abs_path);
		goto umount;
	}

	//Goto start of stream
	if(f_lseek(&FLACfile, context.metadatalength) != FR_OK) {
		f_close(&FLACfile);
		goto umount;
	}

	//The decoder has sample size defined by FLAC_OUTPUT_DEPTH (currently 29 bit)
	//Shift for lower bitrate to align MSB correctly
	sampleShift = FLAC_OUTPUT_DEPTH-context.bps;
	//Fill up fileChunk completely (MAX_FRAMSIZE = valid size of memory fileChunk points to)
	f_read(&FLACfile, fileChunk, MAX_FRAMESIZE, &bytesLeft);
	
	printf("Playing %s\r\n", abs_path);
	printf("Mode: %s\r\n", context.channels==1?"Mono":"Stereo");
	printf("Samplerate: %d Hz\r\n", context.samplerate);
	printf("SampleBits: %d bit\r\n", context.bps);
	printf("Samples: %d\r\n", context.totalsamples);

	while (bytesLeft) {
		if(flac_decode_frame(&context, decodedSamplesLeft, decodedSamplesRight, fileChunk, bytesLeft, yield) < 0) {
			printf("FLAC Decode Failed\r\n");
			break;
		}		

		//Dump the block to the waveOut
		if (word_len == WL_24){
			if (NUM_CHANNELS == CH_MONO){
				blk_per_page = sp_dma_page_size/4;
			}
			else{
				blk_per_page = sp_dma_page_size/8;
			}
		}
		else if (word_len == WL_16){
			if (NUM_CHANNELS == CH_MONO){
				blk_per_page = sp_dma_page_size/2;
			}
			else{
				blk_per_page = sp_dma_page_size/4;
			}
		}
		remain_blk = context.blocksize;
		i = 0;
		tmp_buf = malloc(sp_dma_page_size);
		while(remain_blk) {
			while(!sp_get_free_tx_page()){
				//printf("=============\n");
				vTaskDelay(1);
			}
			if (remain_blk <= blk_per_page){
				tx_len = remain_blk;
			}
			else{
				tx_len = blk_per_page;
			}

			if (word_len == WL_24){
				if (NUM_CHANNELS == CH_MONO){
					for(int j=0; j<tx_len; i++, j++){
						tmp_buf[j] = (decodedSamplesLeft[i]>>sampleShift)<<8;
					}
					sp_write_tx_page((u8 *)tmp_buf, tx_len*4);
					
				}
				else{
					for(int j=0; j<tx_len; i++, j++){
						tmp_buf[2*j] = (decodedSamplesLeft[i]>>sampleShift)<<8;
						tmp_buf[2*j+1] = (decodedSamplesRight[i]>>sampleShift)<<8;
					}	
					sp_write_tx_page((u8 *)tmp_buf, tx_len*8);					
				}
			}
			else if (word_len == WL_16){
				if (NUM_CHANNELS == CH_MONO){
					for(int j=0; j<tx_len; i++, j++){
						((short *)tmp_buf)[j] = (short)(decodedSamplesLeft[i]>>sampleShift);
					}
					sp_write_tx_page((u8 *)tmp_buf, tx_len*2);					
				}
				else{
					for(int j=0; j<tx_len; i++, j++){
						((u16 *)tmp_buf)[2*j] = (u16)(decodedSamplesLeft[i]>>sampleShift);
						((u16 *)tmp_buf)[2*j+1] = (u16)(decodedSamplesRight[i]>>sampleShift);
					}
					sp_write_tx_page((u8 *)tmp_buf, tx_len*4);					
				}
			}
			
			remain_blk -= tx_len; 
		}
		free((void *)tmp_buf);

		//calculate the number of valid bytes left in the fileChunk buffer
		bytesUsed = context.gb.index/8;
		bytesLeft -= bytesUsed;
		//shift the unused stuff to the front of the fileChunk buffer
		memmove(fileChunk, &fileChunk[bytesUsed], bytesLeft);
		//Refill the fileChunk buffer
		f_read(&FLACfile, &fileChunk[bytesLeft], MAX_FRAMESIZE - bytesLeft, &s1);
		//add however many were read
		bytesLeft += s1;
	}
	
	
	f_close(&FLACfile);

umount:
	if(f_mount(NULL, logical_drv, 1) != FR_OK){
		printf("FATFS unmount logical drive fail.\n");
	}

unreg:	
	if(FATFS_UnRegisterDiskDriver(drv_num)) 
	printf("Unregister disk driver from FATFS fail.\n");

	DelayMs(50);
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, DISABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, DISABLE);
	GDMA_ClearINT(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	GDMA_Cmd(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum, DISABLE);
	GDMA_ChnlFree(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	free(sp_tx_buf);
	free(sp_zero_buf);
	return 0;	
}

void example_audio_flac_thread(void* param)
{
	u32 tx_addr;
	u32 tx_length;
	pSP_OBJ psp_obj = (pSP_OBJ)param;

	
	printf("Audio codec flac demo begin......\n");

	for(int i=0; i<FILE_NUM; i++){
		sampling_freq = file[i].sampling_freq;
		word_len = file[i].word_len;
		sr_value = file[i].sr_value;
		wav = file[i].wav;

		sp_dma_page_size = sr_value/100*8;
		sp_zero_buf_size = sp_dma_page_size;

		psp_obj->sample_rate = sampling_freq;
		psp_obj->word_len = word_len;

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
		
		Flac_Play(wav);
	}
exit:
	vTaskDelete(NULL);
}

void example_audio_flac(void)
{
	
	sp_obj.mono_stereo = NUM_CHANNELS;
	sp_obj.direction = APP_LINE_OUT;
	RTIM_Cmd(TIM0, ENABLE);
	if(xTaskCreate(example_audio_flac_thread, ((const char*)"example_audio_flac_thread"), 4000, (void *)(&sp_obj), tskIDLE_PRIORITY + 1, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(example_audio_flac_thread) failed", __FUNCTION__);
}

void vApplicationMallocFailedHook(void)
{

	
}

#endif

