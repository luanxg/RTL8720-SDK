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
#include <signal.h>
#include <string.h>
#include <rtstream.h>
#include <rtsvideo.h>
#include "FreeRTOS.h"
#include "task.h"

extern unsigned char chi_bin[];
extern unsigned char eng_bin[];
extern unsigned char logo_pic_bin[];

#define H_GAP (2)
#define V_GAP (2)

static int g_exit = 0;

struct rts_video_rect_s {
	int16_t left;
	int16_t top;
	int16_t right;
	int16_t bottom;
};

struct rts_video_osd_block_s {
	struct rts_video_rect_s rect; //block 座標
	uint32_t bg_color; //背景顏色
	uint32_t ch_color; //字元顏色
	uint32_t flick_speed; //閃爍速度，每隔2^flick_speed 閃爍一次
	uint8_t bg_enable; //背景使能
	uint8_t h_gap; //字元與字元之間的間隔
	uint8_t v_gap; //字元與字元之間的間隔
	uint8_t flick_enable; //字元閃爍使能
	uint8_t char_color_alpha; //字元半透明
	uint8_t stroke_enable; //字元描邊的開關
	uint8_t stroke_direct; //字元描邊的方向，0: 減去增量，1: 加上增量
	uint8_t stroke_delta; //字元描邊增量
	uint8_t block_type; //0:text, 1:image
};
struct rts_video_osd_attr_s {
	int number; //osd 的block 數量
	enum rts_osd_time_fmt time_fmt; //顯示的時間格式
//	int time_pos; //時間顯示的位置
	enum rts_osd_date_fmt date_fmt; //顯示的日期格式
//	int date_pos; //日期顯示的位置
	uint8_t date_blkidx; //日期顯示的block 的index
	uint8_t time_blkidx; //時間顯示的block 的index
	uint8_t osd_char_w; //osd 字元寬度
	uint8_t osd_char_h; //osd 字元高度
};

