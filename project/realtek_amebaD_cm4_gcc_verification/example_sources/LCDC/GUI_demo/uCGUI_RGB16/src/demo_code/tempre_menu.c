#include "GUI.h"
#include "rt_gui_demo_includes.h"

BUTTON_CIRCLE_CUS_Info CusRtCirTempreBtn[TEMPRE_MENU_CIRCLE_BTN_NUM];
_BUTTON_CUS_Info CusRtTempreReturnBtn[TEMPRE_MENU_RET_BTN_NUM];
TEMPRE_CUS_ICON_Info CusRtTempreIcn;

u32 CusTestCurTempreValue =50;
	
void InitTempreArray(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	

	s32 OrgX;
	s32 OrgY;

	OrgX = CUS_TEMPRE_CIRCLE_WORK_BTN_X;
	OrgY = CUS_TEMPRE_CIRCLE_WORK_BTN_Y;
	CusRtCirTempreBtn[0].OrgxPos = OrgX;
	CusRtCirTempreBtn[0].OrgyPos = OrgY;
	CusRtCirTempreBtn[0].InnerR = CUS_TEMPRE_CIRCLE_WORK_IN_R;
	CusRtCirTempreBtn[0].OuterR = CUS_TEMPRE_CIRCLE_WORK_OUT_R;
	CusRtCirTempreBtn[0].Fontp = CUS_TEMPRE_CIRCLE_WORK_FNT_TYP;
	CusRtCirTempreBtn[0].FntColor = CUS_TEMPRE_CIRCLE_WORK_FNT_COLR;
	CusRtCirTempreBtn[0].CtrColor = CUS_TEMPRE_CIRCLE_WORK_CEN_DISCOLR;
	CusRtCirTempreBtn[0].EdgeColor = CUS_TEMPRE_CIRCLE_WORK_EDG_COLR;
	CusRtCirTempreBtn[0].pBitmap = NULL;
	CusRtCirTempreBtn[0].StrInBtn = "WORK";
	CusRtCirTempreBtn[0].TouchXmin = OrgX-CusRtCirTempreBtn[0].InnerR;
	CusRtCirTempreBtn[0].TouchXmax = OrgX+CusRtCirTempreBtn[0].InnerR;
	CusRtCirTempreBtn[0].TouchYmin = OrgY-CusRtCirTempreBtn[0].InnerR;
	CusRtCirTempreBtn[0].TouchYmax = OrgY+CusRtCirTempreBtn[0].InnerR;
	CusRtCirTempreBtn[0].ID = RT_CUS_CIR_TEMPRE_WORK;
	CusRtCirTempreBtn[0].SelState = RT_CUS_BTN_UNSEL;
	CusRtCirTempreBtn[0].ProState = RT_CUS_TEMPRE_WORK_PRO;

	OrgX = CUS_TEMPRE_CIRCLE_TEST_BTN_X;
	OrgY = CUS_TEMPRE_CIRCLE_TEST_BTN_Y;
	CusRtCirTempreBtn[1].OrgxPos = OrgX;
	CusRtCirTempreBtn[1].OrgyPos = OrgY;
	CusRtCirTempreBtn[1].InnerR = CUS_TEMPRE_CIRCLE_TEST_IN_R;
	CusRtCirTempreBtn[1].OuterR = CUS_TEMPRE_CIRCLE_TEST_OUT_R;
	CusRtCirTempreBtn[1].Fontp = CUS_TEMPRE_CIRCLE_TEST_FNT_TYP;
	CusRtCirTempreBtn[1].FntColor = CUS_TEMPRE_CIRCLE_TEST_FNT_COLR;
	CusRtCirTempreBtn[1].CtrColor = CUS_TEMPRE_CIRCLE_TEST_CEN_DISCOLR;
	CusRtCirTempreBtn[1].EdgeColor = CUS_TEMPRE_CIRCLE_TEST_EDG_COLR;
	CusRtCirTempreBtn[1].pBitmap = NULL;
	CusRtCirTempreBtn[1].StrInBtn = "TEST";
	CusRtCirTempreBtn[1].TouchXmin = OrgX-CusRtCirTempreBtn[1].InnerR;
	CusRtCirTempreBtn[1].TouchXmax = OrgX+CusRtCirTempreBtn[1].InnerR;
	CusRtCirTempreBtn[1].TouchYmin = OrgY-CusRtCirTempreBtn[1].InnerR;
	CusRtCirTempreBtn[1].TouchYmax = OrgY+CusRtCirTempreBtn[1].InnerR;
	CusRtCirTempreBtn[1].ID = RT_CUS_CIR_TEMPRE_TEST;
	CusRtCirTempreBtn[1].SelState = RT_CUS_BTN_UNSEL;
	CusRtCirTempreBtn[1].ProState = RT_CUS_TEMPRE_TEST_PRO;	

	/*return btn*/
	pBitmap = &bmreturn_btn_pic;
	CusRtTempreReturnBtn[0].pBitmap = pBitmap;
	CusRtTempreReturnBtn[0].pUnSelBitmap = NULL;
	CusRtTempreReturnBtn[0].xPos = (LCDXSize - pBitmap->XSize - CUS_TEMPRE_RET_BTN_OFFSET);
	CusRtTempreReturnBtn[0].yPos = (LCDYSize - pBitmap->YSize - CUS_TEMPRE_RET_BTN_OFFSET);
	CusRtTempreReturnBtn[0].TouchXmin = CusRtTempreReturnBtn[0].xPos;
	CusRtTempreReturnBtn[0].TouchXmax = CusRtTempreReturnBtn[0].xPos + pBitmap->XSize;
	CusRtTempreReturnBtn[0].TouchYmin = CusRtTempreReturnBtn[0].yPos;
	CusRtTempreReturnBtn[0].TouchYmax = CusRtTempreReturnBtn[0].yPos + pBitmap->YSize;
	CusRtTempreReturnBtn[0].ID = RT_CUS_CIR_TEMPRE_RET;
	CusRtTempreReturnBtn[0].ProState = RT_CUS_MAINMENU_PRO;
	CusRtTempreReturnBtn[0].SelState = RT_CUS_BTN_UNSEL;

	/*tempre*/
	CusRtTempreIcn.OrgX = 105;
	CusRtTempreIcn.OrgY = 270;
	CusRtTempreIcn.L_Dx = 5;
	CusRtTempreIcn.G_Dx = 9;
	CusRtTempreIcn.G_Color = 0xD1EBF1;
	CusRtTempreIcn.F_Dx = 5;
	CusRtTempreIcn.Lin_Dx = 14;
	CusRtTempreIcn.Lin_S_Dx = 5;
	CusRtTempreIcn.Lin_L_Dx = 10;
	CusRtTempreIcn.L_Color = GUI_RED;
	CusRtTempreIcn.C_X = CusRtTempreIcn.OrgX;
	CusRtTempreIcn.C_Y = 310;
	CusRtTempreIcn.C_R = 35;
	CusRtTempreIcn.C_Color = GUI_RED;	
	CusRtTempreIcn.E_Dx = 10;
	CusRtTempreIcn.S_R_Dx = 65;
	CusRtTempreIcn.S_L_Dx = 95;
	CusRtTempreIcn.S_D_Dx = 50;
	CusRtTempreIcn.S_Len_Dx = 20;
	CusRtTempreIcn.S_La_Color = GUI_RED;	
	CusRtTempreIcn.L_D_Dx = 50;
	CusRtTempreIcn.H_Dx = 250;
	CusRtTempreIcn.TemprMin = 0;
	CusRtTempreIcn.TemprMax = 100;
	CusRtTempreIcn.Fontp = &GUI_Font8x8;
	CusRtTempreIcn.FntColor = GUI_BLACK;
	CusRtTempreIcn.TouchXmin = CusRtTempreIcn.OrgX-CusRtTempreIcn.S_L_Dx;
	CusRtTempreIcn.TouchXmax =  CusRtTempreIcn.OrgX-CusRtTempreIcn.S_R_Dx;
	CusRtTempreIcn.TouchYmin = CusRtTempreIcn.OrgY-CusRtTempreIcn.H_Dx;
	CusRtTempreIcn.TouchYmax = CusRtTempreIcn.OrgY; //CusRtTempreIcn.OrgY-CusRtTempreIcn.H_Dx;
	CusRtTempreIcn.CurTempre = 0;
	CusRtTempreIcn.CurSetTempre = (CusRtTempreIcn.TemprMax-CusRtTempreIcn.TemprMin)/2;
	CusRtTempreIcn.WarningSts = CUS_TEMPRE_NOT_WARNING;
	CusRtTempreIcn.ID = RT_CUS_CIR_TEMPRE_ICON;
	CusRtTempreIcn.SelState = RT_CUS_BTN_UNSEL;
	CusRtTempreIcn.ProState = RT_CUS_TEMPRE_PRO;
}

