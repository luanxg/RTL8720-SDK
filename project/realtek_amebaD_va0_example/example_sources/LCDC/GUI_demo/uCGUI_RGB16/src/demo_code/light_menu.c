#include "GUI.h"
#include "rt_gui_demo_includes.h"

_BUTTON_CUS_Info CusRtLightReturnBtn;

#define CUS_LIGHT_COLOR_BAR_NUM   3

_BUTTON_CUS_Info LightSwicth;
LIGHT_PLATE_CUS_Info LightPlate;
CUS_COLOR_BAR_DEF_INFO CusLightBarArray[CUS_LIGHT_COLOR_BAR_NUM];
CUS_Light_BULB_DISP_DEF_INFO CusLightBultVar;

/*deinit*/
void DeInitLightMenuArray(void)
{
	LightSwicth.SelState = RT_CUS_BTN_UNSEL;
	LightPlate.SelState=RT_CUS_BTN_UNSEL;
}

/*init*/
void InitLightMenuArray(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	
	s32 Color[]={GUI_RED,GUI_GREEN,GUI_BLUE};
	s32 ColorRange[]={0x1f+1,0x3f+1,0x1f+1};
	s32 IdBuffer[]={RT_CUS_LIGHT_RED_BAR,RT_CUS_LIGHT_GREEN_BAR,RT_CUS_LIGHT_BLUE_BAR};


	/*switch*/
	pBitmap = &bmlight_on_pic;
	LightSwicth.pUnSelBitmap = &bmlight_off_pic;
	LightSwicth.pBitmap = pBitmap;
	LightSwicth.xPos = ((LCDXSize/2) - (pBitmap->XSize/2));
	LightSwicth.yPos = ((LCDYSize/2) + LCDYSize/4) ;
	LightSwicth.TouchXmin = LightSwicth.xPos;
	LightSwicth.TouchXmax = LightSwicth.xPos + pBitmap->XSize;
	LightSwicth.TouchYmin = LightSwicth.yPos;
	LightSwicth.TouchYmax = LightSwicth.yPos + pBitmap->YSize;
	LightSwicth.ID = RT_CUS_LIGHT_SWITCH;
	LightSwicth.SelState = RT_CUS_BTN_UNSEL;
	LightSwicth.ProState = NULL;

	/*ret*/
	pBitmap = &bmreturn_btn_pic;
	CusRtLightReturnBtn.pBitmap = pBitmap;
	CusRtLightReturnBtn.pUnSelBitmap = NULL;
	CusRtLightReturnBtn.xPos = (LCDXSize - pBitmap->XSize - CUS_LIGHT_RET_BTN_OFFSET);
	CusRtLightReturnBtn.yPos = (LCDYSize - pBitmap->YSize - CUS_LIGHT_RET_BTN_OFFSET);
	CusRtLightReturnBtn.TouchXmin = CusRtLightReturnBtn.xPos;
	CusRtLightReturnBtn.TouchXmax = CusRtLightReturnBtn.xPos + pBitmap->XSize;
	CusRtLightReturnBtn.TouchYmin = CusRtLightReturnBtn.yPos;
	CusRtLightReturnBtn.TouchYmax = CusRtLightReturnBtn.yPos + pBitmap->YSize;
	CusRtLightReturnBtn.ID = RT_CUS_LIGHT_RET;
	CusRtLightReturnBtn.ProState = RT_CUS_MAINMENU_PRO;
	CusRtLightReturnBtn.SelState = RT_CUS_BTN_UNSEL;

	/*plate*/
	LightPlate.OrgxPos=36;
	LightPlate.OrgyPos=200;
	LightPlate.XRange=150;
	LightPlate.YRange=150;

	LightPlate.XColor=GUI_RED;
	LightPlate.YColor=GUI_GREEN;

	LightPlate.FixColor=GUI_BLUE;
	LightPlate.TouchXmin=LightPlate.OrgxPos;
	LightPlate.TouchXmax=LightPlate.OrgxPos+LightPlate.XRange;
	LightPlate.TouchYmin=LightPlate.OrgyPos;
	LightPlate.TouchYmax=LightPlate.OrgyPos+LightPlate.YRange;
	LightPlate.ID=RT_CUS_LIGHT_PALET;
	LightPlate.SelState=RT_CUS_BTN_UNSEL;
	LightPlate.ProState=NULL;

	/*bars*/
	for(i=0; i<CUS_LIGHT_COLOR_BAR_NUM; i++) {
		CusLightBarArray[i].ColorRange = ColorRange[i];
		CusLightBarArray[i].BarOrgX = 60;
		CusLightBarArray[i].BarOrgY = 230+i*35;
		CusLightBarArray[i].BarXSize = 150;
		CusLightBarArray[i].BarYSize = 8;
		CusLightBarArray[i].BarFillColor = Color[i];

		CusLightBarArray[i].BtnRange = CusLightBarArray[i].BarXSize/2;
		CusLightBarArray[i].BtnX = CusLightBarArray[i].BarOrgX+CusLightBarArray[i].BtnRange;
		CusLightBarArray[i].BtnY = CusLightBarArray[i].BarOrgY+CusLightBarArray[i].BarYSize/2;
		CusLightBarArray[i].BtnR = 8;
		CusLightBarArray[i].BtnFillColor=Color[i];
			
		CusLightBarArray[i].CurColorVal=(CusLightBarArray[i].BtnRange*CusLightBarArray[i].ColorRange)/CusLightBarArray[i].BarXSize;
		CusLightBarArray[i].CurShowStrX = CusLightBarArray[i].BarOrgX+CusLightBarArray[i].BarXSize+16;
		CusLightBarArray[i].CurShowStrY= CusLightBarArray[i].BarOrgY;;
		CusLightBarArray[i].pFnt = &GUI_Font10_ASCII;	
		CusLightBarArray[i].TouchXmin = CusLightBarArray[i].BarOrgX;
		CusLightBarArray[i].TouchXmax = CusLightBarArray[i].BarOrgX+CusLightBarArray[i].BarXSize;
		CusLightBarArray[i].TouchYmin = CusLightBarArray[i].BarOrgY;
		CusLightBarArray[i].TouchYmax = CusLightBarArray[i].BarOrgY+CusLightBarArray[i].BarYSize;

		CusLightBarArray[i].ID = IdBuffer[i];
		CusLightBarArray[i].SelState = RT_CUS_BTN_UNSEL;
		CusLightBarArray[i].ProState = NULL;
	}

	CusLightBultVar.OrgX = 80;
	CusLightBultVar.OrgY = 160;
	CusLightBultVar.R_D_Y = 10;
	CusLightBultVar.R_D_X = 100;
	CusLightBultVar.R_Color = GUI_BLUE;
		
	CusLightBultVar.Ex = CusLightBultVar.OrgX+CusLightBultVar.R_D_X/2;
	CusLightBultVar.Ey = 90;
	CusLightBultVar.Edx = 30;
	CusLightBultVar.Edy = CusLightBultVar.OrgY-CusLightBultVar.Ey-CusLightBultVar.R_D_Y;
	CusLightBultVar.BulbCurFillColor = 0x0000FF;
	CusLightBultVar.BulbOutColor = GUI_WHITE;

	CusLightBultVar.TouchXmin = NULL;
	CusLightBultVar.TouchXmax = NULL;
	CusLightBultVar.TouchYmin = NULL;
	CusLightBultVar.TouchYmax = NULL;

	CusLightBultVar.ID = RT_CUS_LIGHT_BULB;
	CusLightBultVar.SelState = RT_CUS_BTN_UNSEL;
	CusLightBultVar.ProState = NULL;
	
}

