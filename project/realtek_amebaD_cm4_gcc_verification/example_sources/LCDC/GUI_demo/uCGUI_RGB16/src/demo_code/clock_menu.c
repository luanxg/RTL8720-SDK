#include "GUI.h"
#include "gui.h"
#include "math.h"
#include <stdio.h>
#include "GUI.h"
#include "rt_gui_demo_includes.h"

_BUTTON_CUS_Info CusRtClockReturnBtn;

tm_cus timer_rt;


static const uint8_t week[7][4] ={"Mon", "Tue", "Wed", "Thu", "Fri", "Sat","Sun"};


/*clock icon*/
static u16 x_temp[6], y_temp[6];
void DrawClockPointer(uint8_t hour, uint8_t min, uint8_t sec)
{

	x_temp[0] = CLOCK_CENTER_X + (CLOCK_RADIUS-SEC_PTR_OFFSET) * cos(PI/2-PI/30*sec);
	y_temp[0] = CLOCK_CENTER_Y - (CLOCK_RADIUS-SEC_PTR_OFFSET) * sin(PI/2-PI/30*sec);
	x_temp[1] = CLOCK_CENTER_X + (CLOCK_RADIUS_M+2) * cos(PI/2-PI/30*sec);
	y_temp[1] = CLOCK_CENTER_Y - (CLOCK_RADIUS_M+2) * sin(PI/2-PI/30*sec);
	GUI_SetPenSize(1);
	GUI_SetColor(CLOCK_SEC_COLOR);
	GUI_DrawLine(x_temp[0], y_temp[0], x_temp[1], y_temp[1]);

	x_temp[2] = CLOCK_CENTER_X + (CLOCK_RADIUS-MIN_PTR_OFFSET) * cos(PI/2-PI/30*min-PI/30/60*sec);
	y_temp[2] = CLOCK_CENTER_Y - (CLOCK_RADIUS-MIN_PTR_OFFSET) * sin(PI/2-PI/30*min-PI/30/60*sec);
	x_temp[3] = CLOCK_CENTER_X + (CLOCK_RADIUS_M+2) * cos(PI/2-PI/30*min);
	y_temp[3] = CLOCK_CENTER_Y - (CLOCK_RADIUS_M+2) * sin(PI/2-PI/30*min);
	GUI_SetPenSize(2);
	GUI_SetColor(CLOCK_MIN_COLOR);
	GUI_DrawLine(x_temp[2], y_temp[2], x_temp[3], y_temp[3]);

	x_temp[4] = CLOCK_CENTER_X + (CLOCK_RADIUS-HOUR_PTR_OFFSET) * cos(PI/2-PI/6*hour-PI/6/60*min);
	y_temp[4] = CLOCK_CENTER_Y - (CLOCK_RADIUS-HOUR_PTR_OFFSET) * sin(PI/2-PI/6*hour-PI/6/60*min);
	x_temp[5] = CLOCK_CENTER_X + (CLOCK_RADIUS_M+2) * cos(PI/2-PI/6*hour-PI/6/60*min);
	y_temp[5] = CLOCK_CENTER_Y - (CLOCK_RADIUS_M+2) * sin(PI/2-PI/6*hour-PI/6/60*min);
	GUI_SetPenSize(3);
	GUI_SetColor(CLOCK_HOUR_COLOR);
	GUI_DrawLine(x_temp[4], y_temp[4], x_temp[5], y_temp[5]);	
}

void ClearClockPointer(uint8_t hour, uint8_t min, uint8_t sec)
{
	GUI_SetColor(CLOCK_BK_COLOR);
	GUI_SetPenSize(1);
	GUI_DrawLine(x_temp[0], y_temp[0], x_temp[1], y_temp[1]);
	GUI_SetPenSize(2);
	GUI_DrawLine(x_temp[2], y_temp[2], x_temp[3], y_temp[3]);
	GUI_SetPenSize(3);
	GUI_DrawLine(x_temp[4], y_temp[4], x_temp[5], y_temp[5]);
	GUI_SetColor(CLOCK_COLOR);
}

