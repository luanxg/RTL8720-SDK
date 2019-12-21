#include "GUI.h"
#include "rt_gui_demo_includes.h"

/*main menu*/
static menu_range MenuLogo;

/*2th layer*/
static menu_range Menu_2_0_Setting;
static menu_range Menu_2_1_RTC;
static menu_range Menu_2_3_Tempre;
static menu_range Menu_2_4_wave;
static menu_range Menu_2_5_Play;

static int GlobalLcdXsize;
static int GlobalLcdYsize;

static unsigned int MenuState;
static unsigned int TouchState;

/*draw start menu figure*/
void ShowMainMenuRT(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	touch_pos_type Pos;
	int InOutState;

	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();

	pBitmap = &bmrealtek_logo;

	ImgXSize = pBitmap->XSize;

	ImgYSize = pBitmap->YSize;

	GUI_DrawBitmap(pBitmap,(LCDXSize-ImgXSize)/2, ((LCDYSize/2)-20)-ImgYSize);
}

/*show stating menu strings*/
void ShowMainMenuSTR(void)
{
	int i;
	int FnXSize;
	int FnYSize;
	const GUI_FONT* Fontp;
	char * ShowStr = "GUI-DEMO";
	int ImageXSize;
	int ImageYSize;
	touch_pos_type Pos;
	int InOutState;

	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();

	GUI_SetColor(RT_DEFAULT_COLOR);

	/*show demo strings*/
	Fontp = &GUI_FontComic24B_1;
	GUI_SetFont(Fontp);
	FnXSize = Fontp->XMag;
	FnYSize = Fontp->YMag;
	ImageXSize = FnXSize*8;
	ImageYSize = FnYSize;


	GUI_ClearRect(0, LCDYSize/2, LCDXSize-1, LCDYSize-1);
	ShowStartMenuStr();

	/*show website*/
	ShowStr = "www.realtek.com";
	Fontp = &GUI_Font8x12_ASCII;
	GUI_SetFont(Fontp);
	FnXSize = Fontp->XMag;
	FnYSize = Fontp->YMag;
	ImageXSize = FnXSize*15;
	ImageYSize = FnYSize;


	GUI_ClearRect(0, LCDYSize-50, LCDXSize-1, LCDYSize-1);
	GUI_DispStringHCenterAt(ShowStr,-ImageXSize+((LCDXSize/2)-(ImageXSize/2)+ImageXSize),LCDYSize-50);
	
	GUI_DrawRect(5, 5, LCDXSize-6, LCDYSize-6);
	GUI_DrawRect(6, 6, LCDXSize-7, LCDYSize-7);
	GUI_DrawRect(7, 7, LCDXSize-8, LCDYSize-8);
	GUI_DrawRect(9, 9, LCDXSize-10, LCDYSize-10);
	GUI_Delay(3000);
}

/*touch position check*/
void RtCheckTouchPos(touch_pos_type * pos)
{
	/*reserved*/
}

int CheckTouchInRang(menu_range * range, touch_pos_type * pos)
{
	int xmin,ymin;
	int xmax,ymax;

	if(range->x0 > range->x1){
		xmin = range->x1;
		xmax = range->x0;
	} else {
		xmin = range->x0;
		xmax = range->x1;
	}

	if(range->y0 > range->y1){
		ymin = range->y1;
		ymax = range->y0;
	} else {
		ymin = range->y0;
		ymax = range->y1;
	}

	if(((pos->x >= xmin)&&(pos->x <= xmax))&&((pos->y >= ymin)&&(pos->y <= ymax))) {
		return TOUCH_POS_IN;
	} else {
		return TOUCH_POS_OUT;			
	}
}

/*scan during showing logo*/
void RtMenuLogoScan(void)
{
	touch_pos_type Pos;
	int InOutState;

	while(1) {
		RtCheckTouchPos(&Pos);
		InOutState = CheckTouchInRang(&MenuLogo, &Pos);
		if(InOutState == MENU_LOGO_TRG) {
			MenuState = MENU_2;
			break;
		}
		GUI_Delay(5);
	}
	TouchState = MENU_TOUCH_NULL;
}

