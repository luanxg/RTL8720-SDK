#include "mmf_source.h"
#include "mmf_source_mp4de.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "basic_types.h"
#include "section_config.h"
#include "osdep_service.h"
#include "FreeRTOS.h"
#include "task.h"
#include "mp4_demuxer.h"

SDRAM_DATA_SECTION unsigned int all_buf[3600][2]={1};//avoid flashloader issue 
SDRAM_DATA_SECTION unsigned char mp4_buf[2][1024*150];

static unsigned int aac_timestamp   = 0;
static unsigned int video_timestamp = 0;

static int fatfs_init(void* ctx){
	char path[64];
	
	int test_result = 1;
	int ret = 0;
        pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
	//1 init disk drive
	DBG_8195A("Init Disk driver.\n");
	// Set sdio debug level
	DBG_INFO_MSG_OFF(_DBG_SDIO_);  
	DBG_WARN_MSG_OFF(_DBG_SDIO_);
	DBG_ERR_MSG_ON(_DBG_SDIO_);
	
	if(sdio_init_host()!=0){
		DBG_8195A("SDIO host init fail.\n");
		return -1;
	}

	//1 register disk driver to fatfs
	DBG_8195A("Register disk driver to Fatfs.\n");
	mp4de_ctx->drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
        
	if(mp4de_ctx->drv_num < 0){
		DBG_8195A("Rigester disk driver to FATFS fail.\n");
                goto fail;
	}else{
                mp4de_ctx->_drv[0] = mp4de_ctx->drv_num + '0';
		mp4de_ctx->_drv[1] = ':';
                mp4de_ctx->_drv[2] = '/';
                mp4de_ctx->_drv[3] = 0;
	}
        
        DBG_8195A("FatFS Write begin......\n\n");
        printf("\r\ndrv(%s)", mp4de_ctx->_drv);
        if(f_mount(&mp4de_ctx->m_fs,mp4de_ctx->_drv, 1)!= FR_OK){
                DBG_8195A("FATFS mount logical drive fail.\n");
                goto fail;
        }

	return 0;
fail:
	sdio_deinit_host();
	return -1;
}

static int fatfs_close(void *ctx){
        pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
        
        if(f_mount(NULL,mp4de_ctx->_drv, 1) != FR_OK){
                DBG_8195A("FATFS unmount logical drive fail.\n");
        }
        
        if(FATFS_UnRegisterDiskDriver(mp4de_ctx->drv_num))	
                DBG_8195A("Unregister disk driver from FATFS fail.\n");
	
	sdio_deinit_host();
	return 0;
}

void* mp4de_mod_open(void)
{
	pmp4_demux mp4de_ctx = (pmp4_demux)malloc(sizeof(mp4_demux));
	if(!mp4de_ctx){
		printf("malloc failed\r\n");
	}
	memset(mp4de_ctx,0,sizeof(mp4_demux));
        fatfs_init(mp4de_ctx);
        mp4de_ctx->all_buf=all_buf;
        
	return (void*)mp4de_ctx;
}

void mp4de_mod_close(void* ctx)
{
	pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
        fatfs_close(mp4de_ctx);
        free(mp4de_ctx);
	return;
}

