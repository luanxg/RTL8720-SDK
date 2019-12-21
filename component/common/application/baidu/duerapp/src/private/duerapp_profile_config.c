// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_profile_config.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Read baidu profile.
 */

#include "duerapp_config.h"
#include "ff.h"

#define PROFILE_PATH    SDCARD_PATH"/profile"

#if 0
const char *duer_load_profile(const char *path)
{
    char *data = NULL;

    FILE *file = NULL;

    do {
        int rs;

        file = fopen(path, "rb");
        if (file == NULL) {
            DUER_LOGE("Failed to open file: %s", path);
            break;
        }

        rs = fseek(file, 0, SEEK_END);
        if (rs != 0) {
            DUER_LOGE("Seek to file tail failed, rs = %d", rs);
            break;
        }

        long size = ftell(file);
        if (size < 0) {
            DUER_LOGE("Seek to file tail failed, rs = %d", rs);
            break;
        }

        rs = fseek(file, 0, SEEK_SET);
        if (rs != 0) {
            DUER_LOGE("Seek to file head failed, rs = %d", rs);
            break;
        }

        data = (char *)malloc(size + 1);
        if (data == NULL) {
            DUER_LOGE("Alloc the profile data failed with size = %ld", size);
            break;
        }

        rs = fread(data, 1, size, file);
        if (rs < 0) {
            DUER_LOGE("Read file failed, rs = %d", rs);
            free(data);
            data = NULL;
            break;
        }

        data[size] = '\0';
    } while (0);

    if (file) {
        fclose(file);
        file = NULL;
    }

    return data;
}
#else
const char *duer_load_profile(const char *path)
{
	char *data = NULL;

	FIL file;
	FRESULT res;

	do {
		unsigned long size;
		unsigned int br = 0;
		
		DUER_LOGI("Open File: %s", path);
		res = f_open(&file, path, FA_READ | FA_OPEN_EXISTING);
		if(res){
			DUER_LOGE("Failed to open file: %s", path);
			break;
		}
		size = file.fsize;

		DUER_LOGI("Profile size: %d", size);

		data = (char *)malloc(size + 1);
		if (data == NULL) {
			DUER_LOGE("Alloc the profile data failed with size = %ld", size);
			break;
		}

		res = f_read(&file, (void *)data, size, &br);
		if(res != FR_OK)
		{
			DUER_LOGE("Read file failed (res is %d).", res);
			free(data);
			data = NULL;
			break;
		}

		data[size] = '\0';
	} while (0);

	DUER_LOGI("Close File: %s", path);
	f_lseek(&file, 0);
	res = f_close(&file);
	if(res)
		DUER_LOGE("Close file (%s) failed.\n", path);

	return data;
}
#endif
