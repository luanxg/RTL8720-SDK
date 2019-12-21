#include "GUI.h"
#include "rt_gui_demo_includes.h"

/*create button*/
unsigned int CusCreateButton(_BUTTON_CUS_Info * pButton)
{
	if(pButton->SelState == RT_CUS_BTN_UNSEL) {
		if(pButton->pUnSelBitmap!=NULL){
			GUI_DrawBitmap(pButton->pUnSelBitmap, pButton->xPos, pButton->yPos);
		} else {
			GUI_DrawBitmap(pButton->pBitmap, pButton->xPos, pButton->yPos);
		}
	} else {
		if(pButton->pBitmap!=NULL){
			GUI_DrawBitmap(pButton->pBitmap, pButton->xPos, pButton->yPos);
		} else {
			GUI_DispStringAt("no sel pic",pButton->xPos,pButton->yPos);
		}
	}

	return (CUS_DEF_BUTTON_HANDLE)pButton;
}

/*update button states*/
unsigned int CusUpdateButtonSts(_BUTTON_CUS_Info * pButton)
{
	if(pButton->SelState == RT_CUS_BTN_UNSEL) {
		if(pButton->pUnSelBitmap!=NULL){
			GUI_DrawBitmap(pButton->pUnSelBitmap, pButton->xPos, pButton->yPos);
		} else {
			GUI_DrawBitmap(pButton->pBitmap, pButton->xPos, pButton->yPos);
		}
	} else {
		if(pButton->pBitmap!=NULL){
			GUI_DrawBitmap(pButton->pBitmap, pButton->xPos, pButton->yPos);
		} else {
			GUI_DispStringAt("no sel pic",pButton->xPos,pButton->yPos);
		}
	}

	return (CUS_DEF_BUTTON_HANDLE)pButton;
}

/*create tempreture menu button*/
u32 CusCreateCircleButton(BUTTON_CIRCLE_CUS_Info * pBtn)
{
	const GUI_FONT * pFont; 
	s32 FontXSixe;
	s32 FontYSixe;
	s32 TempColor;

	/*outer circle*/
	GUI_SetColor(pBtn->EdgeColor);
	GUI_FillCircle(pBtn->OrgxPos, pBtn->OrgyPos, pBtn->OuterR);

	/*inner*/
	GUI_SetColor(pBtn->CtrColor);
	GUI_FillCircle(pBtn->OrgxPos, pBtn->OrgyPos, pBtn->InnerR);	

	/*Strings*/
	pFont = pBtn->Fontp;
	FontXSixe = pFont->XMag;
	FontYSixe = pFont->YMag;
	GUI_SetFont(pBtn->Fontp);
	GUI_SetColor(pBtn->FntColor);
	TempColor=GUI_GetBkColor();

	GUI_SetBkColor(pBtn->CtrColor);
	
	GUI_DispStringHCenterAt(pBtn->StrInBtn,pBtn->OrgxPos,pBtn->OrgyPos-CUS_TEMPRE_CIRCLE_WORK_STR_Y_OFFSET);
	GUI_SetBkColor(TempColor);

	return (CUS_DEF_CIRCLE_BUTTON_HANDLE)pBtn;
}

u32 CusCircleUpdateButtonSts(BUTTON_CIRCLE_CUS_Info * pBtn)
{
	if(pBtn->SelState == RT_CUS_BTN_UNSEL) {
		pBtn->CtrColor = CUS_TEMPRE_CIRCLE_TEST_CEN_DISCOLR;
	} else {
		pBtn->CtrColor = CUS_TEMPRE_CIRCLE_TEST_CEN_ENCOLR;
	}
	CusCreateCircleButton(pBtn);
}