/*get press ID*/
s32 LightMenuGetPressId(touch_pos_type * PosTemp)
{
	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	_BUTTON_CUS_Info * pLightSwicth;
	_BUTTON_CUS_Info * pLightRet;
	CUS_COLOR_BAR_DEF_INFO * pCusLightBarArray;;

	pLightSwicth = &LightSwicth;
	pLightRet = &CusRtLightReturnBtn;


	Id = RT_CUS_MAIN_INVALID;

	/*switch*/
	Xmin = pLightSwicth->TouchXmin;
	Xmax = pLightSwicth->TouchXmax;
	Ymin = pLightSwicth->TouchYmin;
	Ymax = pLightSwicth->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = pLightSwicth->ID;
			return Id;
	}	

	/*ret*/
	Xmin = pLightRet->TouchXmin;
	Xmax = pLightRet->TouchXmax;
	Ymin = pLightRet->TouchYmin;
	Ymax = pLightRet->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = pLightRet->ID;
			return Id;
	}

	/*bar*/
	for(i=0;i<CUS_LIGHT_COLOR_BAR_NUM;i++){
		pCusLightBarArray = &CusLightBarArray[i];
		Xmin = pCusLightBarArray->TouchXmin;
		Xmax = pCusLightBarArray->TouchXmax;
		Ymin = pCusLightBarArray->TouchYmin;
		Ymax = pCusLightBarArray->TouchYmax;
		if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = pCusLightBarArray->ID;
			break;
		}		
	}

	return Id;
}


