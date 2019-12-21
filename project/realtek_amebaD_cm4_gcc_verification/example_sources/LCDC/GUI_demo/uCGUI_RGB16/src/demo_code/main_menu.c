#include "GUI.h"
#include "rt_gui_demo_includes.h"

_BUTTON_CUS_Info MainMenuArray[CUS_MAIN_MENU_ICON_NUM];

MAIN_MENU_ICON_MEMDEV_DEF MainMenuIconBuffer[CUS_MAIN_MENU_ICON_NUM];

static s32 CusTouchStsFlag = 0;

/*deinit*/
void DeInitMainArray(void)
{
	u32 i;
	for(i=0;i<CUS_MAIN_MENU_ICON_NUM;i++)
	{
		MainMenuArray[i].SelState = RT_CUS_BTN_UNSEL;
	}	
}

/*init*/
void InitMainArray(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	

	pBitmap = &bmclock_pic;
	MainMenuArray[0].pUnSelBitmap = NULL;
	MainMenuArray[0].pBitmap = pBitmap;
	MainMenuArray[0].xPos = ((LCDXSize/2) - (pBitmap->XSize/2));
	MainMenuArray[0].yPos = ((LCDYSize/2) - 3*(pBitmap->YSize/2)-(pBitmap->YSize/4));
	MainMenuArray[0].TouchXmin = MainMenuArray[0].xPos;
	MainMenuArray[0].TouchXmax = MainMenuArray[0].xPos + pBitmap->XSize;
	MainMenuArray[0].TouchYmin = MainMenuArray[0].yPos;
	MainMenuArray[0].TouchYmax = MainMenuArray[0].yPos + pBitmap->YSize;
	MainMenuArray[0].ID = RT_CUS_CLOCK;	
	MainMenuArray[0].ProState = RT_CUS_CLOCK_PRO;

	pBitmap = &bmlight_pic;
	MainMenuArray[1].pBitmap = pBitmap;
	MainMenuArray[1].pUnSelBitmap = NULL;
	MainMenuArray[1].xPos = CUS_MAIN_EDGE_NUM;
	MainMenuArray[1].yPos = ((LCDYSize/2) - (pBitmap->YSize/2));
	MainMenuArray[1].TouchXmin = MainMenuArray[1].xPos;
	MainMenuArray[1].TouchXmax = MainMenuArray[1].xPos + pBitmap->XSize;
	MainMenuArray[1].TouchYmin = MainMenuArray[1].yPos;
	MainMenuArray[1].TouchYmax = MainMenuArray[1].yPos + pBitmap->YSize;
	MainMenuArray[1].ID = RT_CUS_LIGHT;
	MainMenuArray[1].ProState = RT_CUS_LIGHTCTRL_PRO;


	pBitmap = &bmtempre_pic;
	MainMenuArray[2].pBitmap = pBitmap;
	MainMenuArray[2].pUnSelBitmap = NULL;
	MainMenuArray[2].xPos = ((LCDXSize/2) - (pBitmap->XSize/2));
	MainMenuArray[2].yPos = ((LCDYSize/2) - (pBitmap->YSize/2));
	MainMenuArray[2].TouchXmin = MainMenuArray[2].xPos;
	MainMenuArray[2].TouchXmax = MainMenuArray[2].xPos + pBitmap->XSize;
	MainMenuArray[2].TouchYmin = MainMenuArray[2].yPos;
	MainMenuArray[2].TouchYmax = MainMenuArray[2].yPos + pBitmap->YSize;
	MainMenuArray[2].ID = RT_CUS_TEMPRE;
	MainMenuArray[2].ProState = RT_CUS_TEMPRE_PRO;


	pBitmap = &bmgradienter_pic;
	MainMenuArray[3].pUnSelBitmap = NULL;
	MainMenuArray[3].pBitmap = pBitmap;
	MainMenuArray[3].xPos = (LCDXSize - pBitmap->XSize-CUS_MAIN_EDGE_NUM);
	MainMenuArray[3].yPos = ((LCDYSize/2) - (pBitmap->YSize/2));
	MainMenuArray[3].TouchXmin = MainMenuArray[3].xPos;
	MainMenuArray[3].TouchXmax = MainMenuArray[3].xPos + pBitmap->XSize;
	MainMenuArray[3].TouchYmin = MainMenuArray[3].yPos;
	MainMenuArray[3].TouchYmax = MainMenuArray[3].yPos + pBitmap->YSize;
	MainMenuArray[3].ID = RT_CUS_GRAD;	
	MainMenuArray[3].ProState = RT_CUS_GRAD_PRO;


	/*wave*/
	pBitmap = &bmwave_pic;
	MainMenuArray[4].pUnSelBitmap = NULL;
	MainMenuArray[4].pBitmap = pBitmap;
	MainMenuArray[4].xPos = ((LCDXSize/2) - (pBitmap->XSize/2));
	MainMenuArray[4].yPos = ((LCDYSize/2) + 2*(pBitmap->YSize/2));
	MainMenuArray[4].TouchXmin = MainMenuArray[4].xPos;
	MainMenuArray[4].TouchXmax = MainMenuArray[4].xPos + pBitmap->XSize;
	MainMenuArray[4].TouchYmin = MainMenuArray[4].yPos;
	MainMenuArray[4].TouchYmax = MainMenuArray[4].yPos + pBitmap->YSize;
	MainMenuArray[4].ID = RT_CUS_WAVE;
	MainMenuArray[4].ProState = RT_CUS_WAVE_PRO;

	for(i=0;i<CUS_MAIN_MENU_ICON_NUM;i++)
	{
		MainMenuArray[i].SelState = RT_CUS_BTN_UNSEL;
	}
}

