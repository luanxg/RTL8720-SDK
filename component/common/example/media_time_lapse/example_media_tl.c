
/*
* uvc video capture example
*/
#include "example_media_tl.h"
#include "mjpeg2jpeg.h"

#if CONFIG_USE_HTTP_SERVER
#include "lwip/sockets.h"
#include <httpd/httpd.h>
#else
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include "sdio_host.h"
#include <disk_if/inc/sdcard.h>
#endif

#define CACHE_BUF_SIZE (80000)
static _mutex cache_buf_mutex = NULL;
#include "section_config.h"
SDRAM_DATA_SECTION u8 cache_buf[CACHE_BUF_SIZE];
static int cache_size = 0;
/*Creating AV Packet for conversion*/
AVFrame in_Frame;

#if CONFIG_USE_HTTP_SERVER
#define SEND_SIZE (4000)

void example_http_img_get(void)
{
  memcpy(cache_buf, in_Frame.FrameData, in_Frame.FrameLength);
  cache_size = in_Frame.FrameLength;
}

void img_get_cb(struct httpd_conn *conn)
{
  u32 send_offset = 0;
  int left_size;
  httpd_conn_dump_header(conn);
  u8 body[256] = "<HTML><BODY>"\
    "HELLO<BR>"\
    "</BODY></HTML>\0";
  //GET img page
  if(httpd_request_is_method(conn, "GET"))
  {    
    rtw_mutex_get(&cache_buf_mutex);
    example_http_img_get();
    rtw_mutex_put(&cache_buf_mutex);
    left_size = cache_size;
    printf("\n\rhere:cache_size-%d", cache_size);

    httpd_response_write_header_start(conn, "200 OK", "image/jpeg"/*"text/html"*/, /*body size*/cache_size/*strlen(body)*/);
    //httpd_response_write_header(conn, /*header name*/, /*value*/);
    httpd_response_write_header(conn, "connection", "close");
    httpd_response_write_header_finish(conn);
    printf("\n\rheader sent");

    while(left_size > 0)
    {
      if(left_size > SEND_SIZE){
        if(httpd_response_write_data(conn, /*body*/cache_buf + send_offset, /*body size*/SEND_SIZE)<0)
        {
            printf("\n\rsend http response content failed");
            break;
        }
        printf("+");
        send_offset += SEND_SIZE;
        left_size -= SEND_SIZE;
      }else{
        if(httpd_response_write_data(conn, cache_buf + send_offset, left_size)<0)
        {
            printf("\n\rsend http response content failed");
            break;
        }  
        //send_offset += left_size;
        //left_size -= left_size;
        printf("img sent\n\r");
        left_size = 0;
      } 
      vTaskDelay(1);
    }   
  }else
  {
    httpd_response_method_not_allowed(conn, NULL);
  }
  httpd_conn_close(conn);
}
#else
static _sema save_image_sema = NULL;
static u8 is_fatfs_init = 0;
static int iter = 0;

void example_fatfs_img_get(void)
{
  memcpy(cache_buf, in_Frame.FrameData, in_Frame.FrameLength);
  cache_size = in_Frame.FrameLength;
}

void save_image(void)
{
  if(is_fatfs_init)
    rtw_up_sema(&save_image_sema);
}

