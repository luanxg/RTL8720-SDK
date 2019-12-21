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
//#include <unistd.h>
#include <string.h>
#include <rtstream.h>
#include <rtsvideo.h>
#include "FreeRTOS.h"
#include "task.h"

extern unsigned char chi_bin[];
extern unsigned char eng_bin[];
extern unsigned char logo_pic_bin[];

static int enable_osd_pic(struct rts_video_osd_attr *attr, unsigned int pic_addr)
{
	int ret;
	int i;
	BITMAP_S bitmap;
	uint8_t *data;

	if (!attr)
		return -1;

	if (attr->number == 0)
		return -1;

	data = (uint8_t *)pic_addr;
	if (!data)
		return -1;

	bitmap.pixel_fmt = PIXEL_FORMAT_RGB_1BPP;
	bitmap.u32Width = RTS_MAKE_WORD(data[3], data[4]);
	bitmap.u32Height = RTS_MAKE_WORD(data[5], data[6]);
	bitmap.pData = data + RTS_MAKE_WORD(data[0], data[1]);

	for (i = 0; i < attr->number; i++) {
		struct rts_video_osd_block *pblock = attr->blocks + i;

		switch (i) {
		case 0:
			pblock->rect.left = 10;
			pblock->rect.top = 10;
			pblock->rect.right = pblock->rect.left + bitmap.u32Width;
			pblock->rect.bottom = pblock->rect.top + bitmap.u32Height;
			pblock->bg_enable = 0;
			pblock->bg_color = 0x87cefa;
			pblock->ch_color = 0xff0000;
			pblock->flick_enable = 0;
			pblock->char_color_alpha = 0;
			pblock->pbitmap = &bitmap;
			break;
		/*case 1:
			pblock->rect.left = 320;
			pblock->rect.top = 240;
			pblock->rect.right = pblock->rect.left + bitmap.u32Width;
			pblock->rect.bottom = pblock->rect.top + bitmap.u32Height;
			pblock->bg_enable = 1;
			pblock->bg_color = 0x00ff00;
			pblock->ch_color = 0xff0000;
			pblock->flick_enable = 1;
			pblock->char_color_alpha = 0;
			pblock->pbitmap = &bitmap;
			break;*/
		default:
			break;
		}
	}

	ret = rts_set_isp_osd_attr(attr);

	return ret;
}

static int enable_osd_text(struct rts_video_osd_attr *attr,
		unsigned int eng_addr,
		unsigned int chi_addr,
		unsigned int pic_addr)
{
	int ret;
	int i;
	struct rts_osd_text_t showtext = {NULL, 0};
	struct rts_osd_char_t showchar[16];

	if (!attr)
		return -1;

	if (attr->number == 0)
		return -1;

	memset(showchar, 0, sizeof(showchar));
	showchar[0].char_type = char_type_single;
	showchar[0].char_value[0] = 'R';
	showchar[1].char_type = char_type_single;
	showchar[1].char_value[0] = 'E';
	showchar[2].char_type = char_type_single;
	showchar[2].char_value[0] = 'A';
	showchar[3].char_type = char_type_single;
	showchar[3].char_value[0] = 'L';
	showchar[4].char_type = char_type_single;
	showchar[4].char_value[0] = 'S';
	showchar[5].char_type = char_type_single;
	showchar[5].char_value[0] = 'I';
	showchar[6].char_type = char_type_single;
	showchar[6].char_value[0] = 'L';

	showtext.count = 7;
	showtext.content = showchar;

	for (i = 0; i < attr->number; i++) {
		struct rts_video_osd_block *pblock = attr->blocks + i;

		pblock->bg_color = 0x87cefa;
		pblock->ch_color = 0xcb0000;
		pblock->h_gap = 2;
		pblock->v_gap = 2;

		switch (i) {
		case 0:
			pblock->rect.left = 0;
			pblock->rect.top = 0;
			pblock->rect.right = 1200;
			pblock->rect.bottom = 160;
			pblock->bg_enable = 1;
			pblock->flick_enable = 0;
			pblock->char_color_alpha = 15;
			pblock->pshowtext = &showtext;
			break;
		case 1:
			pblock->rect.left = 0;
			pblock->rect.top = 200;
			pblock->rect.right = 1200;
			pblock->rect.bottom = 360;
			pblock->bg_enable = 0;
			pblock->flick_enable = 0;
			pblock->char_color_alpha = 8;
			break;
		case 2:
			pblock->rect.left = 0;
			pblock->rect.top = 400;
			pblock->rect.right = 1200;
			pblock->rect.bottom = 560;
			pblock->bg_enable = 0;
			pblock->flick_enable = 0;
			pblock->char_color_alpha = 0;
			break;
		default:
			break;
		}
	}

	attr->time_fmt = osd_time_fmt_24;
	attr->time_blkidx = 1;
	attr->time_pos = 0;

	attr->date_fmt = osd_date_fmt_0;
	attr->date_blkidx = 2;
	attr->date_pos = 0;

	attr->single_font_addr = eng_addr;
	attr->double_font_addr = chi_addr;

	attr->osd_char_w = 32;
	attr->osd_char_h = 32;

	ret = rts_set_isp_osd_attr(attr);

	return ret;
}

static int disable_osd(struct rts_video_osd_attr *attr)
{
	int i;

	if (!attr)
		return -1;

	for (i = 0; i < attr->number; i++) {
		struct rts_video_osd_block *block = attr->blocks + i;
		block->pbitmap = NULL;
		block->pshowtext = NULL;
		block->bg_enable = 0;
		block->flick_enable = 0;
		block->stroke_enable = 0;
	}
	attr->time_fmt = osd_time_fmt_no;
	attr->date_fmt = osd_date_fmt_no;
	attr->single_font_addr = NULL;
	attr->double_font_addr = NULL;

	rts_set_isp_osd_attr(attr);

	return 0;
}

/*
static void print_usage(char *cmd)
{
	printf("usage:\n");
	printf("  disable OSD: %s 0\n", cmd);	
	printf("    show logo: %s 1 <logo.bin>\n", cmd);	
	printf("    show text: %s 2\n", cmd);	
}
*/

int example_isp_osd_main(int enable, unsigned int eng_addr, unsigned int chi_addr, unsigned int pic_addr)
{
	int ret;
	struct rts_video_osd_attr *attr = NULL;


	ret = rts_av_init();
	if (ret)
		return ret;

	ret = rts_query_isp_osd_attr(0, &attr);
	if (ret) {
		printf("query osd attr fail, ret = %d\n", ret);
		goto exit;
	}

	if (1==enable)
		ret = enable_osd_pic(attr, pic_addr);
	else if (2==enable)
		ret = enable_osd_text(attr, eng_addr, chi_addr, pic_addr);
	else
		ret = disable_osd(attr);

	if (ret) {
                char bfr[3][16]={"disable OSD", "show logo", "show text"};
		printf("%s osd fail, ret = %d\n", bfr[enable], ret);
        }

	rts_release_isp_osd_attr(attr);
exit:
	rts_av_release();

	return ret;
}
static void example_isp_osd_task(void *param)
{
        example_isp_osd_main(2, (unsigned int)eng_bin, (unsigned int)chi_bin, (unsigned int)logo_pic_bin);
        vTaskDelete(NULL);
}

void example_isp_osd(void *arg)
{
        printf("Text OSD Test\r\n");

	if(xTaskCreate(example_isp_osd_task, "OSD", 16*1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