/*update tempreture scale*/
void CusDrawTempreScale(TEMPRE_CUS_ICON_Info * pBtn)
{
	s32 LineOrg;
	s32 LineXmin;
	s32 LineXmax;
	s32 ShowStrPos;
	u32 i;
	s32 TempreStep;
	s32 TempreStepNum;
	s32 PixelStep;

	TempreStep = 5;  //
	TempreStepNum = (pBtn->TemprMax-pBtn->TemprMin)/TempreStep;
	PixelStep = pBtn->H_Dx/TempreStepNum;

	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(pBtn->Fontp);

	LineOrg = pBtn->OrgX+pBtn->Lin_Dx;
	LineXmin = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_S_Dx;
	LineXmax = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx;
	ShowStrPos = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx;
	 
	/*right label*/
	for(i=0; i<pBtn->H_Dx; i++){
		if((i%(PixelStep*2))==PixelStep) {
			GUI_DrawLine(LineOrg,pBtn->OrgY-i,LineXmin,pBtn->OrgY-i);
		}

		if((i%(PixelStep*2))==0) {
			GUI_DrawLine(LineOrg,pBtn->OrgY-i,LineXmax,pBtn->OrgY-i);
			if((TempreStep*(i/PixelStep))<10) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,1);
			}

			if(((TempreStep*(i/PixelStep))<100)&&((TempreStep*(i/PixelStep))>=10)) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,2);
			}

			if(((TempreStep*(i/PixelStep))<1000)&&((TempreStep*(i/PixelStep))>=100)) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,3);
			}
		}	
	}

	LineOrg = pBtn->OrgX-pBtn->Lin_Dx; //;
	LineXmin = pBtn->OrgX-pBtn->Lin_Dx-pBtn->Lin_S_Dx;
	LineXmax = pBtn->OrgX-pBtn->Lin_Dx-pBtn->Lin_L_Dx;
	ShowStrPos = pBtn->OrgX-pBtn->Lin_Dx-pBtn->Lin_L_Dx-pBtn->F_Dx-20;  //

	/*left label*/
	for(i=0; i<pBtn->H_Dx; i++){
		if((i%(PixelStep*2))==PixelStep) {
			GUI_DrawLine(LineXmin,pBtn->OrgY-i,LineOrg,pBtn->OrgY-i);
		}

		if((i%(PixelStep*2))==0) {
			GUI_DrawLine(LineXmax,pBtn->OrgY-i,LineOrg,pBtn->OrgY-i);
			if((TempreStep*(i/PixelStep))<10) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,1);
			}

			if(((TempreStep*(i/PixelStep))<100)&&((TempreStep*(i/PixelStep))>=10)) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,2);
			}

			if(((TempreStep*(i/PixelStep))<1000)&&((TempreStep*(i/PixelStep))>=100)) {
				GUI_DispDecAt((TempreStep*(i/PixelStep)),ShowStrPos,pBtn->OrgY-i,3);
			}
		}	
	}

	
}

CUS_RT_WIN_HANDLE CusCreateTempreIcon(TEMPRE_CUS_ICON_Info * pBtn)
{
	GUI_POINT aPoints[3];

	/*glass*/
	GUI_SetColorIndex(pBtn->G_Color/2);
	GUI_FillRect((pBtn->OrgX-pBtn->G_Dx),(pBtn->OrgY-pBtn->H_Dx),(pBtn->OrgX+pBtn->G_Dx),pBtn->OrgY);

	/*scale*/
	CusDrawTempreScale(pBtn);

	/*liquid*/
	GUI_SetColor(pBtn->L_Color);
	GUI_FillRect((pBtn->OrgX-pBtn->L_Dx),(pBtn->OrgY-pBtn->L_D_Dx),(pBtn->OrgX+pBtn->L_Dx),pBtn->OrgY+pBtn->E_Dx);	

	/*circle*/
	GUI_SetColor(pBtn->L_Color);
	GUI_FillCircle(pBtn->C_X,pBtn->C_Y,pBtn->C_R);

	/*scale*/
	GUI_SetColor(pBtn->S_La_Color);
	aPoints[1].x = 0;
	aPoints[1].y = 0;

	aPoints[0].x = 0;
	aPoints[0].y = pBtn->S_Len_Dx;

	aPoints[2].x = pBtn->S_L_Dx-pBtn->S_R_Dx;
	aPoints[2].y = pBtn->S_Len_Dx/2;

	GUI_FillPolygon(aPoints,countof(aPoints),pBtn->OrgX-pBtn->S_L_Dx,pBtn->OrgY-pBtn->S_D_Dx-pBtn->S_Len_Dx/2);
}