void fatfs_service_thread(void)
{
  int drv_num = 0;
  int Fatfs_ok = 0;
  FRESULT res; 
  FATFS 	m_fs;
  FIL		m_file;
  char	  logical_drv[4]; /* root diretor */
 
  char f_num[15];  

  char filename[64] = {0};
  char path[64];
  int bw;
  int test_result = 1;
  // Set sdio debug level
  DBG_INFO_MSG_OFF(_DBG_SDIO_);  
  DBG_WARN_MSG_OFF(_DBG_SDIO_);
  DBG_ERR_MSG_ON(_DBG_SDIO_);
  hal_sdio_host_adapter_t *psdioh_adapter;
  sdioh_pin_sel_t pin_sel;
  if(sdio_init_host(psdioh_adapter,pin_sel)!=0){
    printf("SDIO host init fail.\n");
    exit(NULL);
  }
  //1 register disk driver to fatfs
  drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
  if(drv_num < 0)
    printf("Rigester disk driver to FATFS fail.\n");
  else{
    Fatfs_ok = 1;
    logical_drv[0] = drv_num + '0';
    logical_drv[1] = ':';
    logical_drv[2] = '/';
    logical_drv[3] = 0;
  }
  //1 Fatfs write 
  if(Fatfs_ok){
    if(f_mount(&m_fs, logical_drv, 1)!= FR_OK){
      printf("FATFS mount logical drive fail.\n");
      goto fail;
    }
    rtw_init_sema(&save_image_sema, 0);
    is_fatfs_init = 1;
    while(is_fatfs_init)
    {
      rtw_down_sema(&save_image_sema);
      rtw_mutex_get(&cache_buf_mutex);
      example_fatfs_img_get();
      rtw_mutex_put(&cache_buf_mutex);      
      memset(filename, 0, 64);
      sprintf(filename, "img");
      sprintf(f_num, "%d", iter);
      strcat(filename,f_num);
      strcat(filename,".jpeg");    
      // write and read test
      strcpy(path, logical_drv);
      sprintf(&path[strlen(path)],"%s",filename);
      //Open source file
      res = f_open(&m_file, path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
      if(res){
        printf("open file (%s) fail.\n", filename);
        continue;
      }
      printf("Test file name:%s\n\n",filename);
      //rewrite from beginning
      if((&m_file)->fsize > 0){
        res = f_lseek(&m_file, 0);
        if(res){
            f_close(&m_file); 
            printf("file Seek error.\n");
            goto out;
          }
      }
      do{
        res = f_write(&m_file, cache_buf, cache_size, (u32*)&bw);
        if(res){
          f_lseek(&m_file, 0); 
          printf("Write error.\n");
          break;
        }
        printf("Write %d bytes.\n", bw);
      } while (bw < cache_size);
      // close source file
      res = f_close(&m_file);
      if(res)
        printf("close file (%s) fail.\n", filename);
      iter++;
    }
out:
    rtw_free_sema(&save_image_sema);
    if(f_mount(NULL, logical_drv, 1) != FR_OK)
      printf("FATFS unmount logical drive fail.\n");
    if(FATFS_UnRegisterDiskDriver(drv_num))	
      printf("Unregister disk driver from FATFS fail.\n");
  }
fail:
  sdio_deinit_host(psdioh_adapter);
  vTaskDelete(NULL);
}

void fatfs_start(void)
{
  if(xTaskCreate(fatfs_service_thread, ((const char*)"fatfs_service_thread"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
    printf("\r\n fatfs_service_thread: Create Task Error\n");
  }
}
#endif
void example_media_tl_main(void *param)
{
  printf("\r\n Enter Timelapse Example\n");
  int err = 0;
  err = -EAGAIN;
  int ret = 0;
  struct uvc_buf_context buf;  
  struct uvc_context *uvc_ctx;  
  /*init usb driver prior to init uvc driver*/
  _usb_init();
  if(wait_usb_ready() < 0){
    printf("\n USB Not Ready \n");
    goto exit;
  }
  /*init uvc driver*/
#if UVC_BUF_DYNAMIC
	if(uvc_stream_init(4,60000) < 0)
            printf("\n Unable to initialize UVC Stream \n");
            goto exit1;
#else
  if(uvc_stream_init() < 0){
    printf("\n Unable to initialize UVC Stream \n");
    goto exit1;
  }
#endif
  uvc_ctx = malloc(sizeof(struct uvc_context)); //Creation of UVC Context
  if(!uvc_ctx){
    printf("\n UVC Context Creation Failed \n");
    goto exit1;
  }
  /* Set UVC Parameters */
  uvc_ctx->fmt_type = UVC_FORMAT_MJPEG;
  uvc_ctx->width = 320;
  uvc_ctx->height = 240;
  uvc_ctx->frame_rate = 5;  
  //uvc_ctx->compression_ratio = 50;
  if(uvc_set_param(uvc_ctx->fmt_type, &uvc_ctx->width, &uvc_ctx->height, &uvc_ctx->frame_rate, &uvc_ctx->compression_ratio) < 0){
    printf("\n UVC Set Parameters Failed \n");
    goto exit2;
  }
  if(uvc_stream_on() < 0){
    printf("\n Unable to turn on UVC Stream\n");
    goto exit2;
  }

  rtw_mutex_init(&cache_buf_mutex);  
#if CONFIG_USE_HTTP_SERVER 
  httpd_reg_page_callback("/", img_get_cb);
  if(httpd_start(80, 1, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE) != 0)
  {
    printf("ERROR: httpd_start");
    httpd_clear_page_callbacks();  
  }
  printf("\n\rhttp server init");
#else
  fatfs_start();
  printf("\n\r fatfs service init");
#endif  
   
    do{ 
      ret = uvc_dqbuf(&buf); 
      if(buf.index < 0){
        printf("\r\n dequeue fail\n");
        err = -EAGAIN;
      }
      if((uvc_buf_check(&buf)<0)||(ret < 0)){
        printf("\n\rbuffer error!");
        uvc_stream_off();
        err = -EIO;	// should return error
      }

#if CONFIG_USE_HTTP_SERVER
      rtw_mutex_get(&cache_buf_mutex);
      in_Frame.FrameData = (char*)buf.data;
      in_Frame.FrameLength = buf.len;
      rtw_mutex_put(&cache_buf_mutex);
#else
      rtw_mutex_get(&cache_buf_mutex);
      in_Frame.FrameData = (char*)buf.data;
      in_Frame.FrameLength = buf.len;
      rtw_mutex_put(&cache_buf_mutex);      
      if(mjpeg2jfif(&in_Frame) == 0)
        save_image();
#endif
      ret = uvc_qbuf(&buf);
      vTaskDelay(1000);
    }while( (err == -EAGAIN) );
    
exit2:  
  rtw_mutex_init(&cache_buf_mutex); 
  /*Close UVC Stream */
  uvc_stream_free();
  /* Free UVC Context */
  free((struct uvc_context *)uvc_ctx);
exit1:
  /* De-initialize USB */
  _usb_deinit();
  /*Delete Task */
exit:
  vTaskDelete(NULL);
}


void example_media_tl(void)
{
  /*user can start their own task here*/
  if(xTaskCreate(example_media_tl_main, ((const char*)"example_media_tl"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
    printf("\r\n example_media_tl_main: Create Task Error\n");
  }	
}


/************************************************************end of rtsp/rtp with motion-jpeg************************************************/