int mp4de_mod_set_param(void* ctx, int cmd, int arg)
{
        int ret = 0;
        pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
	char abs_path[32];
        int index = 0;
	switch(cmd){
        case CMD_SET_ST_TYPE:
                mp4de_ctx->type = arg;
                printf("type = %d\r\n",mp4de_ctx->type);
                break; 
        case CMD_SET_ST_FILENAME:
                memset(mp4de_ctx->filename,0,sizeof(mp4de_ctx->filename));
                strcpy(mp4de_ctx->filename,(char*)arg);
                strcpy(abs_path, mp4de_ctx->_drv);
                sprintf(&abs_path[strlen(abs_path)],"%s", mp4de_ctx->filename);
                ret = f_open(&mp4de_ctx->m_file,abs_path,FA_OPEN_EXISTING | FA_READ);
                if(ret != FR_OK){
                    printf("Open source file %s fail.\n", mp4de_ctx->filename);
                    ret = -1;
                }

                if(mp4de_ctx->type == STORAGE_VIDEO){
                    index = get_video_trak(mp4de_ctx,index);
                    printf("index = %d\r\n",index);
                    mp4de_ctx->total_len = mp4de_ctx->video_len+mp4de_ctx->audio_len;
                }else if(mp4de_ctx->type == STORAGE_AUDIO){
                    index = get_video_trak(mp4de_ctx,index);
                    printf("index = %d\r\n",index);
                    mp4de_ctx->total_len = mp4de_ctx->video_len+mp4de_ctx->audio_len;
                    index = get_audio_trak(mp4de_ctx,index);
                    printf("index = %d\r\n",index);
                    mp4de_ctx->total_len = mp4de_ctx->video_len+mp4de_ctx->audio_len;
                    mp4de_ctx->mp4_index = mp4de_ctx->video_len;
                }else{
                    index = get_video_trak(mp4de_ctx,index);
                    printf("index = %d\r\n",index);
                    mp4de_ctx->total_len = mp4de_ctx->video_len+mp4de_ctx->audio_len;
                    index = get_audio_trak(mp4de_ctx,index);
                    printf("index = %d\r\n",index);
                    mp4de_ctx->total_len = mp4de_ctx->video_len+mp4de_ctx->audio_len;
                    printf("video len = %d audio len = %d total = %d\r\n",mp4de_ctx->video_len,mp4de_ctx->audio_len,mp4de_ctx->total_len);
                    qsort(mp4de_ctx->all_buf,mp4de_ctx->video_len+mp4de_ctx->audio_len,sizeof( int )*2,ccompare);
                }
                break;
        case CMD_SET_ST_START:
                mp4_set_start_status(mp4de_ctx);
                break;
	default:
		ret = -EINVAL;
		break;
	}
        return  ret;
}

#define MP4_OFFSET_INDEX 0
#define MP4_SIZE_INDEX   1