/*update setting scale*/
CUS_RT_WIN_HANDLE CusTempreIconUpdateSetScale(TEMPRE_CUS_ICON_Info * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	GUI_POINT aPoints[3];
	float TempreUnit;
	u32 CurPixel;


	TempMemXsize = pBtn->S_L_Dx-pBtn->S_R_Dx;
	TempMemYsize = pBtn->H_Dx+pBtn->S_Len_Dx;
	OrgX = pBtn->OrgX-pBtn->S_L_Dx;
	OrgY = pBtn->OrgY-pBtn->H_Dx-pBtn->S_Len_Dx/2;
	hMem = GUI_MEMDEV_CreateEx(OrgX,OrgY,TempMemXsize,TempMemYsize,0);
	GUI_MEMDEV_Select(hMem);
	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);

	TempreUnit = ((float)pBtn->H_Dx)/(((float)pBtn->TemprMax)-((float)pBtn->TemprMin));
	CurPixel = (u32)(((float)pBtn->CurSetTempre)*TempreUnit);

	pBtn->S_D_Dx = CurPixel;

	/*scale*/
	GUI_SetColor(pBtn->S_La_Color);
	aPoints[1].x = 0;
	aPoints[1].y = 0;

	aPoints[0].x = 0;
	aPoints[0].y = pBtn->S_Len_Dx;

	aPoints[2].x = pBtn->S_L_Dx-pBtn->S_R_Dx;
	aPoints[2].y = pBtn->S_Len_Dx/2;

	GUI_FillPolygon(aPoints,countof(aPoints),pBtn->OrgX-pBtn->S_L_Dx,pBtn->OrgY-pBtn->S_D_Dx-pBtn->S_Len_Dx/2);
	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);
}

CUS_RT_WIN_HANDLE CusTempreIconUpdateSetScaleDx(TEMPRE_CUS_ICON_Info * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	GUI_POINT aPoints[3];
	float TempreUnit;

	TempMemXsize = pBtn->S_L_Dx-pBtn->S_R_Dx;
	TempMemYsize = pBtn->H_Dx+pBtn->S_Len_Dx;
	OrgX = pBtn->OrgX-pBtn->S_L_Dx;
	OrgY = pBtn->OrgY-pBtn->H_Dx-pBtn->S_Len_Dx/2;
	hMem = GUI_MEMDEV_Create(OrgX,OrgY,TempMemXsize,TempMemYsize);
	GUI_MEMDEV_Select(hMem);
	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);

	TempreUnit = (((float)pBtn->TemprMax)-((float)pBtn->TemprMin))/((float)pBtn->H_Dx);
	pBtn->CurSetTempre = (u32)(((float)pBtn->S_D_Dx)*TempreUnit);

	/*scale*/
	GUI_SetColor(pBtn->S_La_Color);
	aPoints[1].x = 0;
	aPoints[1].y = 0;

	aPoints[0].x = 0;
	aPoints[0].y = pBtn->S_Len_Dx;

	aPoints[2].x = pBtn->S_L_Dx-pBtn->S_R_Dx;
	aPoints[2].y = pBtn->S_Len_Dx/2;

	GUI_FillPolygon(aPoints,countof(aPoints),pBtn->OrgX-pBtn->S_L_Dx,pBtn->OrgY-pBtn->S_D_Dx-pBtn->S_Len_Dx/2);
	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);
}


