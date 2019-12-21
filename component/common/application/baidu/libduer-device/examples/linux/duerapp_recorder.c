/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_recorder.c
 * Auth: Zhang leliang(zhangleliang@baidu.com)
 * Desc: Simulate the recorder from the WAV with 8KHz/16bits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lightduer_log.h"
#include "duerapp_config.h"


#define RECORD_FRAME_COUNT_PER_SECOND   50
#define RECORD_DELAY                    (1000 * 1000 / RECORD_FRAME_COUNT_PER_SECOND)
#define PER_RECORD_DATA_SIZE            2
#define BUFFER_SIZE                     2000

#define TO_LOWER(_x)    (((_x) <= 'Z' && (_x) >= 'A') ? ('a' + (_x) - 'A') : (_x))
#define WAV_HEADER_SAMPLE_RATE_POS 24

#ifndef DT_REG
#define DT_REG  (8)
#endif

static duer_record_event_func g_event_callback = NULL;

static int duer_is_wav(const char *name)
{
    //DUER_LOGI("name:%s", name);
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

static void duer_record_event(int status, const void *data, size_t size)
{
    if (g_event_callback != NULL) {
        g_event_callback(status, data, size);
    }
}

static void duer_test_save_asr_filename(char* asr_file)
{
    FILE *fp = NULL;
    if((fp = fopen("asr_result.txt", "a")) == NULL)
    {
        printf("不能打开文件\n");
        return;
    }
    fwrite("\n", 1, 1, fp);
    fwrite(asr_file, 1, strlen(asr_file), fp);
    fwrite(":", 1, 1, fp);
    fclose(fp);
}

static void duer_record_from_file(const char *path)
{
    FILE *file = NULL;
    char buff[BUFFER_SIZE];
    size_t buffsize = BUFFER_SIZE;
    size_t sample_rate = 0;
    int frame_size = 0;

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

        sample_rate = *((int *)&buff[WAV_HEADER_SAMPLE_RATE_POS]);

        frame_size = sample_rate / RECORD_FRAME_COUNT_PER_SECOND * PER_RECORD_DATA_SIZE;
        if (frame_size > BUFFER_SIZE) {
            frame_size = BUFFER_SIZE;
        }

        duer_record_event(REC_START, path, sample_rate);

        DUER_LOGD("record begin");
        while ((rs = fread(buff, 1, frame_size, file)) != 0) {
            DUER_LOGD("record data:%d", rs);
            duer_record_event(REC_DATA, buff, rs);
            usleep(RECORD_DELAY);
        }
        DUER_LOGD("record stop");

        duer_record_event(REC_STOP, path, 0);
    } while (0);

    if (file) {
        fclose(file);
    }
}

void duer_record_set_event_callback(duer_record_event_func func)
{
    if (g_event_callback != func) {
        g_event_callback = func;
    }
}

void duer_record_from_dir(const char *path, short two_query_interval)
{
    struct stat file_stat;

    int status = stat(path, &file_stat);
    if (status) {
        DUER_LOGE("Open file %s fail", path);
        perror("fail:");
        return;
    }
    if (S_ISREG(file_stat.st_mode)) {
        DUER_LOGI("Open file %s", path);
        duer_record_from_file(path);
    } else if (S_ISDIR(file_stat.st_mode)) {
        DUER_LOGI("Open dir begin:%s", path);
        DIR *dir = NULL;
        char buff[256];
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
                DUER_LOGI("Open file type = %d, name = %s", ent->d_type, ent->d_name);
                if (ent->d_type == DT_REG && duer_is_wav(ent->d_name)) {
                    snprintf(buff + length, sizeof(buff) - length, "%s", ent->d_name);
                    //duer_test_save_asr_filename(ent->d_name);
                    duer_record_from_file(buff);
                    sleep(two_query_interval);
                }
            }
            DUER_LOGI("Open dir end:%s", path);
        } while (0);

        if (dir) {
            closedir(dir);
        }
    } else {
        DUER_LOGE("unknow file type:%s", path);
    }
    return;
}

void duer_record_all(const char* voice_record_path, short two_query_interval)
{
    if (voice_record_path == NULL) {
        DUER_LOGE("voice_record_path is NULL");
        return;
    }
    duer_record_from_dir(voice_record_path, two_query_interval);
}