/*boundray*/
void DrawClockBorder(void)
{
	u16 x, y;
	u16 x_temp, y_temp;
	uint8_t i;
	GUI_SetBkColor(CLOCK_BK_COLOR);
	GUI_SetColor(CLOCK_COLOR);
	GUI_Clear();

	GUI_SetFont(&GUI_Font8x12_ASCII);
	GUI_SetTextAlign(GUI_TA_VCENTER | GUI_TA_HCENTER);



	for(i=0; i<CLOCK_RADIUS_M; i++)
	{
		GUI_DrawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS+i);
	}

	GUI_DrawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS_M);
	for(i=0; i<60; i++)
	{
		x = CLOCK_CENTER_X + (CLOCK_RADIUS) * cos(PI/2-PI/30*i);	
		y = CLOCK_CENTER_Y - (CLOCK_RADIUS) * sin(PI/2-PI/30*i);
		if(i%5 == 0)
		{
			x_temp = CLOCK_CENTER_X + (CLOCK_RADIUS-SCALL_OFFSET_B) * cos(PI/2-PI/30*i);
			y_temp = CLOCK_CENTER_Y - (CLOCK_RADIUS-SCALL_OFFSET_B) * sin(PI/2-PI/30*i);
			GUI_SetPenSize(2);
			GUI_DrawLine(x_temp, y_temp, x, y);
			x_temp = CLOCK_CENTER_X + (CLOCK_RADIUS-NUMBER_OFFSET) * cos(PI/2-PI/30*i);
			y_temp = CLOCK_CENTER_Y - (CLOCK_RADIUS-NUMBER_OFFSET) * sin(PI/2-PI/30*i);

			if(i == 0)
			{
				GUI_DispDecAt(12, x_temp, y_temp, 2);	
			}
			else
			{
				if(i < 50)
				{
					GUI_DispDecAt(i/5, x_temp, y_temp, 1);
				}
				else
				{
					GUI_DispDecAt(i/5, x_temp, y_temp, 2);
				}
			}
		}
		else
		{
			x_temp = CLOCK_CENTER_X + (CLOCK_RADIUS-SCALL_OFFSET_S) * cos(PI/2-PI/30*i);
			y_temp = CLOCK_CENTER_Y - (CLOCK_RADIUS-SCALL_OFFSET_S) * sin(PI/2-PI/30*i);
			GUI_SetPenSize(1);
			GUI_DrawLine(x_temp, y_temp, x, y);
		}		
	}
}

/*time display*/
void ClockShowTime(u16 x, u16 y)
{
	GUI_SetFont(&GUI_Font8x16);
	GUI_DispDecAt(timer_rt.w_year, x, y, 4);
	GUI_DispStringAt("-", x+32, y);
	GUI_DispDecAt(timer_rt.w_month, x+40, y, 2);
	GUI_DispStringAt("-", x+56, y);
	GUI_DispDecAt(timer_rt.w_date, x+64, y, 2);
	GUI_DispStringAt((const char *)week[timer_rt.week], x+88, y);
	GUI_DispDecAt(timer_rt.hour, x+16, y+16, 2);
	GUI_DispStringAt(":", x+32, y+16);
	GUI_DispDecAt(timer_rt.min, x+40, y+16, 2);
	GUI_DispStringAt(":", x+56, y+16);
	GUI_DispDecAt(timer_rt.sec, x+64, y+16, 2);
}

/*deinit*/
void DeInitClockMenuArray(void)
{
	CusRtClockReturnBtn.SelState = RT_CUS_BTN_UNSEL;
}

