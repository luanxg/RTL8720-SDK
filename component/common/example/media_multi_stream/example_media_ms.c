#include "FreeRTOS.h"
#include "task.h"
#include "example_media_ms.h"
#include "mmf_command.h"
#include "mmf_source.h"
#include "mmf_sink.h"
#include "mmf_source_modules/mmf_source_geo.h"
#include "mmf_sink_modules/mmf_sink_rtsp2.h"
#include "mmf_sink_modules/mmf_sink_mp4.h"

#define USE_MCMD_CB 0
#define USE_MCMD_SOCKET 0

#define SHARE_CACHE			0
#define SHARE_REF_COUNT		1

#define CH1 0
#define CH2 1
#define CH3 2
#define CH4 3

static msrc_context		*msrc_geo_ctx = NULL;	
static msink_context	*msink_mp4_ctx = NULL;
static msink_context	*msink_rtsp2_ctx = NULL;

#if USE_MCMD_SOCKET==1
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
	
	if (req[0]=='s') {
		strcpy(rsp,"START MP4 recording");
		printf("mp4 start recording by socket command\n\r");
		mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_START,0);
	}
	else if (req[0]=='e') {
		strcpy(rsp,"STOP MP4 recording");
		printf("mp4 stop recording by socket command\n\r");
		mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_STOP,0);
	}
	else if (!strncmp(req, "n=", 2)){
		strcpy(rsp,"set MP4 Recording file name");
		printf("mp4 set recording file name [%s] by socket command\n\r",(req+2));
		mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_FILENAME,(int)(req+2));
	}	
	else {
		strcpy(rsp,"unknown....\n\r");
	}
	return strlen(rsp);
}
#endif

#if USE_MCMD_CB==1
static mcmd_context		*mcmd_ctx = NULL;

static int rtsp_start_cb (void* param) {
	printf("rtsp_start_cb()\n\r");
	mcmd_t *cmd = mcmd_cmd_create(1,1,0,0,0,0,NULL);
	mcmd_in_cmd_queue(cmd,mcmd_ctx);
	return 0;
}

static int rtsp_stop_cb (void* param) {
	printf("rtsp_stop_cb()\n\r");
	mcmd_t *cmd = mcmd_cmd_create(2,2,0,0,0,0,NULL);
	mcmd_in_cmd_queue(cmd,mcmd_ctx);
	return 0;
}

static int mp4_start_cb (void* param) {
	printf("mp4_start_cb()\n\r");
	mcmd_t *cmd = mcmd_cmd_create(3,3,0,0,0,0,NULL);
	mcmd_in_cmd_queue(cmd,mcmd_ctx);
	return 0;
}

static int mp4_stop_cb (void* param) {
	printf("mp4_stop_cb()\n\r");
	mcmd_t *cmd = mcmd_cmd_create(4,4,0,0,0,0,NULL);
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
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			printf("start CH4\n\r");
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		}
		
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON);
			printf("src streaming on\n\r");
		}
		mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_RUN);
	}
	else if(cmd->bCmd==2 && cmd->bSubCommand== 2) {	// fake: rtsp stop
		printf("\n\rFAKE: RTSP stop\n\r");
		mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_STOP);
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, OFF);
			printf("src streaming off\n\r");
		}
		// Video Streaming
		printf("stop CH1\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		// Audio
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			printf("stop CH4\n\r");
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		}
	}
	else if(cmd->bCmd==3 && cmd->bSubCommand== 3) {	// fake: storage start
		printf("\n\rFAKE: Storage start\n\r");
		// Video to Storage
		printf("start CH2\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH2);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		// Audio
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			printf("start CH4\n\r");
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		}
		
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON);
			printf("src streaming on\n\r");
		}
		mmf_source_sink_state_change(msrc_geo_ctx, &mp4_module, SINK_RUN);
	}
	else if(cmd->bCmd==4 && cmd->bSubCommand== 4) {	// fake: storage stop
		printf("\n\rFAKE: Storage stop\n\r");
		mmf_source_sink_state_change(msrc_geo_ctx, &mp4_module, SINK_STOP);
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, OFF);
			printf("src streaming off\n\r");
		}
		printf("stop CH2\n\r");
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH2);
		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		if (mmf_source_all_sink_off(msrc_geo_ctx)) {
			printf("stop CH4\n\r");
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		}
	}
	return 0;
}
#endif

