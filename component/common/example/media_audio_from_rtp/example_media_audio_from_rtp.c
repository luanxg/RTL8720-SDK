#include "FreeRTOS.h"
#include "task.h"
#include "example_media_audio_from_rtp.h"

#include "mmf_source.h"
#include "mmf_sink.h"
#include "mmf_source_modules/mmf_source_rtp.h"
#if CONFIG_EXAMPLE_MP3_STREAM_RTP
	#include "mmf_sink_modules/mmf_sink_mp3.h"
#else
	#include "mmf_sink_modules/mmf_sink_i2s.h"
#endif

void example_media_audio_from_rtp_main(void *param)
{
	msink_context		*msink_ctx = NULL;
	msrc_context		*msrc_ctx = NULL;
	
	xQueueHandle		src_qid;
	xQueueHandle		sink_qid;
	
	printf("\r\nexample_media_audio_from_rtp_main\r\n");
	
	src_qid = xQueueCreate(3, sizeof(exch_buf_t));
	sink_qid = xQueueCreate(3, sizeof(exch_buf_t));
	
#if CONFIG_EXAMPLE_MP3_STREAM_RTP
	msrc_output_queue_t sink_mp3_q_stru = {0, &mp3_sink_module, sink_qid, SINK_RUN};
#else
	msrc_output_queue_t sink_i2s_q_stru = {0, &i2s_sink_module, sink_qid, SINK_RUN};
#endif
	
	//create rtp source context
	if((msrc_ctx = mmf_source_open(&rtp_src_module, 1, 1, 0))==NULL)
		goto fail;
	// set source input/output queue
	mmf_source_ctrl(msrc_ctx, CMD_SET_INPUT_QUEUE, (int)src_qid);
#if CONFIG_EXAMPLE_MP3_STREAM_RTP
	mmf_source_ctrl(msrc_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_mp3_q_stru);
#else
	mmf_source_ctrl(msrc_ctx, CMD_SET_OUTPUT_QUEUE, (int)&sink_i2s_q_stru);
#endif
	
	//create sink context
#if CONFIG_EXAMPLE_MP3_STREAM_RTP
	if( (msink_ctx = mmf_sink_open(&mp3_sink_module))==NULL)
#else
	if( (msink_ctx = mmf_sink_open(&i2s_sink_module))==NULL)
#endif
		goto fail;
	// set sink input queue
	mmf_sink_ctrl(msink_ctx, CMD_SET_INPUT_QUEUE, (int)sink_qid);
	// rtp source setting
	mmf_source_ctrl(msrc_ctx, CMD_RTP_SET_PRIV_BUF, 1);
	
	// Streaming and task on
	mmf_sink_ctrl(msink_ctx, CMD_SET_STREAMMING, ON);
#if CONFIG_EXAMPLE_MP3_STREAM_RTP	
	mmf_sink_ctrl(msink_ctx, CMD_SET_TASK_ON, 512);
#else
	mmf_sink_ctrl(msink_ctx, CMD_SET_TASK_ON, 0);
#endif
	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, ON);
	mmf_source_ctrl(msrc_ctx, CMD_SET_TASK_ON, 0);
	
	while(msrc_ctx->hdl_task && msink_ctx->hdl_task)
	{
		vTaskDelay(1000);
	}
	if (!msrc_ctx->hdl_task)
		printf("msrc_ctx->hdl_task == NULL, exit\n\r");
	if (!msink_ctx->hdl_task)
		printf("msink_ctx->hdl_task == NULL, exit\n\r");
	
	mmf_source_ctrl(msrc_ctx, CMD_RTP_SET_PRIV_BUF, 0);
	mmf_source_ctrl(msrc_ctx, CMD_SET_STREAMMING, OFF);
	
fail:
	printf("\n\rclose all source/sink and exit\n\r");
	mmf_sink_close(msink_ctx);
	mmf_source_close(msrc_ctx);
	vTaskDelete(NULL);
}

void example_media_audio_from_rtp(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_audio_from_rtp_main, ((const char*)"example_media_audio_from_rtp_main"), 528, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_audio_from_rtp_main: Create Task Error\n");
	}
}

