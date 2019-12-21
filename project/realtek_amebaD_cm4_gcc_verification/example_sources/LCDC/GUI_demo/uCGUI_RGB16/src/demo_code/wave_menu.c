#include "GUI.h"
#include "rt_gui_demo_includes.h"

#include "GUI.h"
#include "LCD_ConfDefaults.h"
#include <math.h>
#include <stdlib.h>


#define YSIZE   (LCD_YSIZE - 100)
#define DEG2RAD (3.1415926f / 180)

#if LCD_BITSPERPIXEL == 1
  #define COLOR_GRAPH0 GUI_WHITE
  #define COLOR_GRAPH1 GUI_WHITE
#else
  #define COLOR_GRAPH0 GUI_GREEN
  #define COLOR_GRAPH1 GUI_YELLOW
#endif

typedef struct {
  I16 *aY;
} PARAM;

_BUTTON_CUS_Info WaveMenuVar;

static void _Draw(void * p) {
  int i;
  PARAM * pParam = (PARAM *)p;
  GUI_SetBkColor(GUI_BLACK);
  GUI_SetColor(GUI_DARKGRAY);
  GUI_ClearRect(19, (LCD_YSIZE - 20) - YSIZE, (LCD_XSIZE - 2), (LCD_YSIZE - 21));
  for (i = 0; i < (YSIZE / 2); i += 20) {
    GUI_DrawHLine((LCD_YSIZE - 20) - (YSIZE / 2) + i, 19, (LCD_XSIZE - 2));
    if (i) {
      GUI_DrawHLine((LCD_YSIZE - 20) - (YSIZE / 2) - i, 19, (LCD_XSIZE - 2));
    }
  }
  for (i = 40; i < (LCD_XSIZE - 20); i += 40) {
    GUI_DrawVLine(18 + i, (LCD_YSIZE - 20) - YSIZE, (LCD_YSIZE - 21));
  }
  GUI_SetColor(COLOR_GRAPH0);
  GUI_DrawGraph(pParam->aY, (LCD_XSIZE - 20), 19, (LCD_YSIZE - 20) - YSIZE);
}


static void _Draw2(void * p) {
  PARAM * pParam = (PARAM *)p;
  _Draw(p);
  GUI_SetColor(COLOR_GRAPH1);
  GUI_DrawGraph(pParam->aY+15, (LCD_XSIZE - 20), 19, (LCD_YSIZE - 20) - YSIZE);
}


static void _Label(void) {
  int x, y;
  GUI_SetBkColor(GUI_BLUE);
  GUI_Clear();
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_Font8x16);
  //GUI_DispStringHCenterAt("Data-Acquisition", 100, 15);
  ShowWaveMenuStr();
  GUI_SetPenSize(1);
  GUI_ClearRect(0, (LCD_YSIZE - 21) - YSIZE, (LCD_XSIZE - 1), (LCD_YSIZE - 1));
  GUI_DrawRect(18, (LCD_YSIZE - 21) - YSIZE, (LCD_XSIZE - 1), (LCD_YSIZE - 20));
  GUI_SetFont(&GUI_Font6x8);
  for (x = 0; x < (LCD_XSIZE - 20); x += 40) {
    int xPos = x + 18;
    GUI_DrawVLine(xPos, (LCD_YSIZE - 20), (LCD_YSIZE - 14));
    GUI_DispDecAt(x / 40, xPos - 2, (LCD_YSIZE - 9), 1);
  }
  for (y = 0; y < YSIZE / 2; y += 20) {
    int yPos = (LCD_YSIZE - 20) - YSIZE / 2 + y;
    GUI_DrawHLine(yPos, 13, 18);
    if (y) {
      GUI_GotoXY(1, yPos - 4);
      GUI_DispSDec(-y / 20, 2);
      yPos = (LCD_YSIZE - 20) - YSIZE / 2 - y;
      GUI_DrawHLine(yPos, 13, 18);
      GUI_GotoXY(1, yPos - 4);
      GUI_DispSDec(y / 20, 2);
    } else {
      GUI_DispCharAt('0', 7, yPos - 4);
    }
  }
  CusUpdateButtonSts(&WaveMenuVar);
}

static void _GetRandomData(I16 * paY, int Time, int n) {
  int aDiff, i;
  if (Time > 5000)
    Time -= 5000;
  if (Time > 2500)
    Time = 5000 - Time;
  Time /= 200;
  aDiff = Time * Time + 1;
  for (i = 0; i < n; i++) {
    if (!i) {
      paY[i] = rand() % YSIZE;
    } else {
      I16 yNew;
      int yD = aDiff - (rand() % aDiff);
      if (rand() & 1) {
        yNew = paY[i-1] + yD;
      } else {
        yNew = paY[i-1] - yD;
      }
      if (yNew > YSIZE) {
        yNew -= yD;
      } else { if (yNew < 0)
        yNew += yD;
      }
      paY[i] = yNew;
    }
  }
}