void * LightMenuGetBtnFromId(s32 Id)
{
	u32 i;
	void * pBtn = NULL;

	if(Id==RT_CUS_LIGHT_SWITCH) {
		pBtn = (void *)&LightSwicth;
	}

	if(Id==RT_CUS_LIGHT_RET) {
		pBtn = (void *)&CusRtLightReturnBtn;
	}

	for(i=0; i<CUS_LIGHT_COLOR_BAR_NUM; i++) {
		if(Id == CusLightBarArray[i].ID) {
			pBtn = &CusLightBarArray[i];
			break;
		}
	}

	return pBtn;	
}

void LightMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_PID_STATE State;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	s32 CurTempColor=0;
	_BUTTON_CUS_Info * pLightSwicth;
	_BUTTON_CUS_Info * pRetBtn;
	CUS_COLOR_BAR_DEF_INFO * pCusLightBarArrayRed;
	CUS_COLOR_BAR_DEF_INFO * pCusLightBarArrayGreen;
	CUS_COLOR_BAR_DEF_INFO * pCusLightBarArrayBlue;
	CUS_Light_BULB_DISP_DEF_INFO * pCusLightBultVar;


	pLightSwicth = &LightSwicth;
	pCusLightBarArrayRed = &CusLightBarArray[0];
	pCusLightBarArrayGreen = &CusLightBarArray[1];
	pCusLightBarArrayBlue = &CusLightBarArray[2];
	pRetBtn = &CusRtLightReturnBtn;
	pCusLightBultVar = &CusLightBultVar;

	CurTempColor = ((pCusLightBarArrayBlue->CurColorVal<<19)|(pCusLightBarArrayGreen->CurColorVal<<10)|(pCusLightBarArrayRed->CurColorVal<<3));

	while(1){	
		GUI_Delay(10);
		GUI_TOUCH_GetState(&State);
		PressSts = CusCheckTouchPressSts(&State);
		if(PressSts == RT_CUS_TOUCHED_RELEASE) {
			CusGetTouchPos(&State, &Pos);
			IconId = LightMenuGetPressId(&Pos);
			if(RT_CUS_LIGHT_SWITCH == IconId) {
				pLightSwicth = LightMenuGetBtnFromId(IconId);
				if(pLightSwicth->SelState == RT_CUS_BTN_UNSEL) {
					if(pLightSwicth->SelState != RT_CUS_BTN_SEL) {
						pLightSwicth->SelState = RT_CUS_BTN_SEL;
						pCusLightBultVar->SelState = RT_CUS_BTN_SEL;
						CusUpdateLightBulbDev(pCusLightBultVar);
					}
				} else {
					pLightSwicth->SelState = RT_CUS_BTN_UNSEL;
					pCusLightBultVar->SelState = RT_CUS_BTN_UNSEL;
					CusUpdateLightBulbDev(pCusLightBultVar);
				}
				CusUpdateButtonSts(pLightSwicth);
			}


			if(RT_CUS_LIGHT_RET == IconId) {
				pRetBtn = LightMenuGetBtnFromId(IconId);
				if(pRetBtn->SelState == RT_CUS_BTN_UNSEL) {
					pRetBtn->SelState = RT_CUS_BTN_SEL;
				} else {
					pRetBtn->SelState = RT_CUS_BTN_UNSEL;				
				}
				Sts->TransSts = pRetBtn->ProState;
				break;
			}

		} else if(PressSts == RT_CUS_TOUCHED_UNRELEASE){
			if(pLightSwicth->SelState == RT_CUS_BTN_SEL) {
				CusGetTouchPos(&State, &Pos);
				IconId = LightMenuGetPressId(&Pos);
				
				if(IconId==RT_CUS_LIGHT_RED_BAR) {
					pCusLightBarArrayRed = LightMenuGetBtnFromId(IconId);
					pCusLightBarArrayRed->BtnRange = Pos.x-pCusLightBarArrayRed->BarOrgX;
					CusUpdateColorBarDx(pCusLightBarArrayRed);
				}

				if(IconId==RT_CUS_LIGHT_GREEN_BAR) {
					pCusLightBarArrayGreen = LightMenuGetBtnFromId(IconId);
					pCusLightBarArrayGreen->BtnRange = Pos.x-pCusLightBarArrayGreen->BarOrgX;
					CusUpdateColorBarDx(pCusLightBarArrayGreen);
				}

				if(IconId==RT_CUS_LIGHT_BLUE_BAR) {
					pCusLightBarArrayBlue = LightMenuGetBtnFromId(IconId);
					pCusLightBarArrayBlue->BtnRange = Pos.x-pCusLightBarArrayBlue->BarOrgX;
					CusUpdateColorBarDx(pCusLightBarArrayBlue);
				}

				if(CurTempColor!=((pCusLightBarArrayBlue->CurColorVal<<19)|(pCusLightBarArrayGreen->CurColorVal<<10)|(pCusLightBarArrayRed->CurColorVal<<3))){
					CurTempColor = ((pCusLightBarArrayBlue->CurColorVal<<19)|(pCusLightBarArrayGreen->CurColorVal<<10)|(pCusLightBarArrayRed->CurColorVal<<3));
					pCusLightBultVar->BulbCurFillColor = CurTempColor;
					CusUpdateLightBulbDev(pCusLightBultVar);
				}
				
			}

		} else {
			
		}
	}
	
}	

/*light menu process*/
void LightMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{
	_BUTTON_CUS_Info * pLightSwicth;
	LIGHT_PLATE_CUS_Info * pLightPlate;

	int NumColors = LCD_GetDevCap(LCD_DEVCAP_NUMCOLORS);
	int xsize = LCD_GetDevCap(LCD_DEVCAP_XSIZE);

	pLightSwicth = &LightSwicth;
	pLightPlate = &LightPlate;

	GUI_SetBkColor(GUI_BLACK);
	GUI_SetColor(GUI_GREEN);
	GUI_Clear();
	
	InitLightMenuArray();
	CusCreateColorBar(&CusLightBarArray[0]);
	CusCreateColorBar(&CusLightBarArray[1]);
	CusCreateColorBar(&CusLightBarArray[2]);
	CusCreateButton(&CusRtLightReturnBtn);
	CusCreateButton(&LightSwicth);
	CusCreateLightBulbDev(&CusLightBultVar);
	ShowLightMenuStr();

	LightMenuStateTransfer(Sts);

	CusResetTouchPressSts();
	DeInitLightMenuArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
}