void DeInitTempreArray(void)
{
	CusRtCirTempreBtn[0].SelState = RT_CUS_BTN_UNSEL;	
	CusRtCirTempreBtn[1].SelState = RT_CUS_BTN_UNSEL;
	CusRtTempreReturnBtn[0].SelState = RT_CUS_BTN_UNSEL;
	CusRtTempreIcn.SelState = RT_CUS_BTN_UNSEL;
}

s32 TempreMenuGetPressId(touch_pos_type * PosTemp)
{
	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	BUTTON_CIRCLE_CUS_Info * pCirlWork;
	BUTTON_CIRCLE_CUS_Info * pCirlTest;
	_BUTTON_CUS_Info * pRetBtn;
	TEMPRE_CUS_ICON_Info * pTempIcn;
	pCirlWork = &CusRtCirTempreBtn[0];
	pCirlTest = &CusRtCirTempreBtn[1];
	pRetBtn = &CusRtTempreReturnBtn[0];
	pTempIcn = &CusRtTempreIcn;
	Id = RT_CUS_MAIN_INVALID;

	/*circle work*/
	Xmin = pCirlWork->TouchXmin;
	Xmax = pCirlWork->TouchXmax;
	Ymin = pCirlWork->TouchYmin;
	Ymax = pCirlWork->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_CIR_TEMPRE_WORK;
			return Id;
	}		

	/*circle test*/
	Xmin = pCirlTest->TouchXmin;
	Xmax = pCirlTest->TouchXmax;
	Ymin = pCirlTest->TouchYmin;
	Ymax = pCirlTest->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_CIR_TEMPRE_TEST;
			return Id;
	}	

	/*ret*/
	Xmin = pRetBtn->TouchXmin;
	Xmax = pRetBtn->TouchXmax;
	Ymin = pRetBtn->TouchYmin;
	Ymax = pRetBtn->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_CIR_TEMPRE_RET;
			return Id;
	}

	/*Icon*/
	Xmin = pTempIcn->TouchXmin;
	Xmax = pTempIcn->TouchXmax;
	Ymin = pTempIcn->TouchYmin;
	Ymax = pTempIcn->TouchYmax;

	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_CIR_TEMPRE_ICON;
			return Id;
	}

	return Id;
}