#define CUS_RANDOM_SHOW_INTERVAL    2
#define CUS_REFRESH_INTERVAL    30
static void _DemoRandomGraph(void) {

  GUI_PID_STATE State;
  s32 PressSts;
  touch_pos_type Pos;
  s32 IconId;
  s32 i,j;
  PARAM Param;
  int tDiff, t0;
  _BUTTON_CUS_Info * pBtn;
  GUI_RECT Rect = {19, (LCD_YSIZE - 20) - YSIZE, (LCD_XSIZE - 2), (LCD_YSIZE - 21)};
  GUI_HMEM hMem = GUI_ALLOC_AllocZero((LCD_XSIZE - 20) * sizeof(I16));
  pBtn = &WaveMenuVar;

  GUI_SetColor(GUI_WHITE);
  GUI_SetBkColor(GUI_BLUE);
  GUI_ClearRect(0, 55, LCD_XSIZE, 75);
  GUI_SetFont(&GUI_FontComic18B_1);
  GUI_DispStringAt("Random graph", 20, 55);
  GUI_Lock();
  Param.aY = GUI_ALLOC_h2p(hMem);
  GUI_SetFont(&GUI_Font6x8);
  t0 = GUI_GetTime();
  while((tDiff = (GUI_GetTime() - t0)) < CUS_RANDOM_SHOW_INTERVAL) {
    int t1, tDiff2;
    _GetRandomData(Param.aY, tDiff, (LCD_XSIZE - 20));
    t1 = GUI_GetTime();
    GUI_MEMDEV_Draw(&Rect, _Draw, &Param, 0, 0);
    tDiff2 = GUI_GetTime() - t1;
	GUI_TOUCH_GetState(&State);
	PressSts = CusCheckTouchPressSts(&State);
	if(PressSts == RT_CUS_TOUCHED_RELEASE) {
		CusGetTouchPos(&State, &Pos);
		IconId = WaveMenuGetPressId(&Pos);
		if(IconId==RT_CUS_WAVE_RET) {
			pBtn = WaveMenuGetBtnFromId(IconId);
			if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
				pBtn->SelState = RT_CUS_BTN_SEL;
			} else {
				pBtn->SelState = RT_CUS_BTN_UNSEL;				
			}
			RtCusDemoVar.TransSts = pBtn->ProState;
			break;			
		}
	}

  }
  GUI_Unlock();
  GUI_ALLOC_Free(hMem);
}

static void _GetSineData(I16 * paY, int n) {
  int i;
  for (i = 0; i < n; i++) {
    float s = sin(i * DEG2RAD * 4);
    paY[i] = s * YSIZE / 2 + YSIZE / 2;
  }
}

static void _DemoSineWave(void) {

  GUI_PID_STATE State;
  s32 PressSts;
  touch_pos_type Pos;
  s32 IconId;
  s32 i,j;
  _BUTTON_CUS_Info * pBtn;
  PARAM Param;
  I16 * pStart;
  int t0, Cnt = 0;
  GUI_RECT Rect = {19, (LCD_YSIZE - 20) - YSIZE, (LCD_XSIZE - 2), (LCD_YSIZE - 21)};
  GUI_HMEM hMem = GUI_ALLOC_AllocZero(405 * sizeof(I16));

  pBtn = &WaveMenuVar;

  GUI_SetColor(GUI_WHITE);
  GUI_SetBkColor(GUI_BLUE);
  GUI_ClearRect(0, 50, LCD_XSIZE, 75);
  GUI_SetFont(&GUI_FontComic18B_1);
  GUI_DispStringAt("Sine wave", 20, 55);
  pStart = GUI_ALLOC_h2p(hMem);
  _GetSineData(pStart, 405);
  GUI_SetFont(&GUI_Font6x8);
  t0 = GUI_GetTime();
  while((GUI_GetTime() - t0) < 10000) {
    int t1, tDiff2;
    if (Cnt++ % 90) {
      Param.aY++;
    } else {
      Param.aY = pStart;
    }
    t1 = GUI_GetTime();
    GUI_MEMDEV_Draw(&Rect, _Draw2, &Param, 0, 0);
    tDiff2 = GUI_GetTime() - t1;
    if (tDiff2 < 100) {
      GUI_Delay(100 - tDiff2);
    }
  GUI_TOUCH_GetState(&State);
  PressSts = CusCheckTouchPressSts(&State);
  if(PressSts == RT_CUS_TOUCHED_RELEASE) {
	  CusGetTouchPos(&State, &Pos);
	  IconId = WaveMenuGetPressId(&Pos);
	  if(IconId==RT_CUS_WAVE_RET) {
		  pBtn = WaveMenuGetBtnFromId(IconId);
		  if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
			  pBtn->SelState = RT_CUS_BTN_SEL;
		  } else {
			  pBtn->SelState = RT_CUS_BTN_UNSEL;			  
		  }
		  RtCusDemoVar.TransSts = pBtn->ProState;
		  break;		  
	  }
  }

  }
  GUI_ALLOC_Free(hMem);
}

