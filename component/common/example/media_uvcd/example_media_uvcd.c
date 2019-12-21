 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "module_isp.h"
#include "module_h264.h"
#include "module_jpeg.h"
#include "module_uvcd.h"

#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_simo.h"
#include "mmf2_miso.h"
#include "mmf2_mimo.h"
#include "example_media_uvcd.h"

#include "video_common_api.h"
#include "sensor_service.h"
#include "example_media_uvcd.h"
   
#if UVCD_TUNNING_MODE
   
#define ENABLE_LDC 1
//------------------------------------------------------------------------------
// Parameter for modules
//------------------------------------------------------------------------------

//STEP 1 ISP->UVCD 2 ISP->H264->UVCD 3 ISP->JPEG->UVCD
mm_context_t* uvcd_ctx       = NULL;
mm_context_t* isp_ctx        = NULL;
mm_context_t* h264_ctx       = NULL;
mm_context_t* jpeg_ctx       = NULL;
mm_siso_t* siso_isp_uvcd     = NULL;
mm_siso_t* siso_isp_h264     = NULL;
mm_siso_t* siso_h264_uvcd    = NULL;
mm_siso_t* siso_isp_jpeg     = NULL;
mm_siso_t* siso_jpeg_uvcd    = NULL;

extern struct uvc_format *uvc_format_ptr;

struct uvc_format *uvc_format_local = NULL;;

#define WIDTH  1920
#define HEIGHT 1080

#define BLOCK_SIZE						1024*10
#define FRAME_SIZE						(WIDTH*HEIGHT/2)
#define BUFFER_SIZE						(FRAME_SIZE*3)

#define FORMAT_NV16 ISP_FORMAT_YUV422_SEMIPLANAR//ISP_FORMAT_YUV422_SEMIPLANAR//ISP_FORMAT_BAYER_PATTERN

#define YUY2_FPS 2
isp_params_t isp_params = {
	.width    = 1920, 
	.height   = 1080,
	.fps      = YUY2_FPS,
	.slot_num = 2,
	.buff_num = 4,
	.format   = FORMAT_NV16,
	.bayer_type = BAYER_TYPE_BEFORE_BLC
};

h264_params_t h264_params = {
	.width          = 1920,
	.height         = 1080,
	.bps            = 2*1024*1024,
	.fps            = 30,
	.gop            = 30,
	.rc_mode        = RC_MODE_H264CBR,
	.mem_total_size = BUFFER_SIZE,
	.mem_block_size = BLOCK_SIZE,
	.mem_frame_size = FRAME_SIZE
};

jpeg_params_t jpeg_params = {
	.width          = 1920,
	.height         = 1080,
	.level          = 5,
	.fps            = 15,
	.mem_total_size = BUFFER_SIZE,
	.mem_block_size = BLOCK_SIZE,
	.mem_frame_size = FRAME_SIZE
};

