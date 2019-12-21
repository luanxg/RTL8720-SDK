
/*
* geo rtsp2 example
*/

#include "FreeRTOS.h"
#include "task.h"
#include "example_media_geo_rtsp2.h"
#include "mmf_command.h"
#include "mmf_source.h"
#include "mmf_sink.h"
#include "mmf_source_modules/mmf_source_geo.h"
#include "mmf_sink_modules/mmf_sink_rtsp2.h"

#define USE_MCMD_CB 0

#define CH1 0
#define CH2 1
#define CH3 2
#define CH4 3

static msrc_context		*msrc_geo_ctx = NULL;
static msink_context	*msink_rtsp2_ctx = NULL;

#if USE_MCMD_CB==1
static mcmd_context		*mcmd_ctx = NULL;

static int rtsp_start_cb (void* param) {
	mcmd_t *cmd = mcmd_cmd_create(1,1,0,0,0,0,NULL);
	mcmd_in_cmd_queue(cmd,mcmd_ctx);
	return 0;
}

static int rtsp_stop_cb (void* param) {
	mcmd_t *cmd = mcmd_cmd_create(2,2,0,0,0,0,NULL);
	mcmd_in_cmd_queue(cmd,mcmd_ctx);
	return 0;
}

static int mcmd_custom_cb (void* param) {
	mcmd_t *cmd = (mcmd_t *) param;
	if(cmd->bCmd==1 && cmd->bSubCommand== 1) {	// fake: rtsp start
		printf("\n\rFAKE: RTSP start\n\r");
		// Video Streaming
		printf("start CH1\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		// Audio
		printf("start CH4\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON);
		printf("src streaming on\n\r");
		mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_RUN);
	}
	else if(cmd->bCmd==2 && cmd->bSubCommand== 2) {	// fake: rtsp stop
		printf("\n\rFAKE: RTSP stop\n\r");
		mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_STOP);
		mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, OFF);
		printf("src streaming off\n\r");
		// Video Streaming
		printf("stop CH1\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		// Audio
		printf("stop CH4\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		printf("src streaming off\n\r");
	}
	return 0;
}
#endif

void example_media_geo_rtsp2_main(void *param)
{
	printf("\r\nexample_media_geo_rtsp2_main\r\n");

	int ret;
	
	xQueueHandle		src_geo_qid;
	xQueueHandle		sink_rtsp2_qid;
	
	src_geo_qid = xQueueCreate(2, sizeof(exch_buf_t));
	sink_rtsp2_qid = xQueueCreate(2, sizeof(exch_buf_t));
	
	msrc_output_queue_t sink_rtsp2_q_stru = {0, &rtsp2_module, sink_rtsp2_qid, SINK_STOP};
	
#if USE_MCMD_CB==1	
	mcmd_ctx = mcmd_context_init();
	mcmd_ctx->cb_custom = mcmd_custom_cb;
	mcmd_queue_task_open(mcmd_ctx);
#endif
	
	//create geo source context
	if((msrc_geo_ctx = mmf_source_open(&geo_module, 1, 1, 0))==NULL)
		goto fail;
	// set geo source input/output queue
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_INPUT_QUEUE, (int)src_geo_qid);
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp2_q_stru);
	
	//create rtsp2 sink context
	if( (msink_rtsp2_ctx = mmf_sink_open(&rtsp2_module))==NULL)
		goto fail;
	// set rtsp2 sink input queue
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_INPUT_QUEUE, (int)sink_rtsp2_qid);
	
	// rtsp2 sink setting
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SELECT_CHANNEL, 0);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CODEC, FMT_A_MP4A_LATM);
	// for MP4A 44100 stereo, video stream will ignore this setting
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_SAMPLERATE, 16000);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CHANNEL, 2);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_APPLY, 0);
	
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SELECT_CHANNEL, 1);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_CODEC, FMT_V_H264);
	// for video, audio stream will ignore this setting
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_FRAMERATE, 30);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_BITRATE, 0);
	//mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_SPS,(int)"Z0KgKedAPAET8uAoEAABd2AAK/IGAAADAGb/MAAHoSD//4wAAAMAzf5gAA9CQf//Ag==");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_SPS,(int)"Z0KgKedAPAET8uAoEAABd2AAK/IGAAADAC+vCAAAHc1lP//jAAADABfXhAAADuayn//wIA==");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_PPS,(int)"aN48gA==");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_LEVEL,(int)"42A029");
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_FLAG, TIME_SYNC_DIS);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_APPLY, 0);
#if USE_MCMD_CB==1
	// set RTSP2 callback for sending mmf commands (dynamically turn on/off source stream)
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_START_CB, (int) rtsp_start_cb);
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_RSTP2_SET_STOP_CB, (int) rtsp_stop_cb);	
#endif
	
	// Streaming and task on
	if((ret = mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_STREAMMING, ON)) != 0)
		goto fail;
	if((ret = mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;

	printf("[example_media_geo_rtsp2] init done\n\r");
	//printf("Available heap %d\n\r", xPortGetFreeHeapSize());
	
#if USE_MCMD_CB==0
	// Video Streaming
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
	// Audio
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON);
	printf("src streaming on\n\r");
	mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_RUN);
#endif
	
	//int toggle = 1;
	// TODO: exit condition or signal
	while(1)
	{
		vTaskDelay(10000);	
	}
	
fail:
	mmf_sink_close(msink_rtsp2_ctx);
	mmf_source_close(msrc_geo_ctx);
	vTaskDelete(NULL);
}

void example_media_geo_rtsp2(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_geo_rtsp2_main, ((const char*)"example_media_geo_rtsp2_main"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_geo_rtsp2_main: Create Task Error\n");
	}
}