/*bk draw*/
void DrawCusMainBkWin(void)
{
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	

	GUI_SetColor(RT_DEFAULT_COLOR);
	GUI_DrawRect(2, 2, LCDXSize-3, LCDYSize-3);
	GUI_DrawRect(3, 3, LCDXSize-4, LCDYSize-4);
	GUI_DrawRect(5, 5, LCDXSize-6, LCDYSize-6);
}

void CusDispAllMainIcon(void)
{
	s32 i;

	for(i=0; i<CUS_MAIN_MENU_ICON_NUM; i++) 
	{
		CusCreateButton(&MainMenuArray[i]);
	}	
}

/*update button states*/
CUS_DEF_BUTTON_HANDLE CusMainUpdateButton(_BUTTON_CUS_Info * pButton)
{
	s32 ColorTemp;

	if(pButton->SelState == RT_CUS_BTN_UNSEL) {
		if(pButton->pUnSelBitmap!=NULL){
		} else {
			ColorTemp = GUI_GetColor();
			GUI_SetColor(GUI_GetBkColor());
			GUI_DrawRect((pButton->xPos-5), (pButton->yPos-5), (pButton->xPos+pButton->pBitmap->XSize+5),pButton->yPos+pButton->pBitmap->YSize+5);
			GUI_SetColor(ColorTemp);
		}
	} else {
		if(pButton->pBitmap!=NULL){
			ColorTemp = GUI_GetColor();
			GUI_SetColor(GUI_RED);
			GUI_DrawRect(pButton->xPos-5, pButton->yPos-5, pButton->xPos+pButton->pBitmap->XSize+5,pButton->yPos+pButton->pBitmap->YSize+5);
			GUI_SetColor(ColorTemp);
		} else {
			GUI_DispStringAt("no sel pic",pButton->xPos,pButton->yPos);
		}
	}

	return (CUS_DEF_BUTTON_HANDLE)pButton;
}


void CusMainMenuStoreAllIcon(void)
{
	s32 i;
	GUI_MEMDEV_Handle hMem;
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	_BUTTON_CUS_Info * pButton;

	for(i=0; i<CUS_MAIN_MENU_ICON_NUM; i++) {
		pButton = &MainMenuArray[i];
		TempMemXsize = pButton->pBitmap->XSize+2*CUS_BTN_SELECT_RANGE_X;
		TempMemYsize = pButton->pBitmap->YSize+2*CUS_BTN_SELECT_RANGE_Y;
		OrgX = pButton->xPos-CUS_BTN_SELECT_RANGE_X;
		OrgY = pButton->yPos-CUS_BTN_SELECT_RANGE_Y;
		MainMenuIconBuffer[i].Hdl = GUI_MEMDEV_CreateEx(OrgX,OrgY,TempMemXsize,TempMemYsize,0);
		MainMenuIconBuffer[i].Id = MainMenuArray[i].ID;
		GUI_MEMDEV_CopyFromLCD(MainMenuIconBuffer[i].Hdl);
	}
	GUI_SelectLCD();
}

/*selection process*/
void CusMainMenuSelMenuDraw(_BUTTON_CUS_Info * pButton)
{
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;


	TempMemXsize = pButton->pBitmap->XSize+2*CUS_BTN_SELECT_RANGE_X;
	TempMemYsize = pButton->pBitmap->YSize+2*CUS_BTN_SELECT_RANGE_Y;
	OrgX = pButton->xPos-CUS_BTN_SELECT_RANGE_X;
	OrgY = pButton->yPos-CUS_BTN_SELECT_RANGE_Y;


	hMemTemp = GUI_MEMDEV_CreateEx(OrgX,OrgY,TempMemXsize,TempMemYsize,0);		
	GUI_MEMDEV_Select(hMemTemp);
	GUI_SetBkColor(GUI_DARKMAGENTA);

	GUI_Clear();
	
	CusCreateButton(pButton);
	
	GUI_MEMDEV_CopyToLCD(hMemTemp);
	
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMemTemp);

}

