
/*
* MMF two source example
*/

#include "FreeRTOS.h"
#include "task.h"

#include "mmf_source.h"
#include "mmf_sink.h"
#include "mmf_sink_modules/mmf_sink_rtsp2.h"
#include "mmf_command.h"

// select source
#define SRC_H264_FILE	1
#define SRC_AAC_FILE	1

//---------------------------------------------------------------------------------------
#if SRC_H264_FILE==1
#include "mmf_source_modules/mmf_source_h264_file.h"
static msrc_context		*msrc_h264_ctx = NULL;
#endif

#if SRC_AAC_FILE==1		
#include "mmf_source_modules/mmf_source_aac_file.h"
static msrc_context		*msrc_aac_ctx = NULL;
#endif

static msink_context	*msink_rtsp2_ctx = NULL;

static mcmd_context		*mcmd_ctx = NULL;

static int mcmd_socket_cb(void* p_req, void* p_rsp)
{
	u8 *req = (u8*) p_req;
	u8 *rsp = (u8*) p_rsp;
	
	for(int i=0; i<strlen(req); i++) {
		if( (req[i] == '\r') || (req[i] == '\n')) {
			req[i] = '\0';
			break;
		}
	}
	
	printf("mcmd_socket_cb: req=%s\n\r",req);
	
	if (req[0]=='a') {
		strcpy(rsp,"A");
		mcmd_t *cmd = mcmd_cmd_create(3,3,0,strlen(req),0,0,req);
		mcmd_in_cmd_queue(cmd,mcmd_ctx);
	}
	else if (req[0]=='b') {
		strcpy(rsp,"B");
		mcmd_t *cmd = mcmd_cmd_create(4,4,0,strlen(req),0,0,req);
		mcmd_in_cmd_queue(cmd,mcmd_ctx);
	}
	else if (req[0]=='c'){
		strcpy(rsp,"C");
		mcmd_t *cmd = mcmd_cmd_create(5,5,0,strlen(req),0,0,req);
		mcmd_in_cmd_queue(cmd,mcmd_ctx);
	}	
	else {
		strcpy(rsp,"unknown....\n\r");
	}
	return strlen(rsp);
}


static int mcmd_custom_cb (void* param) {
	//printf("mcmd_custom_cb in exmaple_ss\n\r");
	mcmd_t *cmd = (mcmd_t *) param;
	if(cmd->bCmd==3 && cmd->bSubCommand== 3)
	{
		printf("command type A: %s\n\r",cmd->data);
	}
	else if(cmd->bCmd==4 && cmd->bSubCommand== 4)
	{
		printf("command type B: %s\n\r",cmd->data);
	}
	else if(cmd->bCmd==5 && cmd->bSubCommand== 5)
	{
		printf("command type C: %s\n\r",cmd->data);
	}
	
	return 0;
}


void example_media_two_source_main(void *param)
{
	rtw_mdelay_os(1000);
	printf("\r\nexample_media_two_source_main\r\n");
	
#if SRC_H264_FILE==1
	xQueueHandle		src_h264_qid;
#endif
	
#if SRC_AAC_FILE==1
	xQueueHandle		src_aac_qid;
#endif
	
	xQueueHandle		sink_qid;
	
#if USE_MCMD_CB==1
	mcmd_ctx = mcmd_context_init();
	mcmd_ctx->cb_custom = mcmd_custom_cb;
	mcmd_queue_task_open(mcmd_ctx);
#endif
	
#if SRC_H264_FILE==1
	src_h264_qid = xQueueCreate(2, sizeof(exch_buf_t));
#endif
#if SRC_AAC_FILE==1	
	src_aac_qid = xQueueCreate(2, sizeof(exch_buf_t));
#endif
	sink_qid = xQueueCreate(4, sizeof(exch_buf_t));
	msrc_output_queue_t sink_rtsp2_q_stru = {0, &rtsp2_module, sink_qid, SINK_RUN};
	
	//create source context
	// mmf_source_open(module, sink_count, max_share_count, cache_size)
	
#if SRC_H264_FILE==1
	if((msrc_h264_ctx = mmf_source_open(&h264f_module, 1, 1, 0))==NULL)
		goto fail;
#endif

#if SRC_AAC_FILE==1	
	if((msrc_aac_ctx = mmf_source_open(&aacf_module, 1, 1, 0))==NULL)
		goto fail;
#endif

#if SRC_H264_FILE==1
	mmf_source_ctrl(msrc_h264_ctx, CMD_SET_INPUT_QUEUE, (int)src_h264_qid);
	mmf_source_ctrl(msrc_h264_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp2_q_stru);
#endif

#if SRC_AAC_FILE==1
	mmf_source_ctrl(msrc_aac_ctx, CMD_SET_INPUT_QUEUE, (int)src_aac_qid);
	mmf_source_ctrl(msrc_aac_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp2_q_stru);
#endif

	//create rtsp2 sink context
	if( (msink_rtsp2_ctx = mmf_sink_open(&rtsp2_module))==NULL)
		goto fail;
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_INPUT_QUEUE, (int)sink_qid);
	
	// SRC_H264_FILE sink setting
