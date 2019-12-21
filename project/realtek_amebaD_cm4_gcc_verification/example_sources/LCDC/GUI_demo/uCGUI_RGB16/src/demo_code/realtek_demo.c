#include "GUI.h"
#include "rt_gui_demo_includes.h"

RT_CUS_DEMO_STS_TYPE  RtCusDemoVar;

unsigned int RtCusLanguage;

/*demo entry */
void RealtekDemo(void) 
{
	RT_CUS_DEMO_STS_TYPE * pState;
	GUI_SetBkColor(GUI_WHITE);GUI_Clear();
	GUI_SetColor(GUI_RED);
	RtCusLanguage = RT_CUS_CHINESE;
	pState = &RtCusDemoVar;
	pState->TransSts = RT_CUS_STARTMENU_PRO;
	while(1)
	{
		/*start menu*/
		switch(pState->TransSts) {
			
		case RT_CUS_STARTMENU_PRO:
			StartMenuFunc(pState);
			break;
		break;

		/*main menu*/
		case RT_CUS_MAINMENU_PRO:
			MainMenuCusPro(pState);
		break;

		/*clock*/
		case RT_CUS_CLOCK_PRO:
			ClockMenuCusPro(pState);
		break;

		/*light*/
		case RT_CUS_LIGHTCTRL_PRO:
			LightMenuCusPro(pState);
		break;

		/*tempreture*/
		case RT_CUS_TEMPRE_PRO:
			TempreMenuCusPro(pState);
		break;

		/*grad...*/
		case RT_CUS_GRAD_PRO:
			GradMenuCusPro(pState);
		break;

		/*data aquisition*/
		case RT_CUS_WAVE_PRO:
			WaveMenuCusPro(pState);
		break;

		deafult:

		break;
		}
		GUI_Delay(3);
	}
}
	 	 			 		    	 				 	  			   	 	 	 	 	 	  	  	      	   		 	 	 		  		  	 		 	  	  			     			       	   	 			  		    	 	     	 				  	 					 	 			   	  	  			 				 		 	 	 			     			 