CUS_RT_WIN_HANDLE CusTempreIconUpdateScale(TEMPRE_CUS_ICON_Info * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	GUI_MEMDEV_Handle hMemTemp;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	GUI_POINT aPoints[3];
	float TempreUnit;
	u32 CurPixel;

	TempMemXsize = pBtn->L_Dx*2+1;
	TempMemYsize = pBtn->H_Dx;
	OrgX = pBtn->OrgX-pBtn->L_Dx;
	OrgY = pBtn->OrgY-pBtn->H_Dx;
	hMem = GUI_MEMDEV_Create(OrgX,OrgY,TempMemXsize,TempMemYsize);
	GUI_MEMDEV_Select(hMem);

	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);

	TempreUnit = ((float)pBtn->H_Dx)/(((float)pBtn->TemprMax)-((float)pBtn->TemprMin));
	CurPixel = (u32)(((float)pBtn->CurTempre)*TempreUnit);

	pBtn->L_D_Dx = CurPixel;
	
	GUI_SetColor(pBtn->L_Color);
	GUI_FillRect((pBtn->OrgX-pBtn->L_Dx),(pBtn->OrgY-pBtn->L_D_Dx),(pBtn->OrgX+pBtn->L_Dx),pBtn->OrgY);

	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);
}

CUS_RT_WIN_HANDLE CusTempreIconShowSetNum(TEMPRE_CUS_ICON_Info * pBtn)
{
	s32 Temp,Tempy,TempS;
	s32 ShowX;
	s32 ShowY;
	s32 TempU;

	Temp = 97;
	Tempy = 0;
	TempS = 30;
	TempU = 19;

	GUI_SetFont(&GUI_Font13B_ASCII);
	GUI_SetColor(GUI_RED);
	GUI_DispStringAt("SETTING:",pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+TempS,pBtn->OrgY-pBtn->H_Dx+Tempy);
	if(pBtn->CurSetTempre<10) {
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurSetTempre,ShowX,ShowY,1);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}

	if((pBtn->CurSetTempre<100)&&((pBtn->CurSetTempre>=10))) {
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurSetTempre,ShowX,ShowY,2);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}
	if((pBtn->CurSetTempre>=100)) {\
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurSetTempre,ShowX,ShowY,3);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}
}

CUS_RT_WIN_HANDLE CusTempreIconShowCurNum(TEMPRE_CUS_ICON_Info * pBtn)
{
	s32 Temp,Tempy,TempS;
	s32 ShowX;
	s32 ShowY;
	s32 TempU;

	Temp = 97;
	Tempy = 30;
	TempS = 30;
	TempU = 19;

	GUI_SetFont(&GUI_Font13B_ASCII);
	GUI_SetColor(GUI_BLUE);
	GUI_DispStringAt("REAL-TIME:",pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+TempS,pBtn->OrgY-pBtn->H_Dx+Tempy);
	if(pBtn->CurTempre<10) {
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurTempre,ShowX,ShowY,1);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}

	if((pBtn->CurTempre<100)&&((pBtn->CurTempre>=10))) {
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurTempre,ShowX,ShowY,2);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}
	if((pBtn->CurTempre>=100)) {
		ShowX = pBtn->OrgX+pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx+Temp;
		ShowY = pBtn->OrgY-pBtn->H_Dx+Tempy;
		GUI_ClearRect(ShowX,ShowY,ShowX+20,ShowY+15);
		GUI_DispDecAt(pBtn->CurTempre,ShowX,ShowY,3);
		GUI_DispStringAt("CEL",ShowX+TempU,ShowY);
	}
}

CUS_RT_WIN_HANDLE CusTempreIconShowWarning(TEMPRE_CUS_ICON_Info * pBtn)
{
	s32 ShowX0;
	s32 ShowY0;
	s32 ShowX1;
	s32 ShowY1;

	ShowX0 = pBtn->OrgX + pBtn->Lin_Dx+pBtn->Lin_L_Dx+pBtn->F_Dx + 30;
	ShowY0 = pBtn->OrgY - pBtn->H_Dx/2;
	ShowX1 = ShowX0+100;
	ShowY1 = ShowY0+60;

	if(pBtn->WarningSts == CUS_TEMPRE_WARNING){
		GUI_SetFont(&GUI_FontComic18B_1);
		GUI_SetColor(GUI_RED);
		GUI_ClearRect(ShowX0,ShowY0,ShowX1,ShowY1);
		GUI_DispStringAt("WARNING !",ShowX0,ShowY0);
	} else {
		GUI_ClearRect(ShowX0,ShowY0,ShowX1,ShowY1);
	}
}