static void _DrawOrData(GUI_COLOR Color, I16 * paY) {
  GUI_SetColor(Color);
  GUI_DrawGraph(paY, (LCD_XSIZE - 20), 19, (LCD_YSIZE - 20) - YSIZE);
}

static void _DemoOrData(void) {
	GUI_PID_STATE State;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	s32 j;
	_BUTTON_CUS_Info * pBtn;
	I16 * pStart;
	int t0, Cnt = 0;
  int i;
  PARAM Param;
  GUI_RECT Rect = {19, (LCD_YSIZE - 20) - YSIZE, (LCD_XSIZE - 2), (LCD_YSIZE - 21)};
  GUI_HMEM hMem = GUI_ALLOC_AllocZero(405 * sizeof(I16));

  pBtn = &WaveMenuVar;
  
  GUI_SetColor(GUI_WHITE);
  GUI_SetBkColor(GUI_BLUE);
  GUI_ClearRect(0, 55, LCD_XSIZE, 75);
  GUI_SetFont(&GUI_FontComic18B_1);
  GUI_DispStringAt("Several waves...", 20 ,55);
  Param.aY = GUI_ALLOC_h2p(hMem);
  _GetSineData(Param.aY, 405);
  GUI_MEMDEV_Draw(&Rect, _Draw, &Param, 0, 0);
  for (i = 0; (i < 90); i++) {
    _DrawOrData(GUI_GREEN, ++Param.aY);
    GUI_Delay(10);
	GUI_TOUCH_GetState(&State);
	PressSts = CusCheckTouchPressSts(&State);
	if(PressSts == RT_CUS_TOUCHED_RELEASE) {
		CusGetTouchPos(&State, &Pos);
		IconId = WaveMenuGetPressId(&Pos);
		if(IconId==RT_CUS_WAVE_RET) {
			pBtn = WaveMenuGetBtnFromId(IconId);
			if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
				pBtn->SelState = RT_CUS_BTN_SEL;
			} else {
				pBtn->SelState = RT_CUS_BTN_UNSEL;				
			}
			RtCusDemoVar.TransSts = pBtn->ProState;
			break;			
		}
	}
  }
  GUI_ALLOC_Free(hMem);
}

void Task_1(void) {
  _Label();
  while(1) {
    _DemoRandomGraph();
	if(RtCusDemoVar.TransSts!=RT_CUS_WAVE_PRO) {
		return;
	}
    _DemoSineWave();
	if(RtCusDemoVar.TransSts!=RT_CUS_WAVE_PRO) {
		return;
	}
    _DemoOrData();
	if(RtCusDemoVar.TransSts!=RT_CUS_WAVE_PRO) {
		return;
	}
  }
}


void DeInitWaveMenuArray(void)
{
	WaveMenuVar.SelState = RT_CUS_BTN_UNSEL;	
}

void InitWaveMenuArray(void)
{
		int i;
		int ImgXSize;
		int ImgYSize;
		const GUI_BITMAP *pBitmap;
		int LCDXSize = LCD_GET_XSIZE();
		int LCDYSize = LCD_GET_YSIZE();	

		s32 OrgX;
		s32 OrgY;

		/*ret*/
		pBitmap = &bmreturn_btn_pic;
		WaveMenuVar.pBitmap = pBitmap;
		WaveMenuVar.pUnSelBitmap = NULL;
		WaveMenuVar.xPos = (LCDXSize - pBitmap->XSize - CUS_WAVE_RET_BTN_OFFSET);
		WaveMenuVar.yPos = CUS_WAVE_RET_BTN_OFFSET;
		WaveMenuVar.TouchXmin = WaveMenuVar.xPos;
		WaveMenuVar.TouchXmax = WaveMenuVar.xPos + pBitmap->XSize;
		WaveMenuVar.TouchYmin = WaveMenuVar.yPos;
		WaveMenuVar.TouchYmax = WaveMenuVar.yPos + pBitmap->YSize;
		WaveMenuVar.ID = RT_CUS_WAVE_RET;
		WaveMenuVar.ProState = RT_CUS_MAINMENU_PRO;
		WaveMenuVar.SelState = RT_CUS_BTN_UNSEL;	
}

s32 WaveMenuGetPressId(touch_pos_type * PosTemp)
{

	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	_BUTTON_CUS_Info * pRet;
	Id = RT_CUS_MAIN_INVALID;
	pRet = &WaveMenuVar;

	/*ret*/
	Xmin = pRet->TouchXmin;
	Xmax = pRet->TouchXmax;
	Ymin = pRet->TouchYmin;
	Ymax = pRet->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_WAVE_RET;
			return Id;
	}	

	return Id;

}

void * WaveMenuGetBtnFromId(s32 Id)
{
	u32 i;
	void * pBtn = NULL;

	if(Id==RT_CUS_WAVE_RET) {
		pBtn = (void *)&WaveMenuVar;
	}
	return pBtn;	
}


void WaveMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{
	Task_1();
}

/*data aquisition menu process*/
void WaveMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{
	InitWaveMenuArray();
	WaveMenuStateTransfer(Sts);
	CusResetTouchPressSts();
	DeInitGradMenuArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();	
}

