#include "GUI.h"
#include "rt_gui_demo_includes.h"

CUS_GRAD_PLATE_Info CusGradVar;
_BUTTON_CUS_Info CusRtGradReturnBtn;

void DeInitGradMenuArray(void)
{
	CusGradVar.SelState=RT_CUS_BTN_UNSEL;
	CusRtGradReturnBtn.SelState = RT_CUS_BTN_UNSEL;
}

void InitGradMenuArray(void)
{
	int i;
	int ImgXSize;
	int ImgYSize;
	const GUI_BITMAP *pBitmap;
	int LCDXSize = LCD_GET_XSIZE();
	int LCDYSize = LCD_GET_YSIZE();	

	s32 OrgX;
	s32 OrgY;

	/*grad*/
	CusGradVar.X0 = 136;
	CusGradVar.Y0=130;
	CusGradVar.R0=30;
	CusGradVar.Color0=GUI_RED;

	CusGradVar.X1=136;
	CusGradVar.Y1=130;
	CusGradVar.R1=30;
	CusGradVar.Color1=GUI_BLUE;

	CusGradVar.RangeXmin = 86;
	CusGradVar.RangeYmin = 80;
	CusGradVar.RangeXmax = 186;
	CusGradVar.RangeYmax = 180;

	CusGradVar.BkColor=GUI_BLACK;
	
	CusGradVar.CIntvl=20;
	CusGradVar.ClCnt=1000;

	CusGradVar.CombineClr=GUI_GREEN;
	CusGradVar.Angle=0;
	CusGradVar.OrgDx=0;
	
	CusGradVar.TouchXmin=CusGradVar.RangeXmin;
	CusGradVar.TouchXmax=CusGradVar.RangeXmax;
	CusGradVar.TouchYmin=CusGradVar.RangeYmin;
	CusGradVar.TouchYmax=CusGradVar.RangeYmax;
	CusGradVar.ID=RT_CUS_GRAD_PLATE_ICON;
	CusGradVar.SelState=RT_CUS_BTN_UNSEL;
	CusGradVar.ProState=NULL;	

	/*ret*/
	pBitmap = &bmreturn_btn_pic;
	CusRtGradReturnBtn.pBitmap = pBitmap;
	
	CusRtGradReturnBtn.pUnSelBitmap = NULL;
	CusRtGradReturnBtn.xPos = (LCDXSize - pBitmap->XSize - CUS_GRAD_RET_BTN_OFFSET);
	CusRtGradReturnBtn.yPos = (LCDYSize - pBitmap->YSize - CUS_GRAD_RET_BTN_OFFSET);
	CusRtGradReturnBtn.TouchXmin = CusRtGradReturnBtn.xPos;
	CusRtGradReturnBtn.TouchXmax = CusRtGradReturnBtn.xPos + pBitmap->XSize;
	CusRtGradReturnBtn.TouchYmin = CusRtGradReturnBtn.yPos;
	CusRtGradReturnBtn.TouchYmax = CusRtGradReturnBtn.yPos + pBitmap->YSize;
	CusRtGradReturnBtn.ID = RT_CUS_GRAD_RET;
	CusRtGradReturnBtn.ProState = RT_CUS_MAINMENU_PRO;
	CusRtGradReturnBtn.SelState = RT_CUS_BTN_UNSEL;
}
void GradMenuShowAngle(CUS_GRAD_PLATE_Info * pBtn)
{
	s32 X;
	s32 Y;
	GUI_SetFont(&GUI_FontComic24B_ASCII);
	GUI_SetColor(GUI_RED);
	X = pBtn->RangeXmin+30;
	Y = pBtn->RangeYmax+100;
	GUI_ClearRect(X-5,Y-5,X+50,Y+50);
	GUI_DispDecAt(pBtn->OrgDx,X,Y,2);GUI_DispStringAt("CEL",X+30,Y);
}

s32 GradMenuGetPressId(touch_pos_type * PosTemp)
{
	u32 i;
	s32 Id;
	s32 Xmin;
	s32 Xmax;
	s32 Ymin;
	s32 Ymax;
	_BUTTON_CUS_Info * pRet;
	Id = RT_CUS_MAIN_INVALID;
	pRet = &CusRtGradReturnBtn;

	/*ret*/
	Xmin = pRet->TouchXmin;
	Xmax = pRet->TouchXmax;
	Ymin = pRet->TouchYmin;
	Ymax = pRet->TouchYmax;
	if(((PosTemp->x>=Xmin)&&(PosTemp->x<=Xmax))&&((PosTemp->y>=Ymin)&&(PosTemp->y<=Ymax))) {
			Id = RT_CUS_GRAD_RET;
			return Id;
	}	

	return Id;
}

void * GradMenuGetBtnFromId(s32 Id)
{
	u32 i;
	void * pBtn = NULL;

	if(Id==RT_CUS_GRAD_RET) {
		pBtn = (void *)&CusRtGradReturnBtn;
	}
	return pBtn;	
}

