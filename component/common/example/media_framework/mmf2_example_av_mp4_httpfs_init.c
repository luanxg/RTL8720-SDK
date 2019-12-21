 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"
#define MP4_FILE_LENGTH      30 //30s
#define MP4_GROUP_FILE_NUM   60 //MP4_FILE_LENGTH*60 = 30m
#define STORAGE_FREE_SPACE   MP4_GROUP_FILE_NUM*MP4_FILE_LENGTH/3
#define MP4_LONG_RUN_TEST    1

//static uint32_t file_serial = 0;
static char sd_filename[64];
static char sd_dirname[32];
static fatfs_sd_params_t fatfs_sd;

static void del_old_file(void)
{
	DIR m_dir;
	FILINFO m_fileinfo;
	char *filename;
	char old_filename[32] = {0};
	char old_filepath[32] = {0};
	WORD filedate = 0, filetime = 0, old_filedate = 0, old_filetime = 0;
#if _USE_LFN
	char fname_lfn[32];
	m_fileinfo.lfname = fname_lfn;
	m_fileinfo.lfsize = sizeof(fname_lfn);
#endif

	if(f_opendir(&m_dir, sd_dirname) == 0) {
		while(1) {
			if((f_readdir(&m_dir, &m_fileinfo) != 0) || m_fileinfo.fname[0] == 0) {
				break;
			}

#if _USE_LFN
			filename = *m_fileinfo.lfname ? m_fileinfo.lfname : m_fileinfo.fname;
#else
			filename = m_fileinfo.fname;
#endif
			if(*filename == '.' || *filename == '..') {
				continue;
			}

			if(!(m_fileinfo.fattrib & AM_DIR)) {
				filedate = m_fileinfo.fdate;
				filetime = m_fileinfo.ftime;

				if(	(strlen(old_filename) == 0) ||
					(filedate < old_filedate) ||
					((filedate == old_filedate) && (filetime < old_filetime))) {

					old_filedate = filedate;
					old_filetime = filetime;
					strcpy(old_filename, filename);
				}
			}
		}

		f_closedir(&m_dir);

		if(strlen(old_filename)) {
			sprintf(old_filepath, "%s/%s", sd_dirname, old_filename);
			printf("del %s\n\r", old_filepath);
			f_unlink(old_filepath);
		}
	}
}

int httpfs_response_cb(void)
{
	rt_printf("httpfs response\r\n");
}

#if ISP_BOOT_MODE_ENABLE
#define VIDEO_LEN 30
#define AUDIO_LEN 30
extern isp_boot_cfg_t isp_boot_cfg_global;