CUS_RT_WIN_HANDLE CusGradMenuShowPlate(CUS_GRAD_PLATE_Info * pBtn)
{
	/*bk color*/
	GUI_SetBkColor(pBtn->BkColor);
	GUI_ClearRect(pBtn->RangeXmin,pBtn->RangeYmin,pBtn->RangeXmax,pBtn->RangeYmax);

	/*circle 0*/
	GUI_SetColor(pBtn->Color0);
	GUI_FillCircle(pBtn->X0,pBtn->Y0,pBtn->R0);
	
	/*circle 1*/
	GUI_SetColor(pBtn->Color1);
	GUI_FillCircle(pBtn->X1,pBtn->Y1,pBtn->R1);
}

CUS_RT_WIN_HANDLE CusGradMenuShowPlateUpdate(CUS_GRAD_PLATE_Info * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;

	TempMemXsize = pBtn->RangeXmax-pBtn->RangeXmin;
	TempMemYsize = pBtn->RangeYmax-pBtn->RangeYmin;
	OrgX = pBtn->RangeXmin;
	OrgY = pBtn->RangeYmin;

	hMem = GUI_MEMDEV_CreateEx(OrgX,OrgY,TempMemXsize,TempMemYsize,0);
	GUI_MEMDEV_Select(hMem);
	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);
	/*bk color*/
	GUI_SetBkColor(pBtn->BkColor);
	GUI_ClearRect(pBtn->RangeXmin,pBtn->RangeYmin,pBtn->RangeXmax,pBtn->RangeYmax);

	if((pBtn->X0==pBtn->X1)&&(pBtn->Y0==pBtn->Y1))
	{
		GUI_SetColor(pBtn->CombineClr);
		GUI_FillCircle(pBtn->X0,pBtn->Y0,pBtn->R0);
		GUI_FillCircle(pBtn->X1,pBtn->Y1,pBtn->R1);
	}
	
	if((pBtn->X0!=pBtn->X1)||(pBtn->X1!=pBtn->Y1))
	{
		/*circle 0*/
		GUI_SetColor(pBtn->Color0);
		GUI_FillCircle(pBtn->X0,pBtn->Y0,pBtn->R0);
		
		/*circle 1*/
		GUI_SetColor(pBtn->Color1);
		GUI_FillCircle(pBtn->X1,pBtn->Y1,pBtn->R1);
	}

	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);
	
}

CUS_RT_WIN_HANDLE CusCreateLightPlate(LIGHT_PLATE_CUS_Info * pBtn)
{
	u32 Color;
	u32 i,j;

	GUI_SetBkColor(GUI_WHITE);
	GUI_Clear();
	for(i=0; i<272; i++)
	{
		for(j=0; j<480; j++){
			//Color=0;
			//Color=((i<<3)|((j<<2)<<8)|((j+i)<<16));
			GUI_SetColor(i*j);
			GUI_DrawPoint(i,j);
		}
	}
}

