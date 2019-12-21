/*
* Media Single Stream example
*/
#include "FreeRTOS.h"
#include "task.h"
#include "example_media_ss.h"
#include "mmf_source.h"
#include "mmf_sink.h"
#include "mmf_sink_modules/mmf_sink_rtsp.h"
#include "wifi_conf.h" //for wifi_is_ready_to_transceive
#include "wifi_util.h" //for getting wifi mode info
#include "lwip_netconf.h" //for LwIP_GetIP, LwIP_GetMAC
#include "uvc/inc/usbd_uvc_desc.h"

#if ENABLE_PROXY_SEVER
extern msink_context	*rtsp_sink;
#endif

// select source
#define SRC_UVC					0
#define SRC_MJPG_FILE			0
#define SRC_H264_FILE	        1
#define SRC_H264_UNIT	        0
#define SRC_AAC_FILE	        0
#define SRC_PCMU_FILE	        0
#define SRC_I2S					0
#define SRC_AAC_ENC				0
#define SRC_PRO_AUDIO			0
#define SRC_H1V6_NV12			0

#if SRC_H1V6_NV12
#define SINK_UVC        1

#define NV12_W 1920
#define NV12_H 1080
#define NV12_FPS 5
#define NV12_FORMAT ISP_FORMAT_BAYER_PATTERN

#endif





//---------------------------------------------------------------------------------------
#if SRC_UVC==1
#include "mmf_source_modules/mmf_source_uvc.h"
#elif SRC_MJPG_FILE==1
#include "mmf_source_modules/mmf_source_mjpg_file.h"
#elif SRC_H264_FILE==1		
#include "mmf_source_modules/mmf_source_h264_file.h"
#elif SRC_H264_UNIT==1		
#include "mmf_source_modules/mmf_source_h264_unit.h"
#elif SRC_AAC_FILE==1		
#include "mmf_source_modules/mmf_source_aac_file.h"
#elif SRC_PCMU_FILE==1		
#include "mmf_source_modules/mmf_source_pcmu_file.h"
#elif SRC_I2S==1
#include "mmf_source_modules/mmf_source_i2s.h"
#elif SRC_AAC_ENC==1
#include "mmf_source_modules/mmf_source_aac_enc.h"
#elif SRC_PRO_AUDIO==1
#include "mmf_source_modules/mmf_source_pro_audio.h" 
#elif SRC_H1V6_NV12
#include "mmf_source_modules/mmf_source_h1v6_nv12.h"
#endif

#include "device.h"
#include "gpio_api.h"   // mbed

static msrc_context	*msrc_ctx = NULL;
static msink_context	*msink_ctx = NULL;
void GPIO_IR()
{
    gpio_t gpio_ir;

    // Init LED control pin
    gpio_init(&gpio_ir, PG_4);
    gpio_dir(&gpio_ir, PIN_OUTPUT);    // Direction: Output
    gpio_mode(&gpio_ir, PullNone);     // No pull


    gpio_write(&gpio_ir, 0);
	hal_delay_ms(1000);
    gpio_write(&gpio_ir, 1);
}

