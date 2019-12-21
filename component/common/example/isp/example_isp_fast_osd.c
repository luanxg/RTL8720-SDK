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

#include <sntp/sntp.h>
#include "freertos_service.h"
#include "osdep_service.h"

extern unsigned char chi_bin[];
extern unsigned char eng_bin[];
extern unsigned char logo_pic_bin[]; 

static int set_osd_string(char *src, struct rts_osd_text_t *ptext)
{
	struct rts_osd_char_t *ret=NULL;
	struct rts_osd_char_t *pchar;
        int i, len;
        
	ptext->count = len = strlen(src);
        pchar = malloc(ptext->count*sizeof(struct rts_osd_char_t));
        if (!pchar)
		return 0;

        memset(pchar, 0, ptext->count*sizeof(struct rts_osd_char_t));
        int j = 0;
        for (i=0; i<len; ) {
	  if (0x0F == ((uint8_t)src[i] >> 4)) {
		pchar[j].char_type = char_type_double;
		memcpy(pchar[j++].char_value, &src[i], 4);
		i += 4; ptext->count-=3;
	  } else if (0x0E == ((uint8_t)src[i] >> 4)) {
		pchar[j].char_type = char_type_double;
		memcpy(pchar[j++].char_value, &src[i], 3);
		i += 3; ptext->count-=2;
	  } else if (0x0C == ((uint8_t)src[i] >> 4)) {
		pchar[j].char_type = char_type_double;
		memcpy(pchar[j++].char_value, &src[i], 2);
		i += 2; ptext->count-=1;
	  } else if (((uint8_t)src[i] < 128)) {
		pchar[j].char_type = char_type_single;
		memcpy(pchar[j++].char_value, &src[i], 1);
		i++;
	  }
        }
	ptext->content = pchar;

        return -1;
}

static void free_osd_string(struct rts_osd_text_t *ptext)
{
	if (ptext->content) {
		free(ptext->content);
                ptext->content = NULL;
                ptext->count = 0;
        }
}

static void strrol(char *str)
{
	int len = strlen(str), size=0;
        char tmp[4];
        
	if (0x0F == ((uint8_t)str[0] >> 4)) {
		size=4;
	} else if (0x0E == ((uint8_t)str[0] >> 4)) {
		size=3;
	} else if (0x0C == ((uint8_t)str[0] >> 4)) {
		size=2;
	} else if (((uint8_t)str[0] < 128)) {
		size=1;
	}
	memcpy(tmp, &str[0], size);
	memcpy(&str[0], &str[size], len-size);
	memcpy(&str[len-size], tmp, size);
}

