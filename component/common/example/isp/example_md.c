/*
 * Realtek Semiconductor Corp.
 *
 * example/example_md.c
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//<unistd.h>
#include <string.h>
#include <signal.h>
//#include <pthread.h>
#include <time.h>
#include <rtstream.h>
#include <rtsvideo.h>
#include "FreeRTOS.h"
#include "osdep_service.h"
#include "task.h"

static int g_exit = 0;

#define GRID_R		30
#define GRID_C		40

static void Termination(int sign)
{
	g_exit = 1;
}

void check_md(void)
{
	struct rts_video_grid_bitmap bitmap;
	unsigned int begin, end;
	int ret, idx, number;

	bitmap.rows = GRID_R;
	bitmap.columns = GRID_C;

	while (!g_exit) {
		rtw_msleep_os(1);
		if (rts_check_isp_md_status(0))
			printf("motion detected\n");
		else
			continue;

                begin = rtw_get_current_time();
		ret = rts_get_isp_md_result(0, &bitmap);
                end = rtw_get_current_time();

                idx = 0;
                number = 0;
                while (idx < ((GRID_R * GRID_C + 7) / 8)) {
                        int i;
                        uint8_t val = bitmap.bitmap[idx++];

                        for (i = 0; i < 8; i++) {
                                printf("%d", (val >> i) & 0x1);
                                number++;
                                if (number % GRID_C == 0)
                                        printf("\n");
                                else
                                        printf(" ");

                                if (number == GRID_R * GRID_C)
                                        break;
                        }
                        if (number == GRID_R * GRID_C)
                                break;
                }

		printf("spend time %ld ms, ret = %d\n",
				(end - begin),
				ret);
	}
}

int enable_md(int enable)
{
	struct rts_video_md_attr *attr = NULL;
	int ret;
	int i;

	ret = rts_query_isp_md_attr(&attr, 1920, 1080);
	if (ret)
		return ret;

	if (attr->number == 0) {
		rts_release_isp_md_attr(attr);
		return -1;
	}

	printf("md block number : %d\n", attr->number);

	for (i = 0; i < attr->number; i++) {
		struct rts_video_md_block *pblock = attr->blocks + i;

		printf("block[%d]'s type is %d\n", i, pblock->type);
		if (enable) {
			if (pblock->type ==  RTS_ISP_BLK_TYPE_GRID) {
				pblock->area.left = 320;
				pblock->area.top = 240;
				pblock->area.cell.width = 10;
				pblock->area.cell.height = 10;
				pblock->area.rows = GRID_R;
				pblock->area.columns = GRID_C;
				pblock->area.length = (pblock->area.rows *
					pblock->area.columns + 7) / 8;
				memset(pblock->area.bitmap, 0xff, pblock->area.length);
			} else if (pblock->type == RTS_ISP_BLK_TYPE_RECT) {
				pblock->rect.left = 320;
				pblock->rect.top = 240;
				pblock->rect.right = 720;
				pblock->rect.bottom = 540;
			} else {
				pblock->enable = 0;
				continue;
			}

			pblock->sensitivity = 50;
			pblock->percentage = 50;
			pblock->frame_interval = 5;

			pblock->enable = 1;
		} else {
			pblock->enable = 0;
		}
	}

	ret = rts_set_isp_md_attr(attr);

	rts_release_isp_md_attr(attr);
        
        

	return ret;
}

int example_md(void)
{
	int ret;

	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);

	//rts_set_log_level(1<<RTS_LOG_MEM);

	ret = rts_av_init();
	if (ret)
		return ret;

	ret = enable_md(1);
	if (ret)
		printf("enable md fail, ret = %d\n", ret);

	while (!g_exit) {
		rtw_msleep_os(1);

		check_md();
	}


	enable_md(0);

	rts_av_release();

	return ret;
}

