
/*
* uvc video capture example
*/

#include "FreeRTOS.h"
#include "task.h"
#include "example_media_video_mp4de.h"
#include "mmf_sink_modules/mmf_sink_rtsp2.h"
#include "mmf_source_modules/mmf_source_mp4de.h"
#include "mmf_command.h"
#include "mmf_source.h"
#include "mmf_sink.h"
#define MP4_VIDEO       0
#define MP4_AUDIO       0
#define MP4_ALL         1
//---------------------------------------------------------------------------------------

static mcmd_context		*mcmd_ctx = NULL;
static msrc_context		*msrc_ctx = NULL;

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


static int mcmd_custom_cb (void* param) {
	
//	printf("mcmd_custom_cb in example_media_geo_rtsp2_main\n\r");
	mcmd_t *cmd = (mcmd_t *) param;
	printf("cmd->bCmd=%d, cmd->bSubCommand=%d\n\r",cmd->bCmd,cmd->bSubCommand);
	if(cmd->bCmd==1 && cmd->bSubCommand== 1)	// fake: rtsp start
	{
		printf("\n\rFAKE: RTSP start\n\r");
		// Video Streaming
		printf("start CH1\n\r");
//		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
//		mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		// Audio
		if (mmf_source_all_sink_off(msrc_ctx)) {
			printf("start CH4\n\r");
			//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, ON);
		}

		if (mmf_source_all_sink_off(msrc_ctx)) {
			//mmf_source_ctrl(msrc_geo_ctx, CMD_SET_STREAMMING, ON);
                  	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, ON);
                        
			printf("src streaming on\n\r");
		}		
		mmf_source_sink_state_change(msrc_ctx, &rtsp2_module, SINK_RUN);
	}
	/*else if(cmd->bCmd==2 && cmd->bSubCommand== 2)	// fake: rtsp stop
	{
		printf("\n\rFAKE: RTSP stop\n\r");	
		mmf_source_sink_state_change(msrc_ctx, &rtsp2_module, SINK_STOP);
		if (mmf_source_all_sink_off(msrc_ctx)) {
			mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, OFF);
			printf("src streaming off\n\r");
		}
		// Video Streaming
		printf("stop CH1\n\r");
		//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH1);
		//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);
		// Audio
		if (mmf_source_all_sink_off(msrc_ctx)) {
			printf("stop CH4\n\r");
			//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SELECT_CHANNEL, CH4);
			//mmf_source_ctrl(msrc_geo_ctx, CMD_GEO_SET_CHANNEL, OFF);	
		}
	}
	*/
	
	return 0;	
}

void example_media_mp4de_main(void *param)
{
        int ret;
	int con = 100;
	msink_context *msink_ctx;
	//msrc_context *msrc_ctx;
	
	xQueueHandle src2sink_qid;
	xQueueHandle sink2src_qid;
	
	src2sink_qid = xQueueCreate(2, sizeof(exch_buf_t));
	sink2src_qid = xQueueCreate(2, sizeof(exch_buf_t));
        msrc_output_queue_t sink_rtsp_q_stru = {0, &rtsp2_module, src2sink_qid, SINK_STOP};
        
        mcmd_ctx = mcmd_context_init();
        mcmd_ctx->cb_custom = mcmd_custom_cb;
	mcmd_queue_task_open(mcmd_ctx);
        
        
	if((msrc_ctx = mmf_source_open(&mp4de_module, 1, 1, 0))==NULL)
		goto fail;
	
	//
#if     MP4_VIDEO 
        mmf_source_ctrl(msrc_ctx, CMD_SET_ST_TYPE,STORAGE_VIDEO);//STORAGE_ALL STORAGE_VIDEO STORAGE_AUDIO 
#elif   MP4_AUDIO
        mmf_source_ctrl(msrc_ctx, CMD_SET_ST_TYPE,STORAGE_AUDIO);//STORAGE_ALL STORAGE_VIDEO STORAGE_AUDIO 
#else
        mmf_source_ctrl(msrc_ctx, CMD_SET_ST_TYPE,STORAGE_ALL);//STORAGE_ALL STORAGE_VIDEO STORAGE_AUDIO 
#endif
        
        mmf_source_ctrl(msrc_ctx, CMD_SET_ST_FILENAME,(int)("AMEBA.mp4"));
	mmf_source_ctrl(msrc_ctx, CMD_SET_INPUT_QUEUE, (int)sink2src_qid);        
        mmf_source_ctrl(msrc_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_rtsp_q_stru);
        	mmf_source_ctrl(msrc_ctx, CMD_SET_APPLY, 0);


        
#if MP4_VIDEO
	// open and setup stream
	if( (msink_ctx = mmf_sink_open(&rtsp2_module))==NULL)
		goto fail;
        //audio will ignore these settings
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_FRAMERATE, 30);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_BITRATE, 0);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_CODEC, FMT_V_H264);        
#endif
#if MP4_AUDIO
	// open and setup stream
	if( (msink_ctx = mmf_sink_open(&rtsp_module))==NULL)
		goto fail;
        //audio will ignore these settings
	mmf_sink_ctrl(msink_ctx, CMD_SET_SAMPLERATE, 16000);
	mmf_sink_ctrl(msink_ctx, CMD_SET_CHANNEL, 2);        
	mmf_sink_ctrl(msink_ctx, CMD_SET_CODEC, FMT_A_MP4A_LATM);
#endif
#if MP4_ALL
        if( (msink_ctx = mmf_sink_open(&rtsp2_module))==NULL)
		goto fail;
        
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SELECT_CHANNEL, 0);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_CODEC, FMT_A_MP4A_LATM);
	// for MP4A 44100 stereo, video stream will ignore this setting
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_SAMPLERATE, 16000);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_CHANNEL, 2);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_APPLY, 0);
        
        mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SELECT_CHANNEL, 1);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_CODEC, FMT_V_H264);
        
        mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_FRAMERATE, 30);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_BITRATE, 0);        
        mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_FLAG,TIME_SYNC_DIS);
#endif
//apply all above setting
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_APPLY, 0);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_START_CB, (int) rtsp_start_cb);
	mmf_sink_ctrl(msink_ctx, CMD_RSTP2_SET_STOP_CB, (int) rtsp_stop_cb);
        
	mmf_sink_ctrl(msink_ctx, CMD_SET_INPUT_QUEUE, (int)src2sink_qid);
	mmf_sink_ctrl(msink_ctx, CMD_SET_OUTPUT_QUEUE, (int)sink2src_qid);       
	// Streaming and task on
	if((ret = mmf_sink_ctrl(msink_ctx, CMD_SET_STREAMMING, ON)) != 0)
		goto fail;
	if((ret = mmf_sink_ctrl(msink_ctx, CMD_SET_TASK_ON, 0)) != 0)
		goto fail;
	// open and setup media

        mmf_source_ctrl(msrc_ctx, CMD_SET_TASK_ON, 0);
    //    int toggle = 1;
	// TODO: exit condition or signal
	while(con)
	{
	  vTaskDelay(1000);	
	}
	mmf_sink_ctrl(msink_ctx, CMD_SET_STREAMMING, OFF);
	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, OFF);
	
fail:	
	mmf_sink_close(msink_ctx);
	mmf_source_close(msrc_ctx);
	vTaskDelete(NULL);
}


void example_media_mp4de(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_mp4de_main, ((const char*)"example_media_mp4de"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_mp4de_main: Create Task Error\n");
	}	
}