/*create bar*/
u32 CusCreateColorBar(CUS_COLOR_BAR_DEF_INFO * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	s32 Ytemp = 0;
	s32 TxtTemp=10;

	s32 LcdX = LCD_GET_XSIZE();
	s32 LcdY =  LCD_GET_YSIZE();

	TempMemXsize = LcdX-1-(pBtn->BarOrgX-pBtn->BtnR);
	TempMemYsize = (pBtn->BarOrgY+pBtn->BarYSize+pBtn->BtnR)-(pBtn->BarOrgY-pBtn->BtnR);
	OrgX = pBtn->BarOrgX-pBtn->BtnR;
	OrgY = pBtn->BarOrgY-pBtn->BtnR;
	hMem = GUI_MEMDEV_Create(OrgX,OrgY,TempMemXsize,TempMemYsize);
	GUI_MEMDEV_Select(hMem);

	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);

	/*clear disp area*/
	GUI_ClearRect(pBtn->BarOrgX-pBtn->BtnR, pBtn->BarOrgY-pBtn->BtnR, LcdX-1, pBtn->BarOrgY+pBtn->BarYSize+pBtn->BtnR);

	/*draw bar*/
	GUI_SetColor(pBtn->BarFillColor);
	GUI_FillRect(pBtn->BarOrgX, pBtn->BarOrgY, pBtn->BarOrgX+pBtn->BarXSize, pBtn->BarOrgY+pBtn->BarYSize);

	/*button*/
	GUI_SetColor(pBtn->BtnFillColor);
	GUI_FillCircle(pBtn->BarOrgX+pBtn->BtnRange, pBtn->BarOrgY+(pBtn->BarYSize)/2, pBtn->BtnR);	
	GUI_SetColor(GUI_WHITE);
	GUI_DrawCircle(pBtn->BarOrgX+pBtn->BtnRange, pBtn->BarOrgY+(pBtn->BarYSize)/2, pBtn->BtnR);	

	/*show strings*/
	GUI_SetFont(pBtn->pFnt);
	if(pBtn->BarFillColor==GUI_RED) {
		GUI_DispStringAt("R", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}

	if(pBtn->BarFillColor==GUI_GREEN) {
		GUI_DispStringAt("G", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}
	if(pBtn->BarFillColor==GUI_BLUE) {
		GUI_DispStringAt("B", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}

	if(pBtn->CurColorVal<10) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 1);
	} else if((pBtn->CurColorVal>=10)&&(pBtn->CurColorVal<100)) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 2);		
	} else if((pBtn->CurColorVal>=100)&&(pBtn->CurColorVal<1000)) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 3);		
	} 
	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);
}

/*create bar Dx*/
u32 CusUpdateColorBarDx(CUS_COLOR_BAR_DEF_INFO * pBtn)
{
	GUI_MEMDEV_Handle hMem;
	s32 TempMemXsize;
	s32 TempMemYsize;
	s32 OrgX;
	s32 OrgY;
	s32 Ytemp = 0;
	s32 TxtTemp=10;

	s32 LcdX = LCD_GET_XSIZE();
	s32 LcdY =  LCD_GET_YSIZE();

	TempMemXsize = LcdX-1-(pBtn->BarOrgX-pBtn->BtnR);
	TempMemYsize = (pBtn->BarOrgY+pBtn->BarYSize+pBtn->BtnR)-(pBtn->BarOrgY-pBtn->BtnR);
	OrgX = pBtn->BarOrgX-pBtn->BtnR;
	OrgY = pBtn->BarOrgY-pBtn->BtnR;
	hMem = GUI_MEMDEV_Create(OrgX,OrgY,TempMemXsize,TempMemYsize);
	GUI_MEMDEV_Select(hMem);

	GUI_ClearRect(OrgX,OrgY,OrgX+TempMemXsize,OrgY+TempMemYsize);

	/*draw bar*/
	GUI_SetColor(pBtn->BarFillColor);
	GUI_FillRect(pBtn->BarOrgX, pBtn->BarOrgY, pBtn->BarOrgX+pBtn->BarXSize, pBtn->BarOrgY+pBtn->BarYSize);

	/*button*/
	GUI_SetColor(pBtn->BtnFillColor);
	GUI_FillCircle(pBtn->BarOrgX+pBtn->BtnRange, pBtn->BarOrgY+(pBtn->BarYSize)/2, pBtn->BtnR);	
	GUI_SetColor(GUI_WHITE);
	GUI_DrawCircle(pBtn->BarOrgX+pBtn->BtnRange, pBtn->BarOrgY+(pBtn->BarYSize)/2, pBtn->BtnR);	

	/*show strings*/
	GUI_SetFont(pBtn->pFnt);
	if(pBtn->BarFillColor==GUI_RED) {
		GUI_DispStringAt("R", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}

	if(pBtn->BarFillColor==GUI_GREEN) {
		GUI_DispStringAt("G", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}
	if(pBtn->BarFillColor==GUI_BLUE) {
		GUI_DispStringAt("B", pBtn->CurShowStrX, pBtn->CurShowStrY+Ytemp);
	}

	pBtn->CurColorVal = ((pBtn->BtnRange)*(pBtn->ColorRange))/pBtn->BarXSize;
	
	if(pBtn->CurColorVal<10) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 1);
	} else if((pBtn->CurColorVal>=10)&&(pBtn->CurColorVal<100)) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 2);		
	} else if((pBtn->CurColorVal>=100)&&(pBtn->CurColorVal<1000)) {
		GUI_DispDecAt(pBtn->CurColorVal, pBtn->CurShowStrX+TxtTemp, pBtn->CurShowStrY+Ytemp, 3);		
	} 
	GUI_MEMDEV_CopyToLCD(hMem);
	GUI_SelectLCD();
	GUI_MEMDEV_Delete(hMem);	
}

 /*create light icon*/