void example_media_ss_main(void *param)
{
//	rtw_mdelay_os(2000);
//        fATW0((void*)"5G_Cloud");
//	fATWC(NULL);
  
        //video_init();
        video_init();
	
	printf("\r\nexample_media_ss_main\r\n");
	printf("src=%s, sink=RTSP\n\r", (SRC_UVC      ==1)? "UVC":
									(SRC_MJPG_FILE==1)? "MJPG_FILE":
									(SRC_H264_FILE==1)? "H264_FILE":
									(SRC_H264_UNIT==1)? "H264_UNIT":
									(SRC_MJPG_FILE==1)? "MJPG_FILE":
									(SRC_AAC_FILE ==1)? "AAC_FILE":
									(SRC_PCMU_FILE==1)? "PCMU_FILE":
									(SRC_H1V6_NV12==1)? "H1V6_NV12":  
									(SRC_PRO_AUDIO==1)? "SRC_PRO_AUDIO":
									(SRC_AAC_ENC  ==1)? "AAC_ENC":
									(SRC_I2S      ==1)? "I2S":"ERROR");
	
	xQueueHandle		src_qid;
	xQueueHandle		sink_qid;
	
	src_qid = xQueueCreate(2, sizeof(exch_buf_t));
	sink_qid = xQueueCreate(2, sizeof(exch_buf_t));
#if SINK_UVC	
	GPIO_IR();
	msrc_output_queue_t sink_uvcd_q_stru = {0, &uvcd_module, sink_qid, SINK_RUN};
#else
	msrc_output_queue_t sink_rtsp_q_stru = {0, &rtsp_module, sink_qid, SINK_RUN};
#endif
	
	//create source context
#if SRC_UVC==1
	if((msrc_ctx = mmf_source_open(&uvc_module, 1, 1, 0))==NULL)
#elif SRC_MJPG_FILE==1
	if((msrc_ctx = mmf_source_open(&mjpgf_module, 1, 1, 0))==NULL)
#elif SRC_H264_FILE==1
	if((msrc_ctx = mmf_source_open(&h264f_module, 1, 1, 0))==NULL)
#elif SRC_H264_UNIT==1
	if((msrc_ctx = mmf_source_open(&h264_unit_module, 1, 1, 0))==NULL)
#elif SRC_AAC_FILE==1
	if((msrc_ctx = mmf_source_open(&aacf_module, 1, 1, 0))==NULL)
#elif SRC_PCMU_FILE==1
	if((msrc_ctx = mmf_source_open(&pcmuf_module, 1, 1, 0))==NULL)
#elif SRC_AAC_ENC==1
	if((msrc_ctx = mmf_source_open(&aac_enc_module, 1, 1, 0))==NULL)
#elif SRC_I2S==1
	if((msrc_ctx = mmf_source_open(&i2s_module, 1, 1, 0))==NULL)
#elif SRC_PRO_AUDIO==1
	if((msrc_ctx = mmf_source_open(&audio_module, 1, 1, 0))==NULL)
#elif SRC_H1V6_NV12
        if((msrc_ctx = mmf_source_open(&h1v6_nv12_module, 1, 1, 0))==NULL)
#else
	#error NO SOURCE SELECTED
	if(1)
#endif
		goto fail;
	// set source input/output queue
	mmf_source_ctrl(msrc_ctx, CMD_SET_INPUT_QUEUE, (int)src_qid);
#if SINK_UVC==1
	mmf_source_ctrl(msrc_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_uvcd_q_stru);
#else	
	mmf_source_ctrl(msrc_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp_q_stru);
#endif

	//create rtsp sink context
	//if( (msink_ctx = mmf_sink_open(&rtsp_module))==NULL)
	//	goto fail;
#if SINK_UVC
	printf("\r\nmmf_sink_open--uvcd_module\r\n");
        if( (msink_ctx = mmf_sink_open(&uvcd_module))==NULL)
		goto fail;
#else
        if( (msink_ctx = mmf_sink_open(&rtsp_module))==NULL)
		goto fail;
#endif
		
#if ENABLE_PROXY_SEVER
	rtsp_sink = msink_ctx;
#endif
	// set sink input queue
	mmf_sink_ctrl(msink_ctx, CMD_SET_INPUT_QUEUE, (int)sink_qid);
	// rtsp sink setting
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_FRAMERATE, 30);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_BITRATE, 0);
	
