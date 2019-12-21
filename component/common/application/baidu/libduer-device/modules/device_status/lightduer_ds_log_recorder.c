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
/*
 * Auth: Yiheng Li(liyiheng01@baidu.com)
 * Desc: recorder module log report
 */
#include "lightduer_ds_log_recorder.h"
#include "lightduer_ds_log.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"

#define CALC_TIME_INTERVAL(_start, _end)    (((_end) > (_start)) ? ((_end) - (_start)) : (0xffffffff - (_start) + (_end) + 1))

enum {
    DUER_REC_STAT_TOTAL,
    DUER_REC_STAT_REQUEST,
    DUER_REC_STAT_NETWORK,
    DUER_REC_STAT_SPEEX,
    DUER_REC_STAT_ENUM_NUM
};

typedef struct _duer_rec_statistics_unit_struct {
    duer_u32_t  _max;
    duer_u32_t  _min;
    duer_u32_t  _sum;
    duer_u32_t  _counts;
} duer_rec_stat_t;

static const char * const g_tags[DUER_REC_STAT_ENUM_NUM] = {
    "total_delay",
    "request_delay",
    "network_delay",
    "speex_compress",
};

static duer_rec_stat_t g_statistics[DUER_REC_STAT_ENUM_NUM];

static duer_status_t duer_ds_log_rec(duer_ds_log_rec_code_t log_code, const baidu_json *log_message) {
    duer_ds_log_level_enum_t log_level = DUER_DS_LOG_LEVEL_INFO;
    switch (log_code) {
    case DUER_DS_LOG_REC_START:
    case DUER_DS_LOG_REC_STOP:
        log_level = DUER_DS_LOG_LEVEL_DEBUG;
        break;
    default:
        DUER_LOGW("unkown log code 0x%0x", log_code);
        break;
    }
    return duer_ds_log(log_level, DUER_DS_LOG_MODULE_RECORDER, log_code, log_message);
}

static void duer_ds_rec_stat_eval(duer_rec_stat_t *stat, duer_u32_t delta) {
    if (stat) {
        if (stat->_min > delta) {
            stat->_min = delta;
        }

        if (stat->_max < delta) {
            stat->_max = delta;
        }

        stat->_sum += delta;
        stat->_counts++;
    }
}

void duer_ds_rec_compress_info_update(const duer_u32_t start, const duer_u32_t end) {
    duer_ds_rec_stat_eval(&g_statistics[DUER_REC_STAT_SPEEX], CALC_TIME_INTERVAL(start, end));
}

void duer_ds_rec_delay_info_update(const duer_u32_t request, const duer_u32_t send_start, const duer_size_t send_finish) {
    // Request delay
    duer_ds_rec_stat_eval(&g_statistics[DUER_REC_STAT_REQUEST], CALC_TIME_INTERVAL(request, send_start));

    // Network Delay
    duer_ds_rec_stat_eval(&g_statistics[DUER_REC_STAT_NETWORK], CALC_TIME_INTERVAL(send_start, send_finish));

    // Total Delay
    duer_ds_rec_stat_eval(&g_statistics[DUER_REC_STAT_TOTAL], CALC_TIME_INTERVAL(request, send_finish));
}

duer_status_t duer_ds_log_rec_start(duer_u32_t id) {
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    duer_s32_t i = 0;

    do {
        DUER_MEMSET(g_statistics, 0, sizeof(g_statistics));
        for (i = 0; i < DUER_REC_STAT_ENUM_NUM; i++) {
            g_statistics[i]._min = 0xffffffff;
        }

        message = baidu_json_CreateObject();
        if (message != NULL) {
            baidu_json_AddNumberToObject(message, "id", id);
        } else {
            DUER_LOGE("Memory Overflow!!!");
        }

        result = duer_ds_log_rec(DUER_DS_LOG_REC_START, message);
    } while (0);

    return result;
}

duer_status_t duer_ds_log_rec_stop(duer_u32_t id) {
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    baidu_json *infos[DUER_REC_STAT_ENUM_NUM] = {NULL};
    duer_s32_t i = 0;

    do {
        message = baidu_json_CreateObject();
        if (message == NULL) {
            DUER_LOGE("message json create fail");
            result = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        for (i = 0; i < DUER_REC_STAT_ENUM_NUM; i++) {
            infos[i] = baidu_json_CreateObject();
            if (infos[i] == NULL) {
                DUER_LOGE("infos[%d] json create fail", i);
                break;
            }
        }

        if (i < DUER_REC_STAT_ENUM_NUM) {
            break;
        }

        baidu_json_AddNumberToObjectCS(message, "id", id);

        for (i = 0; i < DUER_REC_STAT_ENUM_NUM; i++) {
            baidu_json_AddNumberToObjectCS(infos[i], "max", g_statistics[i]._max);
            baidu_json_AddNumberToObjectCS(infos[i], "min", g_statistics[i]._min);
            if (g_statistics[i]._counts) {
                baidu_json_AddNumberToObjectCS(infos[i], "avg",
                        1.0f * g_statistics[i]._sum / g_statistics[i]._counts);
            }

            baidu_json_AddItemToObjectCS(message, g_tags[i], infos[i]);
            DUER_LOGD("%u ==> %s: max = %u, min = %u, avg = %f",
                id, DUER_STRING_OUTPUT(g_tags[i]), g_statistics[i]._max, g_statistics[i]._min,
                g_statistics[i]._counts ? 1.0f * g_statistics[i]._sum / g_statistics[i]._counts : 0);
        }
    } while (0);

    if (i == DUER_REC_STAT_ENUM_NUM) {
        result = duer_ds_log_rec(DUER_DS_LOG_REC_STOP, message);
    } else {
        if (message != NULL) {
            baidu_json_Delete(message);
        }

        for (i = 0; i < DUER_REC_STAT_ENUM_NUM; i++) {
            if (infos[i] != NULL) {
                baidu_json_Delete(infos[i]);
            }
        }
    }

    return result;
}