void GradMenuStateTransfer(RT_CUS_DEMO_STS_TYPE * Sts)
{

	GUI_PID_STATE State;
	s32 PressSts;
	touch_pos_type Pos;
	s32 IconId;
	CUS_GRAD_PLATE_Info * pCusGradVar;
	s32 i,j;
	s32 Cnt=0;
	s32	Ucnt;
	s32 ShowCnt=0;
	_BUTTON_CUS_Info * pBtn;
	pBtn = &CusRtGradReturnBtn;

	i=0;j=0;
	pCusGradVar=&CusGradVar;

	pCusGradVar->X0 = (pCusGradVar->RangeXmax+pCusGradVar->RangeXmin)/2;
	pCusGradVar->Y0 = (pCusGradVar->RangeYmax+pCusGradVar->RangeYmin)/2;
	pCusGradVar->X1 = (pCusGradVar->RangeXmax+pCusGradVar->RangeXmin)/2;
	pCusGradVar->Y1 = (pCusGradVar->RangeYmax+pCusGradVar->RangeYmin)/2;	
	CusGradMenuShowPlateUpdate(pCusGradVar);
	pCusGradVar->OrgMaxDx = (pCusGradVar->RangeXmax-pCusGradVar->RangeXmin)/4;
	Ucnt = (pCusGradVar->RangeXmax-pCusGradVar->RangeXmin)/2;
	while(1) 
	{
		GUI_TOUCH_GetState(&State);
		PressSts = CusCheckTouchPressSts(&State);

		if(PressSts == RT_CUS_TOUCHED_RELEASE) {
			CusGetTouchPos(&State, &Pos);
			IconId = GradMenuGetPressId(&Pos);
			if(IconId==RT_CUS_GRAD_RET) {
				pBtn = GradMenuGetBtnFromId(IconId);
				if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
					pBtn->SelState = RT_CUS_BTN_SEL;
				} else {
					pBtn->SelState = RT_CUS_BTN_UNSEL;				
				}
				Sts->TransSts = pBtn->ProState;
				break;
			}
		}
		switch(Cnt){
			case 0:
				ShowCnt++;
				if(ShowCnt%3==0){
					pCusGradVar->X0--;
					pCusGradVar->X1++;
					pCusGradVar->OrgDx=j;
					CusGradMenuShowPlateUpdate(pCusGradVar);
					j++;
					if(j>=Ucnt){
						Cnt=1;
						j=0;
						Ucnt = (pCusGradVar->RangeXmax-pCusGradVar->RangeXmin)/2;
						ShowCnt=0;
					}
				}
			break;
			
			case 1:
				ShowCnt++;
				if(ShowCnt%3==0){
					pCusGradVar->X0++;
					pCusGradVar->X1--;
					pCusGradVar->OrgDx=j;
					if(ShowCnt%3==0){
						CusGradMenuShowPlateUpdate(pCusGradVar);
					}
					j++;
					if(j>=Ucnt){
						Cnt=0;
						j=0;
						Ucnt = (pCusGradVar->RangeXmax-pCusGradVar->RangeXmin)/2;
						ShowCnt=0;
					}	
				}
				
			break;
			
			case 2:
				ShowCnt++;
				if(ShowCnt%3==0){
					pCusGradVar->Y0--;
					pCusGradVar->Y1++;
					pCusGradVar->OrgDx=j;
					if(ShowCnt%3==0){
						CusGradMenuShowPlateUpdate(pCusGradVar);
					}
					j++;
					if(j>=Ucnt){
						Cnt=3;
						j=0;
						Ucnt = (pCusGradVar->RangeYmax-pCusGradVar->RangeYmin)/4;
						ShowCnt=0;
					}	
				}

			break;
			
			case 3:
				ShowCnt++;
				if(ShowCnt%3==0){
					pCusGradVar->Y0++;
					pCusGradVar->Y1--;
					pCusGradVar->OrgDx=j;
					if(ShowCnt%3==0){
						CusGradMenuShowPlateUpdate(pCusGradVar);
					}
					j++;
					if(j>=Ucnt){
						Cnt=0;
						j=0;
						Ucnt = (pCusGradVar->RangeXmax-pCusGradVar->RangeXmin)/4;
						ShowCnt=0;
					}	
				}
			break;
				
			default:
				
			break;
		}
		GradMenuShowAngle(pCusGradVar);
		GUI_Delay(20);
	}
}	
/*gradienter menu process*/
void GradMenuCusPro(RT_CUS_DEMO_STS_TYPE * Sts)
{
	GUI_SetBkColor(GUI_BLACK);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
	InitGradMenuArray();
	CusGradMenuShowPlate(&CusGradVar);
	CusCreateButton(&CusRtGradReturnBtn);
	ShowGradMenuStr();
	GradMenuStateTransfer(Sts);
	CusResetTouchPressSts();
	DeInitGradMenuArray();
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);
	GUI_Clear();
}