static int enable_osd_text(struct rts_video_osd_attr *attr,
		unsigned int eng_addr,
		unsigned int chi_addr,
		unsigned int pic_addr1,
		unsigned int pic_addr2)
{
	int ret;
	int i, rpt=0, first_time=1;
	struct rts_osd_text_t presettext = {NULL, 0};
	struct rts_osd_text_t showtext = {NULL, 0}, fixtext = {NULL, 0};
        char strbfr1[] = "PCSWSD   ";
        char strbfr2[] = "REALSIL瑞晟   ";
        char *strbfr;
        char versionbfr[32];
	BITMAP_S bitmap1, bitmap2;
	uint8_t *data;
        const int width=1920, height=1080;
        int org[2]={width/2, height/2};
        float point[6][2]={{0,-0.4},{-0.346,-0.2},{-0.346,0.2},{0,0.4},{0.346,0.2},{0.346,-0.2}};

	if (!attr)
		return -1;

	if (attr->number == 0)
		return -1;

        /*time/date preset*/
	attr->time_fmt = osd_time_fmt_24;
	attr->time_blkidx = 1;
	attr->time_pos = 0;
	attr->date_fmt = osd_date_fmt_0;
	attr->date_blkidx = 2;
	attr->date_pos = 0;

        /*logo1 bitmap preset*/
	data = (uint8_t *)pic_addr1;
	if (!data)
		return -1;
	bitmap1.pixel_fmt = PIXEL_FORMAT_RGB_1BPP;
	bitmap1.u32Width = RTS_MAKE_WORD(data[3], data[4]);
	bitmap1.u32Height = RTS_MAKE_WORD(data[5], data[6]);
	bitmap1.pData = data + RTS_MAKE_WORD(data[0], data[1]);
        attr->ppresetbitmap[0] = &bitmap1;

        /*logo2 bitmap preset*/
	data = (uint8_t *)pic_addr2;
	if (!data)
		return -1;
	bitmap2.pixel_fmt = PIXEL_FORMAT_RGB_1BPP;
	bitmap2.u32Width = RTS_MAKE_WORD(data[3], data[4]);
	bitmap2.u32Height = RTS_MAKE_WORD(data[5], data[6]);
	bitmap2.pData = data + RTS_MAKE_WORD(data[0], data[1]);
        attr->ppresetbitmap[1] = &bitmap2;
        attr->presetbmpnum = 2;

        //char bmp data preset
	attr->single_font_addr = eng_addr;
	attr->double_font_addr = chi_addr;
	attr->osd_char_w = 32;
	attr->osd_char_h = 32;

	if (!set_osd_string("aABCDEFGHIJKLMNOPQRSTUVWXYZ瑞晟", &presettext))
		return -1;
	attr->presettext = &presettext;
	ret = rts_preset_isp_osd_attr(attr);
	free_osd_string(&presettext);
	if (ret) {
		printf("osd bmp preset fail, ret = %d\n", ret);
		return -1;
	}

        //set OSD for the first time
	for (i = 0; i < attr->number; i++) {
		struct rts_video_osd_block *pblock = attr->blocks + i;

		pblock->h_gap = 2;
		pblock->v_gap = 2;

                   switch (i) {
                    case 0:
                            if (!set_osd_string(strbfr1, &showtext))
                                    return -1;
                            pblock->bg_color = 0xff00fa;
                            pblock->ch_color = 0xcbcb00;
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = 1200;
                            pblock->rect.bottom = 160;
                            pblock->bg_enable = 1;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 2;
                            pblock->pshowtext = &showtext;
                            break;
                    case 1:
                            pblock->bg_color = 0x87cefa;
                            pblock->ch_color = 0xcb0000;
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = 1200;
                            pblock->rect.bottom = 360;
                            pblock->bg_enable = 1;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 8;
                            break;
                    case 2:
                            pblock->bg_color = 0x87cefa;
                            pblock->ch_color = 0xcb0000;
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = 1200;
                            pblock->rect.bottom = 560;
                            pblock->bg_enable = 1;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 0;
                            break;
                    case 3:
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = pblock->rect.left + bitmap1.u32Width;
                            pblock->rect.bottom = pblock->rect.top + bitmap1.u32Height;
                            pblock->bg_enable = 0;
                            pblock->bg_color = 0x87cefa;
                            //pblock->ch_color = 0xff0000;
                            pblock->ch_color = (pblock->ch_color+0x002F00)&0x00FFFFFF;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 0;
                            pblock->pbitmap = &bitmap1;
                            break;
                    case 4:
                            versionbfr[0] = 0;
                            sprintf(versionbfr,"VERSION %s", rts_get_lib_version());
                            if (!set_osd_string(versionbfr, &fixtext))
                                    return -1;
                            pblock->bg_color = 0x00fffa;
                            pblock->ch_color = (pblock->ch_color+0x000010)&0x00FFFFFF;
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = 1200;
                            pblock->rect.bottom = 160;
                            pblock->bg_enable = 1;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 2;
                            pblock->pshowtext = &fixtext;
                            break;
                     case 5:
                            pblock->rect.left = org[0]+(int)(point[(rpt+6-i)%6][0]*height);
                            pblock->rect.top = org[1]+(int)(point[(rpt+6-i)%6][1]*height);
                            pblock->rect.right = pblock->rect.left + bitmap2.u32Width;
                            pblock->rect.bottom = pblock->rect.top + bitmap2.u32Height;
                            pblock->bg_enable = 0;
                            pblock->bg_color = 0x87cefa;
                            //pblock->ch_color = 0xff0000;
                            pblock->ch_color = (pblock->ch_color+0x000040)&0x00FFFFFF;
                            pblock->flick_enable = 0;
                            pblock->char_color_alpha = 0;
                            pblock->pbitmap = &bitmap2;
                            break;
                    default:
                            break;
                    }
	}
	ret = rts_fast_set_isp_osd_attr(attr);

	free_osd_string(&showtext);
        //set OSD for the first time

	while (1) {
		unsigned int stick, difftick;
		stick = xTaskGetTickCount();
		rpt++;
		struct rts_video_osd_block *pblock = attr->blocks + 0;

                strbfr = (rpt&4)?strbfr1:strbfr2;
                strrol(strbfr);
                if (!set_osd_string(strbfr, &showtext))
                        return -1;

                pblock->rect.top = org[1]+(int)(point[0][1]*height)+((rpt&4)?100:0);
                pblock->pshowtext = &showtext;
                ret = rts_fast_set_one_isp_osd_attr(attr, 0);
                free_osd_string(&showtext);
                if (ret) break;
                
                if ((rpt&0x1F)==0) {  //change block attr
                        pblock = attr->blocks + 1;
                        pblock->rect.left = org[0]+(int)(point[5][0]*height)-100;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 1);
                        if (ret) break;

                        pblock = attr->blocks + 2;
                        //pblock->rect.left = org[0]+(int)(point[4][0]*height)-100;
                        pblock->blk_hide = 0;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 2);
                        if (ret) break;

                        pblock = attr->blocks + 4;
                        pblock->bg_enable = 1;
                        pblock->char_color_alpha = 2;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 4);
                        if (ret) break;

                        pblock = attr->blocks + 5;
                        pblock->flick_enable = 1;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 5);
                        if (ret) break;
                }
                if ((rpt&0x1F)==16) {  //change block attr
                        pblock = attr->blocks + 1;
                        pblock->rect.left = org[0]+(int)(point[5][0]*height);
                        ret = rts_fast_set_one_isp_osd_attr(attr, 1);
                        if (ret) break;

                        pblock = attr->blocks + 2;
                        //pblock->rect.left = org[0]+(int)(point[4][0]*height);
                        pblock->blk_hide = 1;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 2);
                        if (ret) break;

                        pblock = attr->blocks + 4;
                        pblock->bg_enable = 0;
                        pblock->char_color_alpha = 0;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 4);
                        if (ret) break;

                        pblock = attr->blocks + 5;
                        pblock->flick_enable = 0;
                        ret = rts_fast_set_one_isp_osd_attr(attr, 5);
                        if (ret) break;
               }

		difftick = (xTaskGetTickCount()-stick) / portTICK_RATE_MS;
		printf("period:%dmsec heap:%d\n", difftick, xPortGetFreeHeapSize());

		rts_usleep(200000);
	}
	free_osd_string(&fixtext);

	return ret;
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

int example_isp_fast_osd_main(unsigned int eng_addr, unsigned int chi_addr, unsigned int pic_addr1, unsigned int pic_addr2)
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

	ret = enable_osd_text(attr, eng_addr, chi_addr, pic_addr1, pic_addr2);

	if (ret) {
		printf("fast osd fail, ret = %d\n", ret);
        }

	rts_release_isp_osd_attr(attr);
exit:
	rts_av_release();

	return ret;
}


static void example_isp_osd_multi_task(void *arg)
{
        example_isp_fast_osd_main((unsigned int)eng_bin, (unsigned int)chi_bin, (unsigned int)logo_pic_bin, (unsigned int)logo_pic_bin);
        vTaskDelete(NULL);
}

void example_isp_fast_osd(void *arg)
{
        printf("Text/Logo OSD Test\r\n");

	if(xTaskCreate(example_isp_osd_multi_task, "OSD", 8*1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
