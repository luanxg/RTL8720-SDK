#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include <platform_opts.h>

#if CONFIG_MEDIA_H264_TO_SDCARD
#include "example_media_h264_to_sdcard.h"
#include "video_common_api.h"
#include "h264_encoder.h"
#include "isp_api.h"
#include "h264_api.h"
#include "fatfs_sdcard_api.h"
#include "mmf2_dbg.h"
#include "sensor.h"

#define ISP_SW_BUF_NUM	4
#define FATFS_BUF_SIZE	(32*1024)
#define SD_CARD_QUEUE_DEPTH (20)

struct h264_to_sd_card_def_setting def_setting = {
	.height = VIDEO_720P_HEIGHT,
	.width = VIDEO_720P_WIDTH,
	.rcMode = H264_RC_MODE_CBR,
	.bitrate = 2*1024*1024,
	.fps = 30,
	.gopLen = 30,
	.encode_frame_cnt = 300,
	.output_buffer_size = VIDEO_720P_HEIGHT*VIDEO_720P_WIDTH,
	.isp_stream_id = 0,
	.isp_hw_slot = 2,
	.isp_format = ISP_FORMAT_YUV420_SEMIPLANAR,
	.fatfs_filename = "h264_to_sdcard.h264",
};

typedef struct isp_s{
	isp_stream_t* stream;
	
	isp_cfg_t cfg;
	
	isp_buf_t buf_item[ISP_SW_BUF_NUM];
	xQueueHandle output_ready;
	xQueueHandle output_recycle;
}isp_t;

xQueueHandle sd_card_queue;
u8 fatfs_thread_run;

void stop_fatfs_thread()
{
	fatfs_thread_run = 0;
}

u8 fatfs_thread_running()
{
	if (fatfs_thread_run)
		return 1;
	else 
		return 0;
}


void fatfs_thread(void *param)
{
	int ret = 0;
	VIDEO_BUFFER video_buf;
	printf("[FATFS] fatfs_sd_init\n\r");
	// [1][FATFS] fatfs_sd_init
	printf("\n\rfatfs_sd_init\n\r");
	ret = fatfs_sd_init();
	if (ret != 0) {
		printf("\n\rfatfs_sd_init err %d\n\r",ret);
		goto exit;
	}
	sd_card_queue = xQueueCreate(SD_CARD_QUEUE_DEPTH, sizeof(VIDEO_BUFFER));
	xQueueReset(sd_card_queue);
	// [2][FATFS] fatfs_sd_create_write_buf
	ret = fatfs_sd_create_write_buf(FATFS_BUF_SIZE);
	if (ret != 0) {
		printf("\n\fatfs_sd_create_write_buf err %d\n\r",ret);
		goto exit;
	}
	
	printf("[FATFS] fatfs_sd_open_file: %s\n\r",def_setting.fatfs_filename);
	// [3][FATFS] fatfs_sd_open_file
	fatfs_sd_open_file(def_setting.fatfs_filename);
	
	fatfs_thread_run = 1;
	while (fatfs_thread_run || (uxQueueSpacesAvailable(sd_card_queue) != SD_CARD_QUEUE_DEPTH)) {
		xQueueReceive(sd_card_queue, (void *)&video_buf, 0xFFFFFFFF);
		fatfs_sd_write((char*)video_buf.output_buffer,video_buf.output_size);
		if (video_buf.output_buffer != NULL)
			free(video_buf.output_buffer);
	}
	
	fatfs_sd_flush_buf();
	// [13][FATFS] fatfs_sd_close_file
	printf("[FATFS] fatfs_sd_close_file: %s\n\r",def_setting.fatfs_filename);
	fatfs_sd_close_file();
exit:
	vTaskDelete(NULL);
}