/*cancel process*/
void CusMainMenuUnSelMenuDraw(_BUTTON_CUS_Info * pButton)
{
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;


	TempMemXsize = pButton->pBitmap->XSize+2*CUS_BTN_SELECT_RANGE_X;
	TempMemYsize = pButton->pBitmap->YSize+2*CUS_BTN_SELECT_RANGE_Y;
	OrgX = pButton->xPos-CUS_BTN_SELECT_RANGE_X;
	OrgY = pButton->yPos-CUS_BTN_SELECT_RANGE_Y;


	hMemTemp = GUI_MEMDEV_CreateEx(OrgX,OrgY,TempMemXsize,TempMemYsize,0);		
	GUI_MEMDEV_Select(hMemTemp);
	GUI_SetBkColor(CUS_MAIN_MENU_DEFAULT_BK_COLOR);
	GUI_Clear();
	CusCreateButton(pButton);
	
	GUI_MEMDEV_CopyToLCD(hMemTemp);
	
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMemTemp);
}

void CusMainMenuDispTargetIcon(_BUTTON_CUS_Info * pButton)
{
	CusMainMenuSelMenuDraw(pButton);
}

/*get press ID*/
s32 MainMenuGetPressId(touch_pos_type * PosTemp)
{
	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	Id = RT_CUS_MAIN_INVALID;
	
	for(i=0; i<CUS_MAIN_MENU_ICON_NUM; i++){
		Xmin = MainMenuArray[i].TouchXmin;
		Xmax = MainMenuArray[i].TouchXmax;
		Ymin = MainMenuArray[i].TouchYmin;
		Ymax = MainMenuArray[i].TouchYmax;
		if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = MainMenuArray[i].ID;
			break;
		}		
	}
	return Id;
}

_BUTTON_CUS_Info * MainMenuGetBtnFromId(s32 Id)
{
	u32 i;
	_BUTTON_CUS_Info * pBtn = NULL;
	
	for(i=0; i<CUS_MAIN_MENU_ICON_NUM; i++) {
		if(Id == MainMenuArray[i].ID) {
			pBtn = &MainMenuArray[i];
			break;
		}
	}

	return pBtn;
}

void CusGetTouchPos(GUI_PID_STATE * pSts, touch_pos_type * Pos)
{
	Pos->x = pSts->x;
	Pos->y = pSts->y;
}

s32 CusCheckTouchPressSts(GUI_PID_STATE * pSts)
{

	switch(CusTouchStsFlag){
		case 0:
			if(pSts->Pressed==1) {
				CusTouchStsFlag = 1;
			} else {
				return RT_CUS_NOT_TOUCHED;
			}
		break;

		case 1:
			if(pSts->Pressed==0) {
				CusTouchStsFlag = 0;
				return RT_CUS_TOUCHED_RELEASE;
			} else {
				return RT_CUS_TOUCHED_UNRELEASE;
			}
		break;

		default:
		break;
	}
	
}

void CusResetTouchPressSts(void)
{
	CusTouchStsFlag = 0;	
}


void MainMenuSingleClickPro(_BUTTON_CUS_Info * pBtn)
{
	u32 i;
	s32 PreIdFlag = 0;

	CusMainMenuSelMenuDraw(pBtn);
	pBtn->SelState = RT_CUS_BTN_SEL;
	for(i=0; i<CUS_MAIN_MENU_ICON_NUM; i++){
		if(MainMenuArray[i].ID != pBtn->ID) {
			if(MainMenuArray[i].SelState == RT_CUS_BTN_SEL) {
				CusMainMenuUnSelMenuDraw(&MainMenuArray[i]);
				MainMenuArray[i].SelState = RT_CUS_BTN_UNSEL;
			}
		}
	}
}

void MainMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_PID_STATE State;
	s32 MainSts;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	_BUTTON_CUS_Info * pBtn;
	while(1) {	
		GUI_Delay(5);
		GUI_TOUCH_GetState(&State);
		PressSts = CusCheckTouchPressSts(&State);
		if(PressSts == RT_CUS_TOUCHED_RELEASE) {
			CusGetTouchPos(&State, &Pos);
			IconId = MainMenuGetPressId(&Pos);
			if(RT_CUS_MAIN_INVALID != IconId) {
				pBtn = MainMenuGetBtnFromId(IconId);
				if(pBtn->SelState == RT_CUS_BTN_SEL) { 	/*double click process*/
					Sts->TransSts = pBtn->ProState;
					break;
				} else {							 	  /*single clock process*/
					MainMenuSingleClickPro(pBtn);
				}
			}
		}
	}
}

/*main menu process*/
void MainMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{
	InitMainArray();
	CusDispAllMainIcon();
	DrawCusMainBkWin();	
	ShowMainMenuStr();
	MainMenuStateTransfer(Sts);
	CusResetTouchPressSts();
	DeInitMainArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
}