//int switch_isp_raw = 0; //0 SETUP FINISH 1: YUV420 2: YUV422 3:Bayer
//static int switch_ldc = 0;
void example_media_uvcd_init(void)
{
#if CONFIG_LIGHT_SENSOR
	init_sensor_service();
#else
	ir_cut_init(NULL);
	ir_cut_enable(1);
#endif
         //isp_ctx_t* ctx = (isp_ctx_t*)p;
        isp_ctx_t* ctx = NULL;
        
        uvc_format_ptr = (struct uvc_format *)malloc(sizeof(struct uvc_format));
        memset(uvc_format_ptr, 0, sizeof(struct uvc_format));
        
        uvc_format_local = (struct uvc_format *)malloc(sizeof(struct uvc_format));
        memset(uvc_format_local, 0, sizeof(struct uvc_format));
        
        rtw_init_sema(&uvc_format_ptr->uvcd_change_sema,0);
        
        uvc_format_ptr->format = FORMAT_TYPE_YUY2;
        uvc_format_ptr->fps = isp_params.fps;
        uvc_format_ptr->height = isp_params.height;
        uvc_format_ptr->width = isp_params.width;
        uvc_format_ptr->isp_format = FORMAT_NV16;
        uvc_format_ptr->ldc = 1;
        
        uvc_format_local->format = FORMAT_TYPE_YUY2;
        uvc_format_local->fps = isp_params.fps;
        uvc_format_local->height = isp_params.height;
        uvc_format_local->width = isp_params.width;
        uvc_format_local->isp_format = FORMAT_NV16;
        uvc_format_local->ldc = 1;
        
	isp_ctx = mm_module_open(&isp_module);
	if(isp_ctx){
		mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS, (int)&isp_params);
		mm_module_ctrl(isp_ctx, MM_CMD_SET_QUEUE_LEN, isp_params.buff_num);
		mm_module_ctrl(isp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        ctx = (isp_ctx_t*)isp_ctx->priv;
        
        h264_ctx = mm_module_open(&h264_module);
	if(h264_ctx){
		mm_module_ctrl(h264_ctx, CMD_H264_SET_PARAMS, (int)&h264_params);
		mm_module_ctrl(h264_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		mm_module_ctrl(h264_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
#if !CUSTOMER_AMEBAPRO_ADVANCE
        jpeg_ctx = mm_module_open(&jpeg_module);
	if(jpeg_ctx){
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_SET_PARAMS, (int)&jpeg_params);
		mm_module_ctrl(jpeg_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		mm_module_ctrl(jpeg_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_INIT_MEM_POOL, 0);
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_APPLY, 0);
	}else{
		rt_printf("JPEG open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
#endif
        uvcd_ctx = mm_module_open(&uvcd_module);
      //  struct uvc_dev *uvc_ctx = (struct uvc_dev *)uvcd_ctx->priv;
        
	if(uvcd_ctx){
		//mm_module_ctrl(uvcd_ctx, CMD_RTSP2_SET_APPLY, 0);
		//mm_module_ctrl(uvcd_ctx, CMD_RTSP2_SET_STREAMMING, ON);
	}else{
		rt_printf("uvcd open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
          
        siso_isp_uvcd = siso_create();
	if(siso_isp_uvcd){
		siso_ctrl(siso_isp_uvcd, MMIC_CMD_ADD_INPUT, (uint32_t)isp_ctx, 0);
		siso_ctrl(siso_isp_uvcd, MMIC_CMD_ADD_OUTPUT, (uint32_t)uvcd_ctx, 0);
		siso_start(siso_isp_uvcd);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        

        siso_isp_h264 = siso_create();
        
        if(siso_isp_h264){
		siso_ctrl(siso_isp_h264, MMIC_CMD_ADD_INPUT, (uint32_t)isp_ctx, 0);
		siso_ctrl(siso_isp_h264, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_ctx, 0);
		//siso_start(siso_isp_h264);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        siso_isp_jpeg = siso_create();
        
        if(siso_isp_jpeg){
#if !CUSTOMER_AMEBAPRO_ADVANCE
		siso_ctrl(siso_isp_jpeg, MMIC_CMD_ADD_INPUT, (uint32_t)isp_ctx, 0);
		siso_ctrl(siso_isp_jpeg, MMIC_CMD_ADD_OUTPUT, (uint32_t)jpeg_ctx, 0);
#endif
		//siso_start(siso_isp_h264);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        siso_h264_uvcd = siso_create();
  
        if(siso_h264_uvcd){
		siso_ctrl(siso_h264_uvcd, MMIC_CMD_ADD_INPUT, (uint32_t)h264_ctx, 0);
		siso_ctrl(siso_h264_uvcd, MMIC_CMD_ADD_OUTPUT, (uint32_t)uvcd_ctx, 0);
		//siso_start(siso_h264_uvcd);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        siso_jpeg_uvcd = siso_create();
  
        if(siso_jpeg_uvcd){
#if !CUSTOMER_AMEBAPRO_ADVANCE
		siso_ctrl(siso_jpeg_uvcd, MMIC_CMD_ADD_INPUT, (uint32_t)jpeg_ctx, 0);
		siso_ctrl(siso_jpeg_uvcd, MMIC_CMD_ADD_OUTPUT, (uint32_t)uvcd_ctx, 0);
#endif
		//siso_start(siso_h264_uvcd);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
#if 0
        while(1){
                vTaskDelay(1000);
                if(switch_isp_raw){
                    if(uvc_format_local->format == FORMAT_TYPE_YUY2){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          //isp_params.format = switch_isp_raw;
                          siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_h264_uvcd);
                          siso_pause(siso_jpeg_uvcd);
                          //mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          //extern void isp_ldc_switch(int id,int format,int width,int height,int enable);
                          //switch_ldc++;
                          //isp_ldc_switch(isp_params.stream_id,isp_params.format,isp_params.width,isp_params.height,switch_ldc%2);
                          //mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          isp_stream_apply(ctx->stream);
                          extern void isp_ldc_switch(int id,int format,int width,int height,int enable);
                          switch_ldc++;
                          isp_ldc_switch(isp_params.stream_id,isp_params.format,isp_params.width,isp_params.height,switch_ldc%2);
                          mm_module_ctrl(isp_ctx,CMD_ISP_STREAM_START,0);
                          siso_resume(siso_isp_uvcd);
                          printf("siso_resume(siso_isp_uvcd) %d\r\n",switch_ldc%2);
                          switch_isp_raw = 0;
                    }    
                }
        }
#endif        
        
        while(1){
                rtw_down_sema(&uvc_format_ptr->uvcd_change_sema);
                printf("f:%d h:%d s:%d w:%d\r\n",uvc_format_ptr->format,uvc_format_ptr->height,uvc_format_ptr->state,uvc_format_ptr->width);
                if((uvc_format_local->format != uvc_format_ptr->format) || (uvc_format_local->width != uvc_format_ptr->width) || (uvc_format_local->height != uvc_format_ptr->height)){
                    printf("change f:%d h:%d s:%d w:%d\r\n",uvc_format_ptr->format,uvc_format_ptr->height,uvc_format_ptr->state,uvc_format_ptr->width);
                    if(uvc_format_ptr->format == FORMAT_TYPE_YUY2){
                        uvc_format_local->fps = YUY2_FPS;
                        isp_params.format = FORMAT_NV16;
                        isp_params.fps = YUY2_FPS;
#if !CUSTOMER_AMEBAPRO_ADVANCE
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_NV12){
                        uvc_format_local->fps = 15;
                        isp_params.format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        isp_params.fps = 15;
#endif
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_H264){
                        uvc_format_local->format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        uvc_format_local->fps = 30;
                        isp_params.format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        isp_params.fps = 30;
#if !CUSTOMER_AMEBAPRO_ADVANCE
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_MJPEG){
                        uvc_format_local->format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        uvc_format_local->fps = 15;
                        isp_params.format = ISP_FORMAT_YUV420_SEMIPLANAR;
                        isp_params.fps = 15;
#endif
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_BAYER){
                        uvc_format_local->fps = YUY2_FPS;
                        isp_params.format = ISP_FORMAT_BAYER_PATTERN;
                        isp_params.fps = YUY2_FPS;
                    }
                    
                    uvc_format_local->format = uvc_format_ptr->format;
                    uvc_format_local->width = uvc_format_ptr->width;
                    uvc_format_local->height = uvc_format_ptr->height;
                    uvc_format_local->bayer_type = uvc_format_ptr->bayer_type;
                    isp_params.width = uvc_format_local->width;
                    isp_params.height = uvc_format_local->height;
                    isp_params.bayer_type = uvc_format_local->bayer_type;
                    if((uvc_format_ptr->format == FORMAT_TYPE_YUY2) || (uvc_format_ptr->format == FORMAT_TYPE_NV12) || (uvc_format_ptr->format == FORMAT_TYPE_BAYER)){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          
                          siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_h264_uvcd);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_jpeg_uvcd);
#endif
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          siso_resume(siso_isp_uvcd);
                          printf("siso_resume(siso_isp_uvcd)\r\n");
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_H264){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          
                          siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_h264_uvcd);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_jpeg_uvcd);
#endif
                          
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          siso_resume(siso_isp_h264);
                          siso_resume(siso_h264_uvcd);
                          printf("siso_resume(siso_isp_h264_uvcd)\r\n");
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_MJPEG){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_h264_uvcd);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_jpeg_uvcd);
#endif
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                          siso_resume(siso_isp_jpeg);
                          siso_resume(siso_jpeg_uvcd);
#endif
                          printf("siso_resume(siso_isp_jpeg_uvcd)\r\n");
                    }
                }
                if(uvc_format_local->format == FORMAT_TYPE_YUY2 && uvc_format_ptr->format == FORMAT_TYPE_YUY2){
                    if(uvc_format_local->isp_format != uvc_format_ptr->isp_format){
                          uvc_format_local->isp_format = uvc_format_ptr->isp_format;
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          isp_params.format = uvc_format_local->isp_format;
                          siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_h264_uvcd);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_jpeg_uvcd);
#endif
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          siso_resume(siso_isp_uvcd);
                          printf("siso_resume(siso_isp_uvcd)\r\n");
                    }
                }
                if(uvc_format_local->ldc != uvc_format_ptr->ldc){
#if 0
                    extern void isp_ldc_switch(int id,int format,int width,int height,int enable);
                    uvc_format_local->ldc = uvc_format_ptr->ldc;
                    isp_ldc_switch(0,uvc_format_local->isp_format,uvc_format_local->width,uvc_format_local->height,uvc_format_ptr->ldc);
#else
                    mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);

                    siso_pause(siso_isp_uvcd);
                    siso_pause(siso_isp_h264);
                    siso_pause(siso_h264_uvcd);
#if !CUSTOMER_AMEBAPRO_ADVANCE
                    siso_pause(siso_isp_jpeg);
                    siso_pause(siso_jpeg_uvcd);
#endif

                    isp_stream_apply(ctx->stream);
                    extern void isp_ldc_switch(int id,int format,int width,int height,int enable);
                    uvc_format_local->ldc = uvc_format_ptr->ldc;
#if ENABLE_LDC
                    extern int mcu_init_wait_cmd_done_polling();
                    mcu_init_wait_cmd_done_polling();
                    isp_ldc_switch(0,uvc_format_local->isp_format,uvc_format_local->width,uvc_format_local->height,uvc_format_ptr->ldc);
#endif
                    mm_module_ctrl(isp_ctx,CMD_ISP_STREAM_START,0);
                    if(uvc_format_ptr->format == FORMAT_TYPE_YUY2 || uvc_format_ptr->format == FORMAT_TYPE_NV12){
                        siso_resume(siso_isp_uvcd);
                        printf("siso_resume(siso_isp_uvcd)\r\n");
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_H264){
                        siso_resume(siso_isp_h264);
                        siso_resume(siso_h264_uvcd);
                        printf("siso_resume(siso_isp_h264_uvcd)\r\n");;
#if !CUSTOMER_AMEBAPRO_ADVANCE
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_MJPEG){
                        siso_resume(siso_isp_jpeg);
                        siso_resume(siso_jpeg_uvcd);
                        printf("siso_resume(siso_isp_jpeg_uvcd)\r\n");
#endif
                    }else{
                        printf("siso_resume format = %d\r\n",uvc_format_ptr->format);
                    }
#endif
                }
        }

mmf2_example_uvcd_fail:
	
	return;
}

void example_media_uvcd_main(void *param)
{
	example_media_uvcd_init();
	// TODO: exit condition or signal
	while(1)
	{
		vTaskDelay(1000);
	}
	
	//vTaskDelete(NULL);
}

void example_media_uvcd(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_uvcd_main, ((const char*)"example_media_uvcd_main"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_two_source_main: Create Task Error\n");
	}
}
#else
//------------------------------------------------------------------------------
// Parameter for modules
//------------------------------------------------------------------------------

//STEP 1 ISP->UVCD 2 ISP->H264->UVCD 3 ISP->JPEG->UVCD
mm_context_t* uvcd_ctx       = NULL;
mm_context_t* isp_ctx        = NULL;
mm_context_t* h264_ctx       = NULL;
mm_context_t* jpeg_ctx       = NULL;
mm_siso_t* siso_isp_uvcd     = NULL;
mm_siso_t* siso_isp_h264     = NULL;
mm_siso_t* siso_h264_uvcd    = NULL;
mm_siso_t* siso_isp_jpeg     = NULL;
mm_siso_t* siso_jpeg_uvcd    = NULL;

extern struct uvc_format *uvc_format_ptr;

struct uvc_format *uvc_format_local = NULL;;

#define WIDTH  		1920
#define HEIGHT 		1080
#define FPS	   		30
#define BITRATE 	2*1024*1024
#define JPEG_LEVEL 	5

#define BLOCK_SIZE						1024*10
#define FRAME_SIZE						(WIDTH*HEIGHT/2)
#define BUFFER_SIZE						(FRAME_SIZE*3)

#define FORMAT_NV16 ISP_FORMAT_YUV422_SEMIPLANAR//ISP_FORMAT_YUV422_SEMIPLANAR//ISP_FORMAT_BAYER_PATTERN

isp_params_t isp_params = {
	.width    = WIDTH, 
	.height   = HEIGHT,
	.fps      = FPS,
	.slot_num = 2,
	.buff_num = 4,
	.format   = ISP_FORMAT_YUV420_SEMIPLANAR
};

h264_params_t h264_params = {
	.width          = WIDTH,
	.height         = HEIGHT,
	.bps            = BITRATE,
	.fps            = FPS,
	.gop            = FPS,
	.rc_mode        = RC_MODE_H264CBR,
	.mem_total_size = BUFFER_SIZE,
	.mem_block_size = BLOCK_SIZE,
	.mem_frame_size = FRAME_SIZE
};

jpeg_params_t jpeg_params = {
	.width          = WIDTH,
	.height         = HEIGHT,
	.level          = JPEG_LEVEL,
	.fps            = FPS,
	.mem_total_size = BUFFER_SIZE,
	.mem_block_size = BLOCK_SIZE,
	.mem_frame_size = FRAME_SIZE
};

//int switch_isp_raw = 0; //0 SETUP FINISH 1: YUV420 2: YUV422 3:Bayer
//static int switch_ldc = 0;
void example_media_uvcd_init(void)
{
#if CONFIG_LIGHT_SENSOR
	init_sensor_service();
#else
	ir_cut_init(NULL);
	ir_cut_enable(1);
#endif
         //isp_ctx_t* ctx = (isp_ctx_t*)p;
        isp_ctx_t* ctx = NULL;
        
        uvc_format_ptr = (struct uvc_format *)malloc(sizeof(struct uvc_format));
        memset(uvc_format_ptr, 0, sizeof(struct uvc_format));
        
        uvc_format_local = (struct uvc_format *)malloc(sizeof(struct uvc_format));
        memset(uvc_format_local, 0, sizeof(struct uvc_format));
        
        rtw_init_sema(&uvc_format_ptr->uvcd_change_sema,0);
        
        uvc_format_ptr->format = FORMAT_TYPE_H264;
        uvc_format_ptr->fps = isp_params.fps;
        uvc_format_ptr->height = isp_params.height;
        uvc_format_ptr->width = isp_params.width;
        uvc_format_ptr->isp_format = isp_params.format;
        uvc_format_ptr->ldc = 1;
        
        uvc_format_local->format = FORMAT_TYPE_H264;
        uvc_format_local->fps = isp_params.fps;
        uvc_format_local->height = isp_params.height;
        uvc_format_local->width = isp_params.width;
        uvc_format_local->isp_format = isp_params.format;
        uvc_format_local->ldc = 1;
        
	isp_ctx = mm_module_open(&isp_module);
	if(isp_ctx){
		mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS, (int)&isp_params);
		mm_module_ctrl(isp_ctx, MM_CMD_SET_QUEUE_LEN, isp_params.buff_num);
		mm_module_ctrl(isp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        ctx = (isp_ctx_t*)isp_ctx->priv;
        
        h264_ctx = mm_module_open(&h264_module);
	if(h264_ctx){
		mm_module_ctrl(h264_ctx, CMD_H264_SET_PARAMS, (int)&h264_params);
		mm_module_ctrl(h264_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		mm_module_ctrl(h264_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
        
        jpeg_ctx = mm_module_open(&jpeg_module);
	if(jpeg_ctx){
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_SET_PARAMS, (int)&jpeg_params);
		mm_module_ctrl(jpeg_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		mm_module_ctrl(jpeg_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_INIT_MEM_POOL, 0);
		mm_module_ctrl(jpeg_ctx, CMD_JPEG_APPLY, 0);
	}else{
		rt_printf("JPEG open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}

        uvcd_ctx = mm_module_open(&uvcd_module);
      //  struct uvc_dev *uvc_ctx = (struct uvc_dev *)uvcd_ctx->priv;
        
	if(uvcd_ctx){
		//mm_module_ctrl(uvcd_ctx, CMD_RTSP2_SET_APPLY, 0);
		//mm_module_ctrl(uvcd_ctx, CMD_RTSP2_SET_STREAMMING, ON);
	}else{
		rt_printf("uvcd open fail\n\r");
		goto mmf2_example_uvcd_fail;
	}
          
        

        siso_isp_h264 = siso_create();
        
        if(siso_isp_h264){
		siso_ctrl(siso_isp_h264, MMIC_CMD_ADD_INPUT, (uint32_t)isp_ctx, 0);
		siso_ctrl(siso_isp_h264, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_ctx, 0);
		siso_start(siso_isp_h264);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_example_uvcd_fail;
		}
        
        siso_isp_jpeg = siso_create();
        
        if(siso_isp_jpeg){
		siso_ctrl(siso_isp_jpeg, MMIC_CMD_ADD_INPUT, (uint32_t)isp_ctx, 0);
		siso_ctrl(siso_isp_jpeg, MMIC_CMD_ADD_OUTPUT, (uint32_t)jpeg_ctx, 0);
		//siso_start(siso_isp_h264);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_example_uvcd_fail;
		}
        
        siso_h264_uvcd = siso_create();
  
        if(siso_h264_uvcd){
		siso_ctrl(siso_h264_uvcd, MMIC_CMD_ADD_INPUT, (uint32_t)h264_ctx, 0);
		siso_ctrl(siso_h264_uvcd, MMIC_CMD_ADD_OUTPUT, (uint32_t)uvcd_ctx, 0);
		siso_start(siso_h264_uvcd);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_example_uvcd_fail;
		}
        
        siso_jpeg_uvcd = siso_create();
  
        if(siso_jpeg_uvcd){
		siso_ctrl(siso_jpeg_uvcd, MMIC_CMD_ADD_INPUT, (uint32_t)jpeg_ctx, 0);
		siso_ctrl(siso_jpeg_uvcd, MMIC_CMD_ADD_OUTPUT, (uint32_t)uvcd_ctx, 0);
		//siso_start(siso_jpeg_uvcd);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_example_uvcd_fail;
		}      
        
        while(1){
                rtw_down_sema(&uvc_format_ptr->uvcd_change_sema);
                printf("f:%d h:%d s:%d w:%d\r\n",uvc_format_ptr->format,uvc_format_ptr->height,uvc_format_ptr->state,uvc_format_ptr->width);
                if((uvc_format_local->format != uvc_format_ptr->format) || (uvc_format_local->width != uvc_format_ptr->width) || (uvc_format_local->height != uvc_format_ptr->height)){
                    printf("change f:%d h:%d s:%d w:%d\r\n",uvc_format_ptr->format,uvc_format_ptr->height,uvc_format_ptr->state,uvc_format_ptr->width);
                    
                    
                    uvc_format_local->format = uvc_format_ptr->format;
                    uvc_format_local->width = uvc_format_ptr->width;
                    uvc_format_local->height = uvc_format_ptr->height;
                    isp_params.width = uvc_format_local->width;
                    isp_params.height = uvc_format_local->height;
                    if(uvc_format_ptr->format == FORMAT_TYPE_H264){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          
                          //siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_h264_uvcd);
                          siso_pause(siso_jpeg_uvcd);
                          
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          
                          //
                          h264_params.width = isp_params.width;
                          h264_params.height =isp_params.height;
                          mm_module_ctrl(h264_ctx, CMD_H264_SET_PARAMS,(int)&h264_params);
                          mm_module_ctrl(h264_ctx, CMD_H264_UPDATE, 0);
                          //
                          siso_resume(siso_isp_h264);
                          siso_resume(siso_h264_uvcd);
                          printf("siso_resume(siso_isp_h264_uvcd)\r\n");
                    }else if(uvc_format_ptr->format == FORMAT_TYPE_MJPEG){
                          mm_module_ctrl(isp_ctx, CMD_ISP_STREAM_STOP, 0);
                          //siso_pause(siso_isp_uvcd);
                          siso_pause(siso_isp_h264);
                          siso_pause(siso_isp_jpeg);
                          siso_pause(siso_h264_uvcd);
                          siso_pause(siso_jpeg_uvcd);
                          jpeg_params.width = isp_params.width;
                          jpeg_params.height =isp_params.height;
                          mm_module_ctrl(isp_ctx, CMD_ISP_SET_PARAMS,(int)&isp_params);
                          mm_module_ctrl(isp_ctx, CMD_ISP_UPDATE, 0);
                          mm_module_ctrl(jpeg_ctx, CMD_JPEG_SET_PARAMS,(int)&jpeg_params);
                          mm_module_ctrl(jpeg_ctx, CMD_JPEG_UPDATE, 0);
                          siso_resume(siso_isp_jpeg);
                          siso_resume(siso_jpeg_uvcd);
                          printf("siso_resume(siso_isp_jpeg_uvcd)\r\n");
                    }
                }
        }

mmf2_example_uvcd_fail:
	
	return;
}

void example_media_uvcd_main(void *param)
{
	example_media_uvcd_init();
	// TODO: exit condition or signal
	while(1)
	{
		vTaskDelay(1000);
	}
	
	//vTaskDelete(NULL);
}

void example_media_uvcd(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_media_uvcd_main, ((const char*)"example_media_uvcd_main"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_media_two_source_main: Create Task Error\n");
	}
}
#endif