#if SRC_H264_FILE==1
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SELECT_CHANNEL, 0);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CHANNEL, 1);	
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CODEC, FMT_V_H264);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_FRAMERATE, 25);
	//mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_BITRATE, 0);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_SPS,(int)"Z0KgKedAPAET8uAoEAABd2AAK/IGAAADAC+vCAAAHc1lP//jAAADABfXhAAADuayn//wIA==");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_PPS,(int)"aN48gA==");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_LEVEL,(int)"42A029");
	//apply all above setting
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_APPLY, 0);
#endif
	
#if SRC_AAC_FILE==1

	#if SRC_H264_FILE==1
		mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SELECT_CHANNEL, 1);
	#else
		mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SELECT_CHANNEL, 0);
	#endif
	
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CHANNEL, 2);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CODEC, FMT_A_MP4A_LATM);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_SAMPLERATE, 44100);
	//apply all above setting
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_APPLY, 0);
#endif

	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_FLAG, TIME_SYNC_DIS);

	// Streaming and task on
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_STREAMMING, ON);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_TASK_ON, 0);
	
#if SRC_H264_FILE==1	
	mmf_source_ctrl(msrc_h264_ctx, CMD_SET_STREAMMING, ON);
	mmf_source_ctrl(msrc_h264_ctx, CMD_SET_TASK_ON, 0);
#endif

#if SRC_AAC_FILE==1	
	mmf_source_ctrl(msrc_aac_ctx, CMD_SET_STREAMMING, ON);
	mmf_source_ctrl(msrc_aac_ctx, CMD_SET_TASK_ON, 0);
#endif
	//disable wifi power save        
	wifi_disable_powersave();


	// create mmf command soket on port 16385
	// use command socket and callback function (mcmd_socket_cb) to do dynamic control
	mcmd_socket_t* mcmd_socket;
	mcmd_socket = mcmd_socket_create();
	do {
		if (!mcmd_socket) {
			printf("mcmd_ctx == NULL, command socket is not available\n\r");
			break;
		}
		if (mcmd_socket_open(mcmd_socket, (void*)mcmd_socket_cb)<0) {
			mcmd_socket_free(mcmd_socket);
			printf("mcmd_socket_open fail, command socket is not available\n\r");
		}
		else {
			printf("[example_media_two_source_main] mcmd_socket ready\n\r");
		}
	} while(0);
	
	// TODO: exit condition or signal
	while(1)
	{
		vTaskDelay(1000);	
	}


	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_STREAMMING, OFF);
#if SRC_H264_FILE==1	
	mmf_source_ctrl(msrc_h264_ctx, CMD_SET_STREAMMING, OFF);
#endif
#if SRC_AAC_FILE==1
	mmf_source_ctrl(msrc_aac_ctx, CMD_SET_STREAMMING, OFF);
#endif
	
fail:
	printf("close all source/sink and exit\n\r");
	mcmd_context_deinit(mcmd_ctx);
	mmf_sink_close(msink_rtsp2_ctx);
#if SRC_H264_FILE==1
	mmf_source_close(msrc_h264_ctx);
#endif	
#if SRC_AAC_FILE==1
	mmf_source_close(msrc_aac_ctx);
#endif	
	vTaskDelete(NULL);
}


void example_media_two_source(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_two_source_main, ((const char*)"media_2s"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_two_source_main: Create Task Error\n");
	}	
}