static void Termination(int sign)
{
	g_exit = 1;
}
static int enable_osd(struct rts_video_osd_attr *attr,
		unsigned int eng_addr,
		unsigned int chi_addr,
		unsigned int pic_addr)
{
	int ret;
	int i;

	char *ch_char_str = "苏州上海Realtek08广州007Tel110";

	int ch_len = strlen(ch_char_str);
	struct rts_osd_text_t showtext_ch = {NULL, 0};
	struct rts_osd_char_t showchar_ch[160];
	memset(showchar_ch, 0, sizeof(showchar_ch));

	int j = 0;
	for (i = 0; i < ch_len; ) {

		if (0x0F == ((uint8_t)ch_char_str[i] >> 4)) {
			showchar_ch[j].char_type = char_type_double;
			memcpy(showchar_ch[j++].char_value, &ch_char_str[i], 4);
			i += 4;
		} else if (0x0E == ((uint8_t)ch_char_str[i] >> 4)) {
			showchar_ch[j].char_type = char_type_double;
			memcpy(showchar_ch[j++].char_value, &ch_char_str[i], 3);
			i += 3;
		} else if (0x0C == ((uint8_t)ch_char_str[i] >> 4)) {
			showchar_ch[j].char_type = char_type_double;
			memcpy(showchar_ch[j++].char_value, &ch_char_str[i], 2);
			i += 2;
		} else if (((uint8_t)ch_char_str[i] < 128)) {
			showchar_ch[j].char_type = char_type_single;
			memcpy(showchar_ch[j++].char_value, &ch_char_str[i], 1);
			i++;
		}
	}
	showtext_ch.count = j;
	showtext_ch.content = showchar_ch;
	/*show picture*/
	BITMAP_S bitmap;
	uint8_t *data;

	if (!attr)
		return -1;

	if (attr->number == 0)
		return -1;

	data = (uint8_t*)pic_addr;

	bitmap.pixel_fmt = PIXEL_FORMAT_RGB_1BPP;
	bitmap.u32Width = RTS_MAKE_WORD(data[3], data[4]);
	bitmap.u32Height = RTS_MAKE_WORD(data[5], data[6]);
	bitmap.pData = data + RTS_MAKE_WORD(data[0], data[1]);

	printf("pic file len = %d\n", RTS_MAKE_WORD(data[0], data[1]));
	printf("pic size = %d %d\n", bitmap.u32Width, bitmap.u32Height);
	if (attr->number == 0) {
		/*0 : the first stream*/
		return -1;
	}

	attr->osd_char_w = 32;
	attr->osd_char_h = 32;

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
		case 1:
			pblock->rect.left = 100;
			pblock->rect.top = 350;
			pblock->rect.right = 600;
			pblock->rect.bottom = pblock->rect.top +
				attr->osd_char_h + 2 * V_GAP;
			pblock->bg_enable = 1;
			pblock->bg_color = 0xff0000;
			pblock->flick_enable = 0;
			pblock->ch_color = 0xFFFFFF;
			pblock->char_color_alpha = 8;
			break;
		case 2:
			pblock->rect.left = 100;
			pblock->rect.top = 600;
			pblock->rect.right = 600;
			pblock->rect.bottom = pblock->rect.top +
				attr->osd_char_h + 2 * V_GAP;
			pblock->bg_enable = 1;
			pblock->bg_color = 0x0000ff;
			pblock->ch_color = 0xff00ff;
			pblock->flick_enable = 1;
			pblock->char_color_alpha = 0;
			break;
		case 3:
			pblock->rect.left = 100;
			pblock->rect.top = 500;
			pblock->rect.right = 600;
			pblock->rect.bottom = pblock->rect.top +
				attr->osd_char_h + 2 * V_GAP;
			pblock->bg_enable = 1;
			pblock->bg_color = 0x00ff00;
			pblock->flick_enable = 0;
			pblock->ch_color = 0xFF0000;
			pblock->char_color_alpha = 0;
			pblock->pshowtext = &showtext_ch;
			break;
		default:
			break;
		}
	}
	attr->time_fmt = osd_time_fmt_24;
	attr->time_blkidx = 1;
	attr->time_pos = 0;

	attr->date_fmt = osd_date_fmt_10;
	attr->date_blkidx = 2;
	attr->date_pos = 0;

	attr->single_font_addr = eng_addr;
	attr->double_font_addr = chi_addr;

	/*0 : the first stream*/
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

extern int rtsTimezone;
int example_isp_osd_multi_main(int enable, unsigned int eng_addr, unsigned int chi_addr, unsigned int pic_addr)
{
	int i, ret;
	struct rts_video_osd_attr *attr = NULL;

    signal(SIGINT, Termination);
	signal(SIGTERM, Termination);   
    
	rts_set_log_mask(RTS_LOG_MASK_CONS);

	/*run as:
	 * example 1 eng.bin chi.bin pic.bin
	enable = (int)strtol(argv[1], NULL, 0);
	eng_addr = strtol(argv[2], NULL, 16);
	chi_addr = strtol(argv[3], NULL, 16);
	pic_addr = strtol(argv[4], NULL, 16);
	 * */

	rtsTimezone = 8;
        ret = rts_av_init();
	if (ret)
		return ret;

	ret = rts_query_isp_osd_attr(0, &attr);
	if (ret) {
		printf("query osd attr fail, ret = %d\n", ret);
		goto exit;
	}
	printf("attr num = %d\n", attr->number);

	if (enable)
		ret = enable_osd(attr, eng_addr, chi_addr, pic_addr);
	else
		ret = disable_osd(attr);
	if (ret)
		printf("%s osd fail, ret = %d\n",
				enable ? "enable" : "disable", ret);
	    
	while (!g_exit) {
		usleep(1000 * 1000);

		i++;

		if (i % 60 == 0)
			rts_refresh_isp_osd_datetime(attr);
	}
    
    rts_release_isp_osd_attr(attr);
    
exit:
	rts_av_release();

	return ret;
}
struct osd_para_
{
	int rect_t;
	int rect_l;
	int rect_w;
	int rect_h;
};
static struct osd_para_ g_tOSD_para;
static struct rts_video_osd_attr_s g_tOSD_s;
static struct rts_video_osd_block_s g_tOSD_Block_s[6];
static char g_acText[6][256];
static int g_dTextLen[6] = {0,0,0,0,0,0};