/*start menu*/
void ShowStartMenuStr(void)
{
	char * EnShowStr = "GUI-DEMO";
	char * ChShowStr = "GUI演示实例";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_FontComic24B_1;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(TempColor);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,80,300);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,100,300);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*main menu*/
void ShowMainMenuStr(void)
{
	char * EnShowStr = "Main Menu";
	char * ChShowStr = "主菜单";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(TempColor);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,90,50);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,120,50);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*clock menu*/
void ShowClockMenuStr(void)
{
	char * EnShowStr = "Clock";
	char * ChShowStr = "时钟";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(GUI_WHITE);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,90,50);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,120,50);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*light menu*/
void ShowLightMenuStr(void)
{
	char * EnShowStr = "Color Lighting Control";
	char * ChShowStr = "调色灯光控制";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(GUI_YELLOW);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,60,190);
			GUI_DispStringAt("swicth",100,450);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,90,190);
			GUI_DispStringAt("开关",120,450);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*temperature menu*/
void ShowTempreMenuStr(void)
{
	char * EnShowStr = "Thermometer";
	char * ChShowStr = "温度计";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(GUI_BLUE);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,65,450);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,100,450);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*grad... menu*/
void ShowGradMenuStr(void)
{
	char * EnShowStr = "Gradienter";
	char * ChShowStr = "水平仪";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(GUI_WHITE);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,80,30);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,100,30);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*data aquisition menu*/
void ShowWaveMenuStr(void)
{
	char * EnShowStr = "Data Aqisition";
	char * ChShowStr = "数据采集";
	const GUI_FONT* EnFontp;
	const GUI_FONT* ChFontp;
	const GUI_FONT* BkFontp;	
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	u32 TempColor;

	ChFontp = &GUI_FontHZ_rt_demo_chs_font;
	EnFontp = &GUI_Font24B_ASCII;

	BkFontp = GUI_GetFont();
	TempColor = GUI_GetColor();

	GUI_SetColor(GUI_WHITE);
	if(RtCusLanguage == RT_CUS_ENGLISH) {
			GUI_SetFont(EnFontp);
			GUI_DispStringAt(EnShowStr,60,15);
	} else {
			GUI_SetFont(ChFontp);
			GUI_DispStringAt(ChShowStr,100,15);
	}

	GUI_SetColor(TempColor);
	GUI_SetFont(BkFontp);
}

/*start menu process*/
void StartMenuFunc(RT_CUS_DEMO_STS_TYPE * Sts) 
{
  int Cnt =0;
  int i,YPos;
  const GUI_BITMAP *pBitmap;
  int LCDXSize = LCD_GET_XSIZE();
  int LCDYSize = LCD_GET_YSIZE();

  GlobalLcdXsize = LCD_GET_XSIZE();
  GlobalLcdYsize = LCD_GET_YSIZE();
  

  GUI_Init();
  GUI_SetBkColor(GUI_WHITE);GUI_Clear();
  GUI_SetColor(GUI_RED);

  MenuState = MENULOGO_RT;
  TouchState = MENU_TOUCH_NULL;

	while(1) {
	  switch(MenuState)
	  {
	  	/*show start menu*/
		case MENULOGO_RT:
			MenuLogo.x0 = 10;
			MenuLogo.y0 = 10;
			MenuLogo.x1 = GlobalLcdXsize-10;
			MenuLogo.y1 = GlobalLcdXsize-10;
			ShowMainMenuRT();
			if(TouchState != MENU_LOGO_TRG) {
				ShowMainMenuSTR();
			}
			if(TouchState == MENU_LOGO_TRG) {
				MenuState = MENU_2;
			} else {
				MenuState = MENULOGO_RT_SCAN;
			}
			TouchState = MENU_TOUCH_NULL;
		break;

		case MENULOGO_RT_SCAN:
			RtMenuLogoScan();
		break;

		/*goto main menu*/
		case MENU_2:
			MenuState = MENULOGO_RT;
			Sts->TransSts = RT_CUS_MAINMENU_PRO;
			GUI_Clear();
			return;
		break;

		default:

		break;
	  }  
	}
}
	 	 			 		    	 				 	  			   	 	 	 	 	 	  	  	      	   		 	 	 		  		  	 		 	  	  			     			       	   	 			  		    	 	     	 				  	 					 	 			   	  	  			 				 		 	 	 			     			 