int get_mp4_buffer_index(void *ctx,void* b)
{
      exch_buf_t *exbuf = (exch_buf_t*)b;
      pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
      unsigned int len = 0;
      static unsigned char buf_index = 0;
      buf_index = buf_index%2;
      len = mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_SIZE_INDEX]&0x3ffffff;
      fseek(&mp4de_ctx->m_file,mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_OFFSET_INDEX],0);
      if(mp4de_ctx->type == STORAGE_VIDEO){
            if(mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_SIZE_INDEX]&(1<<30)){
                  memcpy(mp4_buf[buf_index],mp4de_ctx->sps,mp4de_ctx->sps_length);
                  memcpy(mp4_buf[buf_index]+mp4de_ctx->sps_length,mp4de_ctx->pps,mp4de_ctx->pps_length);
                  fread(mp4_buf[buf_index]+mp4de_ctx->sps_length+mp4de_ctx->pps_length,len,1,&mp4de_ctx->m_file);
                  mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+0]=0x00;
                  mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+1]=0x00;
                  mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+2]=0x00;
                  mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+3]=0x01;
                  exbuf->len = len+mp4de_ctx->sps_length+mp4de_ctx->pps_length;
            }else{
                  fread(mp4_buf[buf_index],len,1,&mp4de_ctx->m_file);
                  mp4_buf[buf_index][0]=0x00;mp4_buf[buf_index][1]=0x00;mp4_buf[buf_index][2]=0x00;mp4_buf[buf_index][3]=0x01;  
                  exbuf->len = len;
            }
      }else if(mp4de_ctx->type == STORAGE_AUDIO){
            if((mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_SIZE_INDEX]&(1<<31))){
              create_adts_header(mp4de_ctx->sample_index,mp4de_ctx->channel_count,len,mp4_buf[buf_index]);
              fread(mp4_buf[buf_index]+7,len,1,&mp4de_ctx->m_file);
              exbuf->codec_fmt = FMT_A_MP4A_LATM;
              exbuf->len = len+7;
            }else{
              printf("wrong id = %d\r\n");
            }
      }else{
              if((mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_SIZE_INDEX]&(1<<31)) == 0x00){
                      if(mp4de_ctx->all_buf[mp4de_ctx->mp4_index][MP4_SIZE_INDEX]&(1<<30)){
                                    memcpy(mp4_buf[buf_index],mp4de_ctx->sps,mp4de_ctx->sps_length);

/*                                    
printf("\r\nSPS:\r\n");
for(int i=0;i<mp4de_ctx->sps_length;i++){   
  printf("0x%02x, ",mp4_buf[buf_index][i]);
}
unsigned char *t = mp4_buf[buf_index]+mp4de_ctx->sps_length;
printf("\r\nPPS:\r\n");
for(int i=0;i<mp4de_ctx->pps_length;i++){   
  printf("0x%02x, ",t[i]);
}
printf("\r\n");
*/
                               memcpy(mp4_buf[buf_index]+mp4de_ctx->sps_length,mp4de_ctx->pps,mp4de_ctx->pps_length);
                                    fread(mp4_buf[buf_index]+mp4de_ctx->sps_length+mp4de_ctx->pps_length,len,1,&mp4de_ctx->m_file);
                                    mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+0]=0x00;
                                    mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+1]=0x00;
                                    mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+2]=0x00;
                                    mp4_buf[buf_index][mp4de_ctx->sps_length+mp4de_ctx->pps_length+3]=0x01;
                                    exbuf->len = len+mp4de_ctx->sps_length+mp4de_ctx->pps_length;
                      }else{
                                    fread(mp4_buf[buf_index],len,1,&mp4de_ctx->m_file);
                                    mp4_buf[buf_index][0]=0x00;mp4_buf[buf_index][1]=0x00;mp4_buf[buf_index][2]=0x00;mp4_buf[buf_index][3]=0x01;  
                                    exbuf->len = len;
                      }
                      exbuf->codec_fmt = FMT_V_H264;
              }else{
                      create_adts_header(mp4de_ctx->sample_index,mp4de_ctx->channel_count,len,mp4_buf[buf_index]);
                      fread(mp4_buf[buf_index]+7,len,1,&mp4de_ctx->m_file);
                      exbuf->codec_fmt = FMT_A_MP4A_LATM;
                      exbuf->len = len+7;
                      //printf("audio\r\n");
              }
      }
      
      exbuf->index = 0;
      exbuf->data = mp4_buf[buf_index];
      
      if(mp4de_ctx->type == STORAGE_VIDEO){
          exbuf->timestamp = 0;
      }else if(mp4de_ctx->type == STORAGE_AUDIO){
          aac_timestamp+=1024;
          exbuf->timestamp = aac_timestamp;
      }else{
          if(exbuf->codec_fmt == FMT_A_MP4A_LATM){
            aac_timestamp+=1024;//1024*SYSTICK_CLK_HZ / AAC_SAMPLERATE
            //aac_timestamp+=(1024*1000 / 16000);
            exbuf->timestamp = aac_timestamp;
            //printf("au = %d\r\n",exbuf->timestamp);
          }else{
             video_timestamp+=3000;
             exbuf->timestamp = video_timestamp;
             //printf("vi = %d\r\n",exbuf->timestamp);
             //exbuf->timestamp = aac_timestamp;
             //exbuf->timestamp = rtsp_get_current_tick();
             //exbuf->timestamp = 0;
          } 
      }
      
      exbuf->state = STAT_READY;
      mp4de_ctx->mp4_index++;
      if(mp4de_ctx->type == STORAGE_AUDIO){
          mp4de_ctx->mp4_index=mp4de_ctx->mp4_index%mp4de_ctx->total_len;
          if(mp4de_ctx->mp4_index == 0){
              mp4de_ctx->mp4_index = mp4de_ctx->video_len;
          }
      }else{
          mp4de_ctx->mp4_index=mp4de_ctx->mp4_index%mp4de_ctx->total_len;
      }
#if 0
      if(mp4de_ctx->type == STORAGE_ALL){
        if(exbuf->codec_fmt==FMT_V_H264)
          exbuf->state = STAT_USED;
      } 
#endif
      
      buf_index++;
      return 0;
      
}
int free_mp4_buffer_index(void *ctx,void* b)
{
      exch_buf_t *exbuf = (exch_buf_t*)b;
      pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
      return 0;
}


int mp4de_mod_handle(void* ctx, void* b)
{
	int ret = 0;
	
	pmp4_demux mp4de_ctx = (pmp4_demux)ctx;
	
	exch_buf_t *exbuf = (exch_buf_t*)b;
	
	/*get uvc buffer for new data*/
	if(exbuf->state!=STAT_READY){
            get_mp4_buffer_index(mp4de_ctx,exbuf);
	}
	mmf_source_add_exbuf_sending_list(exbuf, &rtsp2_module);
	return 0;

}

msrc_module_t mp4de_module =
{
	.create = mp4de_mod_open,
	.destroy = mp4de_mod_close,
	.set_param = mp4de_mod_set_param,
	.handle = mp4de_mod_handle,
};