u32 CusCreateLightBulbDev(CUS_Light_BULB_DISP_DEF_INFO * pBtn)
{
	s32 Ytemp = 0;
	s32 TxtTemp=10;

	s32 LcdX = LCD_GET_XSIZE();
	s32 LcdY =  LCD_GET_YSIZE();	

	if(pBtn->SelState == RT_CUS_BTN_SEL) {
		GUI_SetColor(pBtn->BulbCurFillColor);
		GUI_FillEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		GUI_SetColor(pBtn->BulbOutColor);
		GUI_DrawEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		/*left*/
		GUI_SetColor(pBtn->BulbCurFillColor);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->OrgY-pBtn->R_D_Y-8,pBtn->OrgX+16,pBtn->OrgY-pBtn->R_D_Y-20);
		GUI_DrawLine(pBtn->OrgX+5, pBtn->Ey,pBtn->OrgX+16,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->Ey-pBtn->R_D_Y-40,pBtn->OrgX+16,pBtn->Ey-pBtn->R_D_Y/2-30);

		/*right*/
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->OrgY-pBtn->R_D_Y-20,pBtn->OrgX+10+2*pBtn->Edx+30,pBtn->OrgY-pBtn->R_D_Y-8);
		GUI_DrawLine(pBtn->OrgX+16+ 2*pBtn->Edx+10, pBtn->Ey,pBtn->OrgX+2*pBtn->Edx+35,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);

		GUI_DrawLine(pBtn->Ex, pBtn->Ey-pBtn->Edy-10,pBtn->Ex,pBtn->Ey-pBtn->Edy-20);
	} else {
		GUI_SetColor(GUI_GetBkColor());
		GUI_FillEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		GUI_SetColor(pBtn->BulbOutColor);
		GUI_DrawEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);

		GUI_SetColor(GUI_GetBkColor());
		/*left*/
		GUI_DrawLine(pBtn->OrgX+10, pBtn->OrgY-pBtn->R_D_Y-8,pBtn->OrgX+16,pBtn->OrgY-pBtn->R_D_Y-20);
		GUI_DrawLine(pBtn->OrgX+5, pBtn->Ey,pBtn->OrgX+16,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->Ey-pBtn->R_D_Y-40,pBtn->OrgX+16,pBtn->Ey-pBtn->R_D_Y/2-30);

		/*right*/
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->OrgY-pBtn->R_D_Y-20,pBtn->OrgX+10+2*pBtn->Edx+30,pBtn->OrgY-pBtn->R_D_Y-8);
		GUI_DrawLine(pBtn->OrgX+16+ 2*pBtn->Edx+10, pBtn->Ey,pBtn->OrgX+2*pBtn->Edx+35,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);

		GUI_DrawLine(pBtn->Ex, pBtn->Ey-pBtn->Edy-10,pBtn->Ex,pBtn->Ey-pBtn->Edy-20);		
	}

	
	/*base*/
	GUI_SetColor(pBtn->R_Color);
	GUI_FillRect(pBtn->OrgX,pBtn->OrgY-pBtn->R_D_Y,pBtn->OrgX+pBtn->R_D_X, pBtn->OrgY);
	GUI_SetColor(pBtn->BulbOutColor);
	GUI_DrawRect(pBtn->OrgX,pBtn->OrgY-pBtn->R_D_Y,pBtn->OrgX+pBtn->R_D_X, pBtn->OrgY);
}