#if SRC_UVC==1 || SRC_MJPG_FILE==1 || SRC_H1V6_NV12 == 1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_V_MJPG);
#elif SRC_H264_FILE==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_V_H264);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SPS,(int)"Z0LADZp0BsHvN4CIAAADAAgAAAMBlHihVQ==");
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_PPS,(int)"aM48gA==");
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_LEVEL,(int)"42c00d");
#elif SRC_H264_UNIT==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_V_H264);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SPS,(int)"Z0LADZp0BsHvN4CIAAADAAgAAAMBlHihVQ==");
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_PPS,(int)"aM48gA==");
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_LEVEL,(int)"42c00d");
#elif SRC_AAC_FILE==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SAMPLERATE, 44100);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CHANNEL, 2);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_A_MP4A_LATM);
#elif SRC_AAC_ENC==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SAMPLERATE, 8000);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CHANNEL, 1);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_A_MP4A_LATM);
#elif SRC_PCMU_FILE==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SAMPLERATE, 8000);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CHANNEL, 1);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_A_PCMU);
#elif SRC_I2S==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SAMPLERATE, 8000);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CHANNEL, 1);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_A_PCMA);
#elif SRC_PRO_AUDIO==1
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_SAMPLERATE, 8000);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CHANNEL, 1);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_CODEC, FMT_A_PCMA);
#else
	#error NO SOURCE FORMAT
#endif
	//apply all above setting
	mmf_sink_ctrl(msink_ctx, CMD_RSTP_SET_APPLY, 0);

	// source setting
#if SRC_UVC==1
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_HEIGHT, 480);
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_WIDTH, 640);
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_FRAMERATE, 30);
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_CPZRATIO, 0);
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_FRAMETYPE, FMT_V_MJPG);
	mmf_source_ctrl(msrc_ctx, CMD_UVC_SET_APPLY, 0);
#elif SRC_I2S==1
	mmf_source_ctrl(msrc_ctx, CMD_I2S_SET_FRAMETYPE, FMT_A_PCMA);
#elif SRC_AAC_ENC==1
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_SET_BIT_LENGTH, 16);
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_SET_SAMPLE_RATE, 8000);
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_MEMORY_SIZE, 10*1024); // 1 frame MAX 768 bytes, 8KHz , 1s 6K?
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_BLOCK_SIZE, 128); // multiple of 128
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_MAX_FRAME_SIZE, 1024);
	mmf_source_ctrl(msrc_ctx, CMD_AACEN_SET_APPLY, 0);
#elif SRC_PRO_AUDIO==1
	mmf_source_ctrl(msrc_ctx, CMD_AUDIO_SET_FRAMETYPE, FMT_A_PCMA);
#elif SRC_H1V6_NV12==1
	mmf_source_ctrl(msrc_ctx, CMD_ISP_STREAMID, 0);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_HW_SLOT, 2);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_SW_SLOT, 6);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_HEIGHT, NV12_H);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_WIDTH, NV12_W);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_FPS, NV12_FPS);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_FORMAT, NV12_FORMAT);
	mmf_source_ctrl(msrc_ctx, CMD_ISP_APPLY, 1);
#endif

	// Streaming and task on
	mmf_sink_ctrl(msink_ctx, CMD_SET_STREAMMING, ON);
	mmf_sink_ctrl(msink_ctx, CMD_SET_TASK_ON, 0);

	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, ON);
	mmf_source_ctrl(msrc_ctx, CMD_SET_TASK_ON, 0);
	
	//disable wifi power save
	wifi_disable_powersave();
	
	// TODO: exit condition or signal
	while(msrc_ctx->hdl_task && msink_ctx->hdl_task)
	{
		vTaskDelay(1000);
	}

	if (!msrc_ctx->hdl_task)
		printf("msrc_ctx->hdl_task == NULL, exit\n\r");
	if (!msink_ctx->hdl_task)
		printf("msink_ctx->hdl_task == NULL, exit\n\r");

	mmf_sink_ctrl(msink_ctx, CMD_SET_STREAMMING, OFF);
	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, OFF);
fail:
	printf("close all source/sink and exit\n\r");
	mmf_sink_close(msink_ctx);
	mmf_source_close(msrc_ctx);
	vTaskDelete(NULL);
}

void example_media_ss(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_ss_main, ((const char*)"example_media_ss"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_ss_main: Create Task Error\n");
	}	
}