void * TempreMenuGetBtnFromId(s32 Id)
{
	u32 i;
	void * pBtn = NULL;

	if(Id==RT_CUS_CIR_TEMPRE_WORK) {
		pBtn = (void *)&CusRtCirTempreBtn[0];
	}

	if(Id==RT_CUS_CIR_TEMPRE_TEST) {
		pBtn = (void *)&CusRtCirTempreBtn[1];
	}

	if(Id==RT_CUS_CIR_TEMPRE_RET) {
		pBtn = (void *)&CusRtTempreReturnBtn[0];		
	}

	if(Id==RT_CUS_CIR_TEMPRE_ICON) {
		pBtn = (void *)&CusRtTempreIcn;		
	}

	return pBtn;
}


void TempreMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_PID_STATE State;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	BUTTON_CIRCLE_CUS_Info * pCirlWork;
	BUTTON_CIRCLE_CUS_Info * pCirlTest;
	_BUTTON_CUS_Info * pRetBtn;
	TEMPRE_CUS_ICON_Info * pTempIcn;
	u32 Cnt=0;
	s32 CusTempSet;
	s32 CusTempre;
	s32 ShowTempreSet;
	s32 ShowTempre;
	pCirlWork = &CusRtCirTempreBtn[0];
	pCirlTest = &CusRtCirTempreBtn[1];
	pRetBtn = &CusRtTempreReturnBtn[0];
	pTempIcn = &CusRtTempreIcn;

	ShowTempreSet = pTempIcn->CurSetTempre;
	ShowTempre = pTempIcn->CurTempre;

	CusTempreIconUpdateSetScale(pTempIcn);
	CusTempreIconUpdateScale(pTempIcn); 	
	
	while(1) {	
		GUI_Delay(10);
		GUI_TOUCH_GetState(&State);
		PressSts = CusCheckTouchPressSts(&State);
		if(PressSts == RT_CUS_TOUCHED_RELEASE) {
			CusGetTouchPos(&State, &Pos);
			IconId = TempreMenuGetPressId(&Pos);
			if(RT_CUS_CIR_TEMPRE_WORK == IconId) {
				pCirlWork = TempreMenuGetBtnFromId(IconId);
				if(pCirlWork->SelState == RT_CUS_BTN_UNSEL) {
					if(pCirlTest->SelState != RT_CUS_BTN_SEL) {
						pCirlWork->SelState = RT_CUS_BTN_SEL;
					}
				} else {
					pCirlWork->SelState = RT_CUS_BTN_UNSEL;				
				}
				CusCircleUpdateButtonSts(pCirlWork);
			}

			if(RT_CUS_CIR_TEMPRE_TEST == IconId) {
				pCirlTest = TempreMenuGetBtnFromId(IconId);
				if(pCirlTest->SelState == RT_CUS_BTN_UNSEL) {
					if(pCirlWork->SelState != RT_CUS_BTN_SEL) {					
						pCirlTest->SelState = RT_CUS_BTN_SEL;
					}
				} else {
					pCirlTest->SelState = RT_CUS_BTN_UNSEL;				
				}
				CusCircleUpdateButtonSts(pCirlTest);
			}

			if(RT_CUS_CIR_TEMPRE_RET == IconId) {
				pRetBtn = TempreMenuGetBtnFromId(IconId);
				if(pRetBtn->SelState == RT_CUS_BTN_UNSEL) {
					pRetBtn->SelState = RT_CUS_BTN_SEL;
				} else {
					pRetBtn->SelState = RT_CUS_BTN_UNSEL;				
				}
				Sts->TransSts = pRetBtn->ProState;
				break;
			}

			#if 0
			if(RT_CUS_MAIN_INVALID != IconId) {
				pCirlWork = TempreMenuGetBtnFromId(IconId);
				if(pCirlWork->SelState == RT_CUS_BTN_SEL) { /*double click process*/
					Sts->TransSts = pCirlWork->ProState;
					break;
				} else {							   /*single clock process*/
					MainMenuSingleClickPro(pCirlWork);
				}
			}
			#endif
		} else if(PressSts == RT_CUS_TOUCHED_UNRELEASE){
			CusGetTouchPos(&State, &Pos);
			IconId = TempreMenuGetPressId(&Pos);
			if(IconId==RT_CUS_CIR_TEMPRE_ICON) {
				pTempIcn = TempreMenuGetBtnFromId(IconId);
				pTempIcn->S_D_Dx = pTempIcn->OrgY-Pos.y+1;
				CusTempreIconUpdateSetScaleDx(pTempIcn);
			}

			if(pCirlTest->SelState == RT_CUS_BTN_SEL) {
				//GUI_DispDecAt(pCirlTest->SelState,0,0,3);
				//GUI_DispDecAt(CusTestCurTempreValue,0,50,3);
				if(CusTestCurTempreValue>100) {
					CusTestCurTempreValue=0;
				}
				Cnt++;
				if(Cnt%10==0) {
					CusTestCurTempreValue++;
				}
				CusTempre = CusTestCurTempreValue;
				if((CusTempre != pTempIcn->CurTempre)&&((CusTempre>=pTempIcn->TemprMin)&&(CusTempre<=pTempIcn->TemprMax))) {
						pTempIcn->CurTempre = CusTempre;
						CusTempreIconUpdateScale(pTempIcn);
				}	
			}
			
			if(pCirlWork->SelState == RT_CUS_BTN_SEL) {
				CusTempre = CusTestCurTempreValue;
				if((CusTempre != pTempIcn->CurTempre)&&((CusTempre>=pTempIcn->TemprMin)&&(CusTempre<=pTempIcn->TemprMax))) {
					pTempIcn->CurTempre = CusTempre;
					CusTempreIconUpdateScale(pTempIcn);
				}				
			}

			if(ShowTempreSet!=pTempIcn->CurSetTempre) {
				ShowTempreSet = pTempIcn->CurSetTempre;
				CusTempreIconShowSetNum(pTempIcn);
			}

			if(ShowTempre!=pTempIcn->CurTempre) {
				ShowTempre = pTempIcn->CurTempre;
				CusTempreIconShowCurNum(pTempIcn);
			}

			if((pCirlWork->SelState==RT_CUS_BTN_SEL)||(pCirlTest->SelState==RT_CUS_BTN_SEL)) {
				if(pTempIcn->CurTempre>=pTempIcn->CurSetTempre) {
					if(pTempIcn->WarningSts==CUS_TEMPRE_NOT_WARNING) {
						pTempIcn->WarningSts = CUS_TEMPRE_WARNING;
						CusTempreIconShowWarning(pTempIcn);
					}
				} else {
					if(pTempIcn->WarningSts==CUS_TEMPRE_WARNING) {
						pTempIcn->WarningSts = CUS_TEMPRE_NOT_WARNING;
						CusTempreIconShowWarning(pTempIcn);
					}					
				}				
			}

		} else {
			if(pCirlTest->SelState == RT_CUS_BTN_SEL) {
				if(CusTestCurTempreValue>100) {
					CusTestCurTempreValue=0;
				}
				Cnt++;
				if(Cnt%10==0) {
					CusTestCurTempreValue++;
				}
				CusTempre = CusTestCurTempreValue;
				if((CusTempre != pTempIcn->CurTempre)&&((CusTempre>=pTempIcn->TemprMin)&&(CusTempre<=pTempIcn->TemprMax))) {
						pTempIcn->CurTempre = CusTempre;
						CusTempreIconUpdateScale(pTempIcn);
				}	
			}
			
			if(pCirlWork->SelState == RT_CUS_BTN_SEL) {
				CusTempre = CusTestCurTempreValue;
				if((CusTempre != pTempIcn->CurTempre)&&((CusTempre>=pTempIcn->TemprMin)&&(CusTempre<=pTempIcn->TemprMax))) {
					pTempIcn->CurTempre = CusTempre;
					CusTempreIconUpdateScale(pTempIcn);
				}				
			}

			if(ShowTempreSet!=pTempIcn->CurSetTempre) {
				ShowTempreSet = pTempIcn->CurSetTempre;
				CusTempreIconShowSetNum(pTempIcn);
			}

			if(ShowTempre!=pTempIcn->CurTempre) {
				ShowTempre = pTempIcn->CurTempre;
				CusTempreIconShowCurNum(pTempIcn);
			}			

			if((pCirlWork->SelState==RT_CUS_BTN_SEL)||(pCirlTest->SelState==RT_CUS_BTN_SEL)) {
				if(pTempIcn->CurTempre>=pTempIcn->CurSetTempre) {
					if(pTempIcn->WarningSts==CUS_TEMPRE_NOT_WARNING) {
						pTempIcn->WarningSts = CUS_TEMPRE_WARNING;
						CusTempreIconShowWarning(pTempIcn);
					}
				} else {
					if(pTempIcn->WarningSts==CUS_TEMPRE_WARNING) {
						pTempIcn->WarningSts = CUS_TEMPRE_NOT_WARNING;
						CusTempreIconShowWarning(pTempIcn);
					}					
				}				
			}
		}
	}
}

void TempreMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{

	InitTempreArray();
	CusCreateCircleButton(&CusRtCirTempreBtn[0]);
	CusCreateCircleButton(&CusRtCirTempreBtn[1]);
	CusCreateButton(&CusRtTempreReturnBtn[0]);
	CusCreateTempreIcon(&CusRtTempreIcn);
	CusTempreIconShowSetNum(&CusRtTempreIcn);
	CusTempreIconShowCurNum(&CusRtTempreIcn);
	ShowTempreMenuStr();
	TempreMenuStateTransfer(&RtCusDemoVar);
	CusResetTouchPressSts();
	DeInitTempreArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
}