void isp_frame_cb(void* p)
{
	BaseType_t xTaskWokenByReceive = pdFALSE;
	BaseType_t xHigherPriorityTaskWoken;
	
	isp_t* ctx = (isp_t*)p;
	isp_info_t* info = &ctx->stream->info;
	isp_cfg_t *cfg = &ctx->cfg;
	isp_buf_t buf;
	isp_buf_t queue_item;
	
	int is_output_ready = 0;
	
	u32 timestamp = xTaskGetTickCountFromISR();
	
	if(info->isp_overflow_flag == 0){
		is_output_ready = xQueueReceiveFromISR(ctx->output_recycle, &buf, &xTaskWokenByReceive) == pdTRUE;
	}else{
		info->isp_overflow_flag = 0;
		ISP_DBG_ERROR("isp overflow = %d\r\n",cfg->isp_id);
	}
	
	if(is_output_ready){
		isp_handle_buffer(ctx->stream, &buf, MODE_EXCHANGE);
		xQueueSendFromISR(ctx->output_ready, &buf, &xHigherPriorityTaskWoken);	
	}else{
		isp_handle_buffer(ctx->stream, NULL, MODE_SKIP);
	}
	if( xHigherPriorityTaskWoken || xTaskWokenByReceive)
		taskYIELD ();
}

