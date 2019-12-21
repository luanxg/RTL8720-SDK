/*
 * Realtek Semiconductor Corp.
 *
 * example/example_mask.c
 *
 * Copyright (C) 2016      Ming Qian<ming_qian@realsil.com.cn>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//<unistd.h>
#include <string.h>
#include <signal.h>
#include <rtstream.h>
#include <rtsvideo.h>

static int g_exit = 0;

static void Termination(int sign)
{
	g_exit = 1;
}

int enable_mask(struct rts_video_mask_attr *attr, int idx, int enable)
{
	int i;

	if (attr->number == 0)
		return -1;

	if (idx >= attr->number || idx < 0)
		return -1;

	printf("mask block number : %d\n", attr->number);

	for (i = 0; i < attr->number; i++) {
		struct rts_video_mask_block *pblock = attr->blocks + i;

		printf("block[%d]'s type is %d\n", i, pblock->type);

		if (i != idx)
			continue;

		if (enable) {
			if (pblock->type == RTS_ISP_BLK_TYPE_GRID) {
				pblock->area.left = 320;
				pblock->area.top = 240;
				pblock->area.cell.width = 80;
				pblock->area.cell.height = 80;
				pblock->area.rows = 4;
				pblock->area.columns = 4;
				pblock->area.length = (pblock->area.rows *
					pblock->area.columns + 7) / 8;
				memset(pblock->area.bitmap, 0xff, pblock->area.length);
			} else if (pblock->type == RTS_ISP_BLK_TYPE_RECT) {
				pblock->rect.left = 340 * ((i - 1) % 2);
				pblock->rect.top = 260 * ((i - 1) / 2);
				pblock->rect.right = pblock->rect.left + 320;
				pblock->rect.bottom = pblock->rect.top + 240;
			} else {
				pblock->enable = 0;
				continue;
			}

			pblock->enable = 1;

		} else {
			pblock->enable = 0;
		}
	}

	return rts_set_isp_mask_attr(attr);
}

int example_mask(int idx, int w, int h)
{
	int ret;
	struct rts_video_mask_attr *attr = NULL;

	signal(SIGINT, Termination);
	signal(SIGTERM, Termination);

	ret = rts_av_init();
	if (ret)
		return ret;

	ret = rts_query_isp_mask_attr(&attr, w, h);
	if (ret)
		return ret;

	ret = enable_mask(attr, idx, 1);
	if (ret) {
		printf("enable mask fail, ret = %d\n", ret);
		goto exit;
	}

	while (!g_exit) {
		rtw_msleep_os(1);

	}

	enable_mask(attr, idx, 0);

exit:
	rts_release_isp_mask_attr(attr);
	attr = NULL;

	rts_av_release();

	return ret;
}