/*Init*/
void InitClockMenuArray(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	

	s32 OrgX;
	s32 OrgY;

	pBitmap = &bmreturn_btn_pic;
	CusRtClockReturnBtn.pBitmap = pBitmap;
	
	CusRtClockReturnBtn.pUnSelBitmap = NULL;
	CusRtClockReturnBtn.xPos = (LCDXSize - pBitmap->XSize - CUS_CLOCK_RET_BTN_OFFSET);
	CusRtClockReturnBtn.yPos = (LCDYSize - pBitmap->YSize - CUS_CLOCK_RET_BTN_OFFSET);
	CusRtClockReturnBtn.TouchXmin = CusRtClockReturnBtn.xPos;
	CusRtClockReturnBtn.TouchXmax = CusRtClockReturnBtn.xPos + pBitmap->XSize;
	CusRtClockReturnBtn.TouchYmin = CusRtClockReturnBtn.yPos;
	CusRtClockReturnBtn.TouchYmax = CusRtClockReturnBtn.yPos + pBitmap->YSize;
	CusRtClockReturnBtn.ID = RT_CUS_CLOCK_RET;
	CusRtClockReturnBtn.ProState = RT_CUS_MAINMENU_PRO;
	CusRtClockReturnBtn.SelState = RT_CUS_BTN_UNSEL;
}

/*get press ID*/
s32 ClockMenuGetPressId(touch_pos_type * PosTemp)
{
	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	_BUTTON_CUS_Info * pRet;
	Id = RT_CUS_MAIN_INVALID;
	pRet = &CusRtClockReturnBtn;

	/*ret*/
	Xmin = pRet->TouchXmin;
	Xmax = pRet->TouchXmax;
	Ymin = pRet->TouchYmin;
	Ymax = pRet->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_CLOCK_RET;
			return Id;
	}	

	return Id;

}

_BUTTON_CUS_Info * ClockMenuGetBtnFromId(s32 Id)
{
	u32 i;
	void * pBtn = NULL;

	if(Id==RT_CUS_CLOCK_RET) {
		pBtn = (void *)&CusRtClockReturnBtn;
	}
	return pBtn;

}

void ClockMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_PID_STATE State;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	s32 i,j;
	u8 hour = 0;
    	u8 min = 0;
    	u8 sec = 0;
	u32 Cnt=0;
	_BUTTON_CUS_Info * pBtn;

	timer_rt.hour = 0;
	timer_rt.min=0;
	timer_rt.sec=0;

	pBtn = &CusRtClockReturnBtn;

	while (1)
	{	
		GUI_TOUCH_GetState(&State);
		PressSts = CusCheckTouchPressSts(&State);
		
		if(PressSts == RT_CUS_TOUCHED_RELEASE) {
			CusGetTouchPos(&State, &Pos);
			IconId = ClockMenuGetPressId(&Pos);
			if(IconId==RT_CUS_CLOCK_RET) {
				pBtn = ClockMenuGetBtnFromId(IconId);
				if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
					pBtn->SelState = RT_CUS_BTN_SEL;
				} else {
					pBtn->SelState = RT_CUS_BTN_UNSEL;				
				}
				Sts->TransSts = pBtn->ProState;
				break;			
			}
		}	
	
		if(Cnt%100==0) {
			ClearClockPointer(hour, min, sec);
			if(timer_rt.sec==59) {
				timer_rt.sec=0;
				timer_rt.min++;
			}
			if(timer_rt.min==59) {
				timer_rt.hour++;
			}
			hour = timer_rt.hour;
			min = timer_rt.min;
			sec = timer_rt.sec;
			DrawClockPointer(hour, min, sec); 
			ClockShowTime(75, 300);
			GUI_DispStringAt("	2018-11-03", 70, 350);
			timer_rt.sec++;
		}
		GUI_Delay(10);
		Cnt++;
	}

}

/*clock menu process*/
void ClockMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_SetBkColor(GUI_BLACK);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
	InitClockMenuArray();
	DrawClockBorder();
	CusCreateButton(&CusRtClockReturnBtn);
	ShowClockMenuStr();
	ClockMenuStateTransfer(Sts);

	CusResetTouchPressSts();
	DeInitClockMenuArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
}

