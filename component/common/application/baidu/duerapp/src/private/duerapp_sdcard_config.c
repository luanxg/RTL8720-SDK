// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_sdcard_config.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Duer Application sdcard configuration.
 */
#include <diag.h>
#include "duerapp_config.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>

static char logical_drv[4]; /* root diretor */
int drv_num = 0;
static FATFS m_fs;
extern ll_diskio_drv SD_disk_Driver;

int initialize_sdcard(void)
{
	DUER_LOGD("Initializing SD card...");
	
	ConfigDebug[LEVEL_ERROR] |= BIT(MODULE_SDIO);
	ConfigDebug[LEVEL_WARN] |= BIT(MODULE_SDIO);	

	// register disk driver to fatfs
	DUER_LOGD("Register disk driver to Fatfs.");
	drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
	if(drv_num < 0){
		DUER_LOGE("Rigester disk driver to FATFS fail.");
		return -1;
	}else{ 
		logical_drv[0] = drv_num + '0';
		logical_drv[1] = ':';
		logical_drv[2] = '/';
		logical_drv[3] = 0;	
	}

	if(f_mount(&m_fs, logical_drv, 1)!= FR_OK){
		DUER_LOGE("FATFS mount logical drive fail.");
		goto fail;
	}

	DUER_LOGI("SD card initialized.");
	return 0;

fail:
	SD_DeInit();
	DUER_LOGE("SD card initialized failed!");
	return -1;
}

int finalize_sdcard()
{
    if(f_mount(NULL, logical_drv, 1) != FR_OK){
        DUER_LOGE("FATFS unmount logical drive fail.");
    }
    
    if(FATFS_UnRegisterDiskDriver(drv_num))	
        DUER_LOGE("Unregister disk driver from FATFS fail.");

	SD_DeInit();
    return 0;
}