void example_media_h264_to_sdcard_thread(void *param)
{
	int ret;
	VIDEO_BUFFER video_buf;
	u8 start_recording = 0;
    isp_init_cfg_t isp_init_cfg;
	isp_t isp_ctx;
	
	if(xTaskCreate(fatfs_thread, ((const char*)"fatfs_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(fatfs_thread) failed", __FUNCTION__);
	
	while (!fatfs_thread_running()) {
		vTaskDelay(100);
	}
	
	printf("[H264] init video related settings\n\r");
	// [4][H264] init video related settings
	
	memset(&isp_init_cfg, 0, sizeof(isp_init_cfg));
	isp_init_cfg.pin_idx = ISP_PIN_IDX;
	isp_init_cfg.clk = SENSOR_CLK_USE;
	isp_init_cfg.ldc = LDC_STATE;
    isp_init_cfg.fps = SENSOR_FPS;
	
	video_subsys_init(&isp_init_cfg);
#if CONFIG_LIGHT_SENSOR
	init_sensor_service();
#else
	ir_cut_init(NULL);
	ir_cut_enable(1);
#endif
	
	printf("[H264] create encoder\n\r");
	// [5][H264] create encoder
	struct h264_context* h264_ctx;
	ret = h264_create_encoder(&h264_ctx);
	if (ret != H264_OK) {
		printf("\n\rh264_create_encoder err %d\n\r",ret);
		goto exit;
	}

	printf("[H264] get & set encoder parameters\n\r");
	// [6][H264] get & set encoder parameters
	struct h264_parameter h264_parm;
	ret = h264_get_parm(h264_ctx, &h264_parm);
	if (ret != H264_OK) {
		printf("\n\rh264_get_parmeter err %d\n\r",ret);
		goto exit;
	}
	
	h264_parm.height = def_setting.height;
	h264_parm.width = def_setting.width;
	h264_parm.rcMode = def_setting.rcMode;
	h264_parm.bps = def_setting.bitrate;
	h264_parm.ratenum = def_setting.fps;
	h264_parm.gopLen = def_setting.gopLen;
	
	ret = h264_set_parm(h264_ctx, &h264_parm);
	if (ret != H264_OK) {
		printf("\n\rh264_set_parmeter err %d\n\r",ret);
		goto exit;
	}
	
	printf("[H264] init encoder\n\r");
	// [7][H264] init encoder
	ret = h264_init_encoder(h264_ctx);
	if (ret != H264_OK) {
		printf("\n\rh264_init_encoder_buffer err %d\n\r",ret);
		goto exit;
	}
	
	// [8][ISP] init ISP
	printf("[ISP] init ISP\n\r");
	memset(&isp_ctx,0,sizeof(isp_ctx));
	isp_ctx.output_ready = xQueueCreate(ISP_SW_BUF_NUM, sizeof(isp_buf_t));
	isp_ctx.output_recycle = xQueueCreate(ISP_SW_BUF_NUM, sizeof(isp_buf_t));
	
	isp_ctx.cfg.isp_id = def_setting.isp_stream_id;
	isp_ctx.cfg.format = def_setting.isp_format;
	isp_ctx.cfg.width = def_setting.width;
	isp_ctx.cfg.height = def_setting.height;
	isp_ctx.cfg.fps = def_setting.fps;
	isp_ctx.cfg.hw_slot_num = def_setting.isp_hw_slot;
	
	isp_ctx.stream = isp_stream_create(&isp_ctx.cfg);
	
	isp_stream_set_complete_callback(isp_ctx.stream, isp_frame_cb, (void*)&isp_ctx);
	
	for (int i=0; i<ISP_SW_BUF_NUM; i++ ) {
		unsigned char *ptr =(unsigned char *) malloc(def_setting.width*def_setting.height*3/2);
		if (ptr==NULL) {
			printf("[ISP] Allocate isp buffer[%d] failed\n\r",i);
			while(1);
		}
		isp_ctx.buf_item[i].slot_id = i;
		isp_ctx.buf_item[i].y_addr = (uint32_t) ptr;
		isp_ctx.buf_item[i].uv_addr = isp_ctx.buf_item[i].y_addr + def_setting.width*def_setting.height;
		
		if (i<def_setting.isp_hw_slot) {
			// config hw slot
			//printf("\n\rconfig hw slot[%d] y=%x, uv=%x\n\r",i,isp_ctx.buf_item[i].y_addr,isp_ctx.buf_item[i].uv_addr);
			isp_handle_buffer(isp_ctx.stream, &isp_ctx.buf_item[i], MODE_SETUP);
		}
		else {
			// extra sw buffer
			//printf("\n\rextra sw buffer[%d] y=%x, uv=%x\n\r",i,isp_ctx.buf_item[i].y_addr,isp_ctx.buf_item[i].uv_addr);
			if(xQueueSend(isp_ctx.output_recycle, &isp_ctx.buf_item[i], 0)!= pdPASS) {
				printf("[ISP] Queue send fail\n\r");
				while(1);
			}
		}
	}
	
	isp_stream_apply(isp_ctx.stream);
	isp_stream_start(isp_ctx.stream);
	
			printf("\n\rStart recording %d frames...\n\r",def_setting.encode_frame_cnt);
			int enc_cnt = 0;
			while (enc_cnt < def_setting.encode_frame_cnt) {
				// [9][ISP] get isp data
				isp_buf_t isp_buf;
				if(xQueueReceive(isp_ctx.output_ready, &isp_buf, 10) != pdTRUE) {
					continue;
				}
				
				// [10][H264] encode data
				video_buf.output_buffer_size = def_setting.output_buffer_size;
				video_buf.output_buffer = malloc(video_buf.output_buffer_size);
				if (video_buf.output_buffer== NULL) {
					printf("Allocate output buffer fail\n\r");
					continue;
				}
				ret = h264_encode_frame(h264_ctx, &isp_buf, &video_buf);
				if (ret != H264_OK) {
					printf("\n\rh264_encode_frame err %d\n\r",ret);
					if (video_buf.output_buffer != NULL)
						free(video_buf.output_buffer);
					continue;
				}
				enc_cnt++;
				
				if (enc_cnt % def_setting.fps == 0) {
					printf("%d / %d ...\n\r",enc_cnt,def_setting.encode_frame_cnt);
				}
				
				// [11][ISP] put back isp buffer
				xQueueSend(isp_ctx.output_recycle, &isp_buf, 10);
				
				// [12][FATFS] write encoded data into sdcard
				if (start_recording) {
					xQueueSend(sd_card_queue, (void *)&video_buf, 0xFFFFFFFF);
				}
				else {
					if (h264_is_i_frame(video_buf.output_buffer)) {
						start_recording = 1;
						xQueueSend(sd_card_queue, (void *)&video_buf, 0xFFFFFFFF);
					}
					else {
						if (video_buf.output_buffer != NULL)
							free(video_buf.output_buffer);
					}
				}
			}
	
exit:
	stop_fatfs_thread();
	isp_stream_stop(isp_ctx.stream);
	xQueueReset(isp_ctx.output_ready);
	xQueueReset(isp_ctx.output_recycle);
	for (int i=0; i<ISP_SW_BUF_NUM; i++ ) {
		unsigned char* ptr = (unsigned char*) isp_ctx.buf_item[i].y_addr;
		if (ptr) 
			free(ptr);
	}
	vTaskDelete(NULL);
}

void example_media_h264_to_sdcard(void)
{
	if(xTaskCreate(example_media_h264_to_sdcard_thread, ((const char*)"example_media_h264_to_sdcard_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_media_h264_to_sdcard_thread) failed", __FUNCTION__);
}
#endif //CONFIG_MEDIA_H264_TO_SDCARD