void mmf2_example_av_mp4_httpfs_init(void)
{
	isp_v1_ctx = mm_module_open(&isp_module);
        isp_v1_params.boot_mode = ISP_FAST_BOOT;
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		int i = 0;
		for(i=0;i<isp_boot_cfg_global.isp_config.hw_slot_num;i++)
			mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_SELF_BUF, isp_boot_cfg_global.isp_buffer[i]);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
                isp_v1_params.boot_mode = ISP_NORMAL_BOOT;
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}

	h264_v1_ctx = mm_module_open(&h264_module);
	if(h264_v1_ctx){
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_SET_QUEUE_LEN, VIDEO_LEN);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso_isp_h264_v1 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("siso_isp_h264_v1 started\n\r");
	
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, AUDIO_LEN);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("AUDIO open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	aac_ctx = mm_module_open(&aac_module);
	if(aac_ctx){
		mm_module_ctrl(aac_ctx, CMD_AAC_SET_PARAMS, (int)&aac_params);
		mm_module_ctrl(aac_ctx, MM_CMD_SET_QUEUE_LEN, AUDIO_LEN);
		mm_module_ctrl(aac_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(aac_ctx, CMD_AAC_INIT_MEM_POOL, 0);
		mm_module_ctrl(aac_ctx, CMD_AAC_APPLY, 0);
	}else{
		rt_printf("AAC open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
			
        // Create video folder and check free space
	if(fatfs_sd_init()<0){
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	DIR m_dir;
	fatfs_sd_get_param(&fatfs_sd);
	sprintf(sd_dirname, "%s", "VIDEO");
	sprintf(sd_filename, "%s/%s", sd_dirname, "mp4_record");
	if(f_opendir(&m_dir, sd_dirname) == 0){
		f_closedir(&m_dir);
	}
	else{
		f_mkdir(sd_dirname);
	}
	
	if(fatfs_get_free_space() < STORAGE_FREE_SPACE){	  
		del_old_file();
	}
		
	if(fatfs_get_free_space() < STORAGE_FREE_SPACE) {
		rt_printf("ERROR: free space < 50MB\n\r");
	}
	
        mp4_ctx = mm_module_open(&mp4_module);
	mp4_v1_params.record_file_num = MP4_GROUP_FILE_NUM;
        mp4_v1_params.record_length = MP4_FILE_LENGTH;	
	//file_serial = 1234;
	//sprintf(mp4_v1_params.record_file_name, "%s_%d", sd_filename, file_serial);
	sprintf(mp4_v1_params.record_file_name, "%s", sd_filename);
	
	int loopmode = MP4_LONG_RUN_TEST;
	if(mp4_ctx){
		mm_module_ctrl(mp4_ctx, CMD_MP4_SET_PARAMS, (int)&mp4_v1_params);
		mm_module_ctrl(mp4_ctx, CMD_MP4_LOOP_MODE, loopmode);
		mm_module_ctrl(mp4_ctx, CMD_MP4_START, mp4_v1_params.record_file_num);		
	}else{
		rt_printf("MP4 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("MP4 opened\n\r");
	
	//--------------HTTP File Server---------------
	httpfs_ctx = mm_module_open(&httpfs_module);
	if(httpfs_ctx){
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_SET_PARAMS, (int)&httpfs_params);
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_SET_RESPONSE_CB, (int)httpfs_response_cb);
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_APPLY, 0);
	}else{
		rt_printf("HTTPFS open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	siso_audio_aac = siso_create();
	if(siso_audio_aac){
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_OUTPUT, (uint32_t)aac_ctx, 0);
		siso_start(siso_audio_aac);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("siso1 started\n\r");

	miso_h264_aac_mp4 = miso_create();
	if(miso_h264_aac_mp4){
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_INPUT0, (uint32_t)h264_v1_ctx, 0);
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_INPUT1, (uint32_t)aac_ctx, 0);
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_OUTPUT, (uint32_t)mp4_ctx, 0);
		miso_start(miso_h264_aac_mp4);
	}else{
		rt_printf("miso open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	rt_printf("miso started\n\r");
	
#if 1
	//vTaskDelay(1000);
        pre_example_entry();
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	wlan_network();
#endif
#endif
	return;
mmf2_exmaple_av_mp4_httpfs_fail:
	
	return;
}
#else

void mmf2_example_av_mp4_httpfs_init(void)
{
	isp_v1_ctx = mm_module_open(&isp_module);
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	h264_v1_ctx = mm_module_open(&h264_module);
	if(h264_v1_ctx){
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_H264_QUEUE_LEN);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}	
	
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("AUDIO open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	aac_ctx = mm_module_open(&aac_module);
	if(aac_ctx){
		mm_module_ctrl(aac_ctx, CMD_AAC_SET_PARAMS, (int)&aac_params);
		mm_module_ctrl(aac_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(aac_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(aac_ctx, CMD_AAC_INIT_MEM_POOL, 0);
		mm_module_ctrl(aac_ctx, CMD_AAC_APPLY, 0);
	}else{
		rt_printf("AAC open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	 // Create video folder and check free space
	if(fatfs_sd_init()<0){
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	DIR m_dir;
	fatfs_sd_get_param(&fatfs_sd);
	sprintf(sd_dirname, "%s", "VIDEO");
	sprintf(sd_filename, "%s/%s", sd_dirname, "mp4_record");
	if(f_opendir(&m_dir, sd_dirname) == 0){
		f_closedir(&m_dir);
	}
	else{
		f_mkdir(sd_dirname);
	}
	
	if(fatfs_get_free_space() < STORAGE_FREE_SPACE){	  
		del_old_file();
	}
		
	if(fatfs_get_free_space() < STORAGE_FREE_SPACE) {
		rt_printf("ERROR: free space < 50MB\n\r");
	}
	
        mp4_ctx = mm_module_open(&mp4_module);
	mp4_v1_params.record_file_num = MP4_GROUP_FILE_NUM;
        mp4_v1_params.record_length = MP4_FILE_LENGTH;	
	//file_serial = 1234;
	//sprintf(mp4_v1_params.record_file_name, "%s_%d", sd_filename, file_serial);
	sprintf(mp4_v1_params.record_file_name, "%s", sd_filename);
	
	int loopmode = MP4_LONG_RUN_TEST;
	if(mp4_ctx){
		mm_module_ctrl(mp4_ctx, CMD_MP4_SET_PARAMS, (int)&mp4_v1_params);
		mm_module_ctrl(mp4_ctx, CMD_MP4_LOOP_MODE, loopmode);
		mm_module_ctrl(mp4_ctx, CMD_MP4_START, mp4_v1_params.record_file_num);		
	}else{
		rt_printf("MP4 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("MP4 opened\n\r");
	
	//--------------HTTP File Server---------------
	httpfs_ctx = mm_module_open(&httpfs_module);
	if(httpfs_ctx){
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_SET_PARAMS, (int)&httpfs_params);
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_SET_RESPONSE_CB, (int)httpfs_response_cb);
		mm_module_ctrl(httpfs_ctx, CMD_HTTPFS_APPLY, 0);
	}else{
		rt_printf("HTTPFS open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	siso_audio_aac = siso_create();
	if(siso_audio_aac){
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_OUTPUT, (uint32_t)aac_ctx, 0);
		siso_start(siso_audio_aac);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("siso1 started\n\r");
	
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	
	rt_printf("siso2 started\n\r");
	
	miso_h264_aac_mp4 = miso_create();
	if(miso_h264_aac_mp4){
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_INPUT0, (uint32_t)h264_v1_ctx, 0);
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_INPUT1, (uint32_t)aac_ctx, 0);
		miso_ctrl(miso_h264_aac_mp4, MMIC_CMD_ADD_OUTPUT, (uint32_t)mp4_ctx, 0);
		miso_start(miso_h264_aac_mp4);
	}else{
		rt_printf("miso open fail\n\r");
		goto mmf2_exmaple_av_mp4_httpfs_fail;
	}
	rt_printf("miso started\n\r");
	
	return;
mmf2_exmaple_av_mp4_httpfs_fail:
	
	return;
}
#endif
