// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_recorder.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Simulate the recorder from the WAV with 8KHz/16bits.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "duerapp_config.h"
#include "ff.h"

#define SAMPLES_PATH    SDCARD_PATH"/samples"

#define BUFFER_SIZE         2000 //((2000 / RECORD_FARME_SIZE) * RECORD_FARME_SIZE)
#define RECORD_DELAY(_x)    ((_x) * 20 * 10000 / RECORD_FARME_SIZE / 10000)

#define TO_LOWER(_x)    (((_x) <= 'Z' && (_x) >= 'A') ? ('a' + (_x) - 'A') : (_x))

static duer_record_event_func g_event_callback = NULL;

static int duer_is_wav(const char *name)
{
    const char *p = NULL;
    const char *wav = ".wav";

    if (name == NULL) {
        return 0;
    }

    while (*name != '\0') {
        if (*name == '.') {
            p = name;
        }
        name++;
    }

    if (p == NULL) {
        return 0;
    }

    while (*p != '\0' && *wav != '\0' && TO_LOWER(*p) == *wav) {
        p++;
        wav++;
    }

    return *p == *wav;
}

void duer_record_event(int status, const void *data, size_t size)
{
    if (g_event_callback != NULL) {
        g_event_callback(status, data, size);
    }
}

#if 0
static void duer_record_from_file(const char *path)
{
    FILE *file = NULL;
    char buff[BUFFER_SIZE];
    size_t buffsize = BUFFER_SIZE;

    do {
        file = fopen(path, "rb");
        if (file == NULL) {
            DUER_LOGE("Open file %s", path);
            break;
        }

        // Skip the header
        int rs = fread(buff, 1, 44, file);
        if (rs != 44) {
            DUER_LOGE("Read header failed %s", path);
            break;
        }

        duer_record_event(REC_START, path, 0);

        buffsize = 1 + rand() % (BUFFER_SIZE - 1);
        while ((rs = fread(buff, 1, buffsize, file)) != 0) {
            duer_record_event(REC_DATA, buff, rs);
            vTaskDelay(RECORD_DELAY(rs) / portTICK_PERIOD_MS);
        }

        duer_record_event(REC_STOP, path, 0);
    } while (0);

    if (file) {
        fclose(file);
    }
}
#else
static void duer_record_from_file(const char *path)
{
	FIL file;
	FRESULT res;
	char buff[BUFFER_SIZE];
	size_t buffsize = BUFFER_SIZE;
	unsigned int br = 0;

	do {
		DUER_LOGI("Open File: %s", path);
		res = f_open(&file, path, FA_READ | FA_OPEN_EXISTING);
		if(res){
			DUER_LOGE("Failed to open file: %s", path);
			break;
		}

		// Skip the header
		res = f_read(&file, (void *)buff, 44, &br);
		if(res != FR_OK || br != 44)
		{
			DUER_LOGE("Read header failed(res is %d, br is %d).", res, br);
			break;
		}

		duer_record_event(REC_START, path, 0);

		buffsize = 1 + rand() % (BUFFER_SIZE - 1);
		res = f_read(&file, (void *)buff, buffsize, &br);
		while (br > 0 && res == FR_OK) {
			duer_record_event(REC_DATA, buff, br);
			vTaskDelay(RECORD_DELAY(br) / portTICK_PERIOD_MS);
			res = f_read(&file, (void *)buff, buffsize, &br);
		}

		duer_record_event(REC_STOP, path, 0);
	} while (0);

	DUER_LOGI("Close File: %s", path);
	f_lseek(&file, 0);
	res = f_close(&file);
	if(res)
		DUER_LOGE("Close file (%s) failed.\n", path);

}
#endif
void duer_record_set_event_callback(duer_record_event_func func)
{
    if (g_event_callback != func) {
        g_event_callback = func;
    }
}

#if 0
void duer_record_from_dir(const char *path)
{
    DIR *dir = NULL;

    char buff[100];
    size_t length = 0;

    do {
        length = snprintf(buff, sizeof(buff), "%s/", path);
        if (length == sizeof(buff)) {
            DUER_LOGE("The path (%s) is too long!", buff);
            break;
        }

        dir = opendir(path);
        if (dir == NULL) {
            DUER_LOGE("Open %s failed!", path);
            break;
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            DUER_LOGD("Open file type = %d, name = %s", ent->d_type, ent->d_name);
            if (ent->d_type == 1 && duer_is_wav(ent->d_name)) {
                snprintf(buff + length, sizeof(buff) - length, "%s", ent->d_name);
                duer_record_from_file(buff);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
    } while (0);

    if (dir) {
        closedir(dir);
    }
}
#else
void duer_record_from_dir(const char *path)
{
	DIR dir;
	FRESULT res;
	FILINFO finfo;
	char buff[100];
	size_t length = 0;

	do {
		length = snprintf(buff, sizeof(buff), "%s/", path);
		if (length == sizeof(buff)) {
			DUER_LOGE("The path (%s) is too long!", buff);
			break;
		}

		res = f_opendir(&dir, path);
		if (res != FR_OK) {
			DUER_LOGE("Open %s failed!", path);
			break;
		}
		res = f_readdir(&dir, &finfo);
		while( f_readdir(&dir, &finfo) == FR_OK){
			DUER_LOGD("Open file name = %s", finfo.fname);
			if (duer_is_wav(finfo.fname)) {
				snprintf(buff + length, sizeof(buff) - length, "%s", finfo.fname);
				duer_record_from_file(buff);
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

		}
	} while (0);

	res = f_closedir(&dir);
	if (res != FR_OK) {
		DUER_LOGE("Open %s failed!", path);
	}
}
#endif

void duer_record_all()
{
    duer_record_from_dir(SAMPLES_PATH);
}