int example_isp_osd_multi_set_block(int a_dIndex, unsigned char *a_pucBuff)
{
	memcpy(&g_tOSD_Block_s[a_dIndex], a_pucBuff, sizeof(g_tOSD_Block_s[0]));
	
	printf("[OSD_MSG:Block%d] rect.left:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.left);
	printf("[OSD_MSG:Block%d] rect.top:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.top);
	printf("[OSD_MSG:Block%d] rect.right:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.right);
	printf("[OSD_MSG:Block%d] rect.bottom:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.bottom);
	printf("[OSD_MSG:Block%d] bg_color:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].bg_color);
	printf("[OSD_MSG:Block%d] ch_color:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].ch_color);
	printf("[OSD_MSG:Block%d] flick_speed:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].flick_speed);
	printf("[OSD_MSG:Block%d] bg_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].bg_enable);
	printf("[OSD_MSG:Block%d] h_gap:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].h_gap);
	printf("[OSD_MSG:Block%d] v_gap:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].v_gap);
	printf("[OSD_MSG:Block%d] flick_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].flick_enable);
	printf("[OSD_MSG:Block%d] char_color_alpha:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].char_color_alpha);
	printf("[OSD_MSG:Block%d] stroke_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_enable);
	printf("[OSD_MSG:Block%d] stroke_direct:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_direct);
	printf("[OSD_MSG:Block%d] stroke_delta:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_delta);
	printf("[OSD_MSG:Block%d] block_type:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].block_type);
}

int example_isp_osd_multi_get_string(int a_dIndex, unsigned char *a_pucBuff, int *a_pdLength)
{
	*a_pdLength = g_dTextLen[a_dIndex];
	memcpy(a_pucBuff, g_acText[a_dIndex], g_dTextLen[a_dIndex]);
}

int example_isp_osd_multi_set_string(int a_dIndex, unsigned char *a_pucBuff, int a_dLength)
{
	int i=0;
	g_dTextLen[a_dIndex] = a_dLength;
	memcpy(g_acText[a_dIndex], a_pucBuff, a_dLength);
	g_acText[a_dIndex][a_dLength] = 0;
	
	printf("[OSD_MSG:Block%d] Text: ", a_dIndex);
	for(i=0; i<a_dLength; i++)
		printf("%c", g_acText[a_dIndex][i]);
	printf("\r\n");
}

int example_isp_osd_multi_get_block(int a_dIndex, unsigned char *a_pucBuff)
{
	memcpy(a_pucBuff, &g_tOSD_Block_s[a_dIndex], sizeof(g_tOSD_Block_s[0]));
}

int example_isp_osd_multi_set_para(unsigned char *a_pucBuff)
{
	memcpy(&g_tOSD_s, a_pucBuff, sizeof(g_tOSD_s));
	
	printf("[OSD_MSG] number:%d \r\n", g_tOSD_s.number);
	printf("[OSD_MSG] time_fmt:%d \r\n", g_tOSD_s.time_fmt);
	printf("[OSD_MSG] date_fmt:%d \r\n", g_tOSD_s.date_fmt);
	printf("[OSD_MSG] date_blkidx:%d \r\n", g_tOSD_s.date_blkidx);
	printf("[OSD_MSG] time_blkidx:%d \r\n", g_tOSD_s.time_blkidx);
	printf("[OSD_MSG] osd_char_w:%d \r\n", g_tOSD_s.osd_char_w);
	printf("[OSD_MSG] osd_char_h:%d \r\n", g_tOSD_s.osd_char_h);
}

int example_isp_osd_multi_get_para(unsigned char *a_pucBuff)
{
	memcpy(a_pucBuff, &g_tOSD_s, sizeof(g_tOSD_s));
}
struct rts_osd_char_t showchar_ch[6][160];

int example_isp_osd_multi_run(int enable, unsigned int eng_addr, unsigned int chi_addr, unsigned int pic_addr, unsigned int para_addr)
{
	int ret;
	struct rts_video_osd_attr *attr = NULL;
	memcpy(&g_tOSD_para, (char*)para_addr, sizeof(g_tOSD_para));
	printf("%d %d %d %d \r\n", g_tOSD_para.rect_t, g_tOSD_para.rect_l, g_tOSD_para.rect_w, g_tOSD_para.rect_h);

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	/*run as:
	 * example 1 eng.bin chi.bin pic.bin
	enable = (int)strtol(argv[1], NULL, 0);
	eng_addr = strtol(argv[2], NULL, 16);
	chi_addr = strtol(argv[3], NULL, 16);
	pic_addr = strtol(argv[4], NULL, 16);
	 * */

	rtsTimezone = 12;
        ret = rts_av_init();
	if (ret)
		return ret;

	ret = rts_query_isp_osd_attr(0, &attr);
	if (ret) {
		printf("query osd attr fail, ret = %d\n", ret);
		goto exit;
	}
	printf("attr num = %d\n", attr->number);

	if (enable)
	{
		int ret;
		int i, k;

		char *ch_char_str;// = "苏州上海Realtek08广州007Tel110";

		int ch_len = 0;
		struct rts_osd_text_t showtext_ch[6];
		

		/*show picture*/
		BITMAP_S bitmap;
		uint8_t *data;

		if (!attr)
			return -1;

		if (attr->number == 0)
			return -1;

		data = (uint8_t*)pic_addr;

		bitmap.pixel_fmt = PIXEL_FORMAT_RGB_1BPP;
		bitmap.u32Width = RTS_MAKE_WORD(data[3], data[4]);
		bitmap.u32Height = RTS_MAKE_WORD(data[5], data[6]);
		bitmap.pData = data + RTS_MAKE_WORD(data[0], data[1]);

		printf("pic file len = %d\n", RTS_MAKE_WORD(data[0], data[1]));
		printf("pic size = %d %d\n", bitmap.u32Width, bitmap.u32Height);
		if (attr->number == 0) {
			/*0 : the first stream*/
			return -1;
		}

		attr->osd_char_w = 32;
		attr->osd_char_h = 32;
		
		attr->time_fmt = osd_time_fmt_24;
		attr->time_blkidx = 1;
		attr->time_pos = 0;

		attr->date_fmt = osd_date_fmt_10;
		attr->date_blkidx = 2;
		attr->date_pos = 0;

		attr->number = g_tOSD_s.number;
		
		for (i = 0; i < attr->number; i++)
		{
			struct rts_video_osd_block *pblock = attr->blocks + i;

			if(g_tOSD_Block_s[i].block_type == 1)
			{
				pblock->rect.left = g_tOSD_Block_s[i].rect.left;
				pblock->rect.top = g_tOSD_Block_s[i].rect.top;
				pblock->rect.right = pblock->rect.left + bitmap.u32Width;
				pblock->rect.bottom = pblock->rect.top + bitmap.u32Height;
				pblock->bg_enable = 0;
				pblock->bg_color = 0x87cefa;
				pblock->ch_color = 0xff0000;
				pblock->flick_enable = 0;
				pblock->char_color_alpha = 0;
				pblock->pbitmap = &bitmap;
			}
			else
			{
				pblock->rect.left = g_tOSD_Block_s[i].rect.left;
				pblock->rect.top = g_tOSD_Block_s[i].rect.top;
				pblock->rect.right = g_tOSD_Block_s[i].rect.right;
				pblock->rect.bottom = g_tOSD_Block_s[i].rect.bottom;
				pblock->bg_enable = 1;
				pblock->bg_color = 0x00ff00;
				pblock->flick_enable = 0;
				pblock->ch_color = 0xFF0000;
				pblock->char_color_alpha = 0;
				
				if(i==attr->time_blkidx || i==attr->date_blkidx)
				{
					pblock->pshowtext = NULL;
				}
				else
				{
					ch_char_str = g_acText[i];
					ch_len = strlen(ch_char_str);
					memset(showchar_ch[i], 0, sizeof(showchar_ch[i]));
					int j = 0;
					for (k = 0; k < ch_len; ) {

						if (0x0F == ((uint8_t)ch_char_str[k] >> 4)) {
							showchar_ch[i][j].char_type = char_type_double;
							memcpy(showchar_ch[i][j++].char_value, &ch_char_str[k], 4);
							k += 4;
						} else if (0x0E == ((uint8_t)ch_char_str[k] >> 4)) {
							showchar_ch[i][j].char_type = char_type_double;
							memcpy(showchar_ch[i][j++].char_value, &ch_char_str[k], 3);
							k += 3;
						} else if (0x0C == ((uint8_t)ch_char_str[k] >> 4)) {
							showchar_ch[i][j].char_type = char_type_double;
							memcpy(showchar_ch[i][j++].char_value, &ch_char_str[k], 2);
							k += 2;
						} else if (((uint8_t)ch_char_str[k] < 128)) {
							showchar_ch[i][j].char_type = char_type_single;
							memcpy(showchar_ch[i][j++].char_value, &ch_char_str[k], 1);
							k++;
						}
					}
					showtext_ch[i].count = j;
					showtext_ch[i].content = showchar_ch[i];
			
					pblock->pshowtext = &showtext_ch[i];
				}
			}
		}
		attr->single_font_addr = eng_addr;
		attr->double_font_addr = chi_addr;

		/*0 : the first stream*/
		ret = rts_set_isp_osd_attr(attr);
		ret = rts_query_isp_osd_attr(0, &attr);
//	printf("[OSD_MSG] number:%d \r\n", g_tOSD_s.number);
//	printf("[OSD_MSG] time_fmt:%d \r\n", g_tOSD_s.time_fmt);
//	printf("[OSD_MSG] date_fmt:%d \r\n", g_tOSD_s.date_fmt);
//	printf("[OSD_MSG] date_blkidx:%d \r\n", g_tOSD_s.date_blkidx);
//	printf("[OSD_MSG] time_blkidx:%d \r\n", g_tOSD_s.time_blkidx);
//	printf("[OSD_MSG] osd_char_w:%d \r\n", g_tOSD_s.osd_char_w);
//	printf("[OSD_MSG] osd_char_h:%d \r\n", g_tOSD_s.osd_char_h);

//	printf("[OSD_MSG:Block%d] rect.left:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.left);
//	printf("[OSD_MSG:Block%d] rect.top:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.top);
//	printf("[OSD_MSG:Block%d] rect.right:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.right);
//	printf("[OSD_MSG:Block%d] rect.bottom:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].rect.bottom);
//	printf("[OSD_MSG:Block%d] bg_color:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].bg_color);
//	printf("[OSD_MSG:Block%d] ch_color:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].ch_color);
//	printf("[OSD_MSG:Block%d] flick_speed:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].flick_speed);
//	printf("[OSD_MSG:Block%d] bg_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].bg_enable);
//	printf("[OSD_MSG:Block%d] h_gap:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].h_gap);
//	printf("[OSD_MSG:Block%d] v_gap:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].v_gap);
//	printf("[OSD_MSG:Block%d] flick_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].flick_enable);
//	printf("[OSD_MSG:Block%d] char_color_alpha:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].char_color_alpha);
//	printf("[OSD_MSG:Block%d] stroke_enable:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_enable);
//	printf("[OSD_MSG:Block%d] stroke_direct:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_direct);
//	printf("[OSD_MSG:Block%d] stroke_delta:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].stroke_delta);
//	printf("[OSD_MSG:Block%d] block_type:%d \r\n", a_dIndex, g_tOSD_Block_s[a_dIndex].block_type);
	}
	else
		ret = disable_osd(attr);
	if (ret)
		printf("%s osd fail, ret = %d\n",
				enable ? "enable" : "disable", ret);

	rts_release_isp_osd_attr(attr);
exit:
	rts_av_release();

	return ret;
}

int example_isp_osd_multi_query()
{
	int i, ret;
	struct rts_video_osd_attr *attr = NULL;

	rts_set_log_mask(RTS_LOG_MASK_CONS);

	ret = rts_av_init();
	if (ret)
		goto exit;

	ret = rts_query_isp_osd_attr(0, &attr);
	if (ret) {
		printf("query osd attr fail, ret = %d\n", ret);
		goto exit;
	}


	printf("[OSD_QUERY_MSG]      number:%d \r\n", attr->number);
	printf("[OSD_QUERY_MSG]    time_fmt:%d \r\n", attr->time_fmt);
	printf("[OSD_QUERY_MSG] time_blkidx:%d \r\n", attr->time_blkidx);
	printf("[OSD_QUERY_MSG]    date_fmt:%d \r\n", attr->date_fmt);
	printf("[OSD_QUERY_MSG] date_blkidx:%d \r\n", attr->date_blkidx);
	printf("[OSD_QUERY_MSG]  osd_char_w:%d \r\n", attr->osd_char_w);
	printf("[OSD_QUERY_MSG]  osd_char_h:%d \r\n", attr->osd_char_h);
	
	for(i=0; i<attr->number; i++)
	{
		printf("[OSD_QUERY_MSG blk%d] rect.left:%d \r\n", i, attr->blocks[i].rect.left);
		printf("[OSD_QUERY_MSG blk%d] rect.top:%d \r\n", i, attr->blocks[i].rect.top);
		printf("[OSD_QUERY_MSG blk%d] rect.right:%d \r\n", i, attr->blocks[i].rect.right);
		printf("[OSD_QUERY_MSG blk%d] rect.bottom:%d \r\n", i, attr->blocks[i].rect.bottom);
		printf("[OSD_QUERY_MSG blk%d] bg_enable:%d \r\n", i, attr->blocks[i].bg_enable);
		printf("[OSD_QUERY_MSG blk%d] bg_color:%d \r\n", i, attr->blocks[i].bg_color);
		printf("[OSD_QUERY_MSG blk%d] ch_color:%d \r\n", i, attr->blocks[i].ch_color);
		printf("[OSD_QUERY_MSG blk%d] h_gap:%d \r\n", i, attr->blocks[i].h_gap);
		printf("[OSD_QUERY_MSG blk%d] v_gap:%d \r\n", i, attr->blocks[i].v_gap);
		printf("[OSD_QUERY_MSG blk%d] flick_enable:%d \r\n", i, attr->blocks[i].flick_enable);
		printf("[OSD_QUERY_MSG blk%d] flick_speed:%d \r\n", i, attr->blocks[i].flick_speed);
		printf("[OSD_QUERY_MSG blk%d] char_color_alpha:%d \r\n", i, attr->blocks[i].char_color_alpha);
		printf("[OSD_QUERY_MSG blk%d] stroke_enable:%d \r\n", i, attr->blocks[i].stroke_enable);
		printf("[OSD_QUERY_MSG blk%d] stroke_direct:%d \r\n", i, attr->blocks[i].stroke_direct);
		printf("[OSD_QUERY_MSG blk%d] stroke_delta:%d \r\n", i, attr->blocks[i].stroke_delta);
		printf("[OSD_QUERY_MSG blk%d] pshowtext:0x%X \r\n", i, (int)attr->blocks[i].pshowtext);
		printf("[OSD_QUERY_MSG blk%d] pbitmap:0x%X \r\n", i, (int)attr->blocks[i].pbitmap);
	}
	
	rts_release_isp_osd_attr(attr);

exit:
	rts_av_release();

	return ret;
}
static void example_isp_osd_multi_task(void *arg)
{
        example_isp_osd_multi_main(1, (unsigned int)eng_bin, (unsigned int)chi_bin, (unsigned int)logo_pic_bin);
        vTaskDelete(NULL);
}

void example_isp_osd_multi(void *arg)
{
        printf("Text/Logo OSD Test\r\n");

	if(xTaskCreate(example_isp_osd_multi_task, "OSD", 8*1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}