void example_media_ms_main(void *param)
{
	printf("\r\nexample_media_ms_main\r\n");
	
	int ret;
	
	xQueueHandle		src_geo_qid;
	xQueueHandle		sink_rtsp2_qid;
	xQueueHandle		sink_mp4_qid;
	
	src_geo_qid = xQueueCreate(2, sizeof(exch_buf_t));
	sink_rtsp2_qid = xQueueCreate(2, sizeof(exch_buf_t));
	sink_mp4_qid = xQueueCreate(2, sizeof(exch_buf_t));
	
	msrc_output_queue_t sink_rtsp2_q_stru = {0, &rtsp2_module, sink_rtsp2_qid, SINK_STOP};
	msrc_output_queue_t sink_mp4_q_stru = {1, &mp4_module, sink_mp4_qid, SINK_STOP};
	
#if USE_MCMD_CB==1
	mcmd_ctx = mcmd_context_init();
	mcmd_ctx->cb_custom = mcmd_custom_cb;
	mcmd_queue_task_open(mcmd_ctx);
#endif
	
	//create geo source context
#if SHARE_CACHE
	if((msrc_geo_ctx = mmf_source_open(&geo_module, 2, 2, 700))==NULL)
#elif SHARE_REF_COUNT
	if((msrc_geo_ctx = mmf_source_open(&geo_module, 2, 2, 0))==NULL)
#else
	#error NO SHARE TYPE
#endif
		goto fail;
	// set geo source input/output queue
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_INPUT_QUEUE, (int)src_geo_qid);
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp2_q_stru);
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_mp4_q_stru);
	
	//create rtsp2 sink context
	if( (msink_rtsp2_ctx = mmf_sink_open(&rtsp2_module))==NULL)
		goto fail;
	// set rtsp2 sink input queue
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_INPUT_QUEUE, (int)sink_rtsp2_qid);
	
	//create mp4 sink context
	if( (msink_mp4_ctx = mmf_sink_open(&mp4_module))==NULL)
		goto fail;
	// set mp4 sink input queue
	mmf_sink_ctrl(msink_mp4_ctx, CMD_SET_INPUT_QUEUE, (int)sink_mp4_qid);
	
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
	// mp4 sink setting
	int recording_time,recording_type;
	recording_time = 60;
	recording_type = STORAGE_ALL; // STORAGE_ALL STORAGE_VIDEO STORAGE_AUDIO
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_HEIGHT, 720);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_WIDTH, 1280);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_FRAMERATE, 30);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_CHANNEL_CNT, 2);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_SAMPLERATE, 16000);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_PERIOD, recording_time); // second
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_TYPE, recording_type);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_FILENAME,(int)("AMEBA"));
#if USE_MCMD_CB==1
	// set MP4 callback for sending mmf commands (dynamically turn on/off source stream)
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_START_CB, (int) mp4_start_cb);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_STOP_CB, (int) mp4_stop_cb);
#endif
	printf("Recording parameters: duration=%d, type=%s\n\r",recording_time,
		(recording_type==STORAGE_ALL?"STORAGE_ALL":
		 recording_type==STORAGE_VIDEO?"STORAGE_VIDEO":
		 recording_type==STORAGE_AUDIO?"STORAGE_AUDIO":"unknown"));
	
	// Streaming and task on
	if((ret = mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_STREAMMING, ON)) != 0)
		goto fail;
	if((ret = mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;
	if((ret = mmf_sink_ctrl(msink_mp4_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;
	
	printf("[example_media_ms_main] init done\n\r");
	//printf("Available heap %d\n\r", xPortGetFreeHeapSize());
	
#if USE_MCMD_SOCKET==1
	// create mmf command soket on port 16385
	// use command socket and callback function (mcmd_socket_cb) to start/stop recording mp4 into SD card
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
			printf("[example_media_ms_main] mcmd_socket task running\n\r");
		}
	} while(0);
#endif
	
#if USE_MCMD_CB==0
	// Video Streaming
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON)) != 0)
		goto fail;
	
	// Video Storage
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH2);
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON)) != 0)
		goto fail;
	
	// Audio
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON)) != 0)
		goto fail;
	if((ret = mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON)) != 0)
		goto fail;
	printf("src streaming on\n\r");
	mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_RUN);
	mmf_source_sink_state_change(msrc_geo_ctx, &mp4_module, SINK_RUN);
	
	printf("Start recording mp4 into SD card... (duration: %d)\n\r",recording_time);
	mmf_sink_ctrl(msink_mp4_ctx, CMD_MP4_SET_ST_START,0);
#endif
	
	// TODO: exit condition or signal
	while(1)
	{
		vTaskDelay(1000);
	}
	if (!msink_rtsp2_ctx->hdl_task)
		printf("msink_rtsp2_ctx->hdl_task == NULL, exit\n\r");
	if (!msink_mp4_ctx->hdl_task)
		printf("msink_mp4_ctx->hdl_task == NULL, exit\n\r");
	if (!msrc_geo_ctx->hdl_task)
		printf("msrc_geo_ctx->hdl_task == NULL, exit\n\r");
	
	mmf_sink_ctrl(msink_rtsp2_ctx, CMD_SET_STREAMMING, OFF);
	mmf_source_sink_state_change(msrc_geo_ctx, &rtsp2_module, SINK_STOP);
	mmf_source_sink_state_change(msrc_geo_ctx, &mp4_module, SINK_STOP);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH2);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
	mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
	mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, OFF);
	
fail:
	printf("\n\rclose all source/sink and exit\n\r");
	mmf_sink_close(msink_rtsp2_ctx);
	mmf_sink_close(msink_mp4_ctx);
	mmf_source_close(msrc_geo_ctx);
	vTaskDelete(NULL);
}

void example_media_ms(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_ms_main, ((const char*)"example_media_ms"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_ms_main: Create Task Error\n");
	}
}