/*update bulb dev*/
u32 CusUpdateLightBulbDev(CUS_Light_BULB_DISP_DEF_INFO * pBtn)
{
	s32 Ytemp = 0;
	s32 TxtTemp=10;

	s32 LcdX = LCD_GET_XSIZE();
	s32 LcdY =  LCD_GET_YSIZE();	

	/*draw bulb*/
	if(pBtn->SelState == RT_CUS_BTN_SEL) {
		GUI_SetColor(pBtn->BulbCurFillColor);
		GUI_FillEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		GUI_SetColor(pBtn->BulbOutColor);
		GUI_DrawEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		/*left*/
		GUI_SetColor(pBtn->BulbCurFillColor);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->OrgY-pBtn->R_D_Y-8,pBtn->OrgX+16,pBtn->OrgY-pBtn->R_D_Y-20);
		GUI_DrawLine(pBtn->OrgX+5, pBtn->Ey,pBtn->OrgX+16,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->Ey-pBtn->R_D_Y-40,pBtn->OrgX+16,pBtn->Ey-pBtn->R_D_Y/2-30);

		/*right*/
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->OrgY-pBtn->R_D_Y-20,pBtn->OrgX+10+2*pBtn->Edx+30,pBtn->OrgY-pBtn->R_D_Y-8);
		GUI_DrawLine(pBtn->OrgX+16+ 2*pBtn->Edx+10, pBtn->Ey,pBtn->OrgX+2*pBtn->Edx+35,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);

		GUI_DrawLine(pBtn->Ex, pBtn->Ey-pBtn->Edy-10,pBtn->Ex,pBtn->Ey-pBtn->Edy-20);
	} else {
		GUI_SetColor(GUI_GetBkColor());
		GUI_FillEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);
		GUI_SetColor(pBtn->BulbOutColor);
		GUI_DrawEllipse(pBtn->Ex,pBtn->Ey,pBtn->Edx,pBtn->Edy);

		GUI_SetColor(GUI_GetBkColor());
		/*left*/
		GUI_DrawLine(pBtn->OrgX+10, pBtn->OrgY-pBtn->R_D_Y-8,pBtn->OrgX+16,pBtn->OrgY-pBtn->R_D_Y-20);
		GUI_DrawLine(pBtn->OrgX+5, pBtn->Ey,pBtn->OrgX+16,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+10, pBtn->Ey-pBtn->R_D_Y-40,pBtn->OrgX+16,pBtn->Ey-pBtn->R_D_Y/2-30);

		/*right*/
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->OrgY-pBtn->R_D_Y-20,pBtn->OrgX+10+2*pBtn->Edx+30,pBtn->OrgY-pBtn->R_D_Y-8);
		GUI_DrawLine(pBtn->OrgX+16+ 2*pBtn->Edx+10, pBtn->Ey,pBtn->OrgX+2*pBtn->Edx+35,pBtn->Ey);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);
		GUI_DrawLine(pBtn->OrgX+16+2*pBtn->Edx+10, pBtn->Ey-pBtn->R_D_Y/2-30,pBtn->OrgX+10+2*pBtn->Edx+20,pBtn->Ey-pBtn->R_D_Y-40);

		GUI_DrawLine(pBtn->Ex, pBtn->Ey-pBtn->Edy-10,pBtn->Ex,pBtn->Ey-pBtn->Edy-20);		
	}

}


