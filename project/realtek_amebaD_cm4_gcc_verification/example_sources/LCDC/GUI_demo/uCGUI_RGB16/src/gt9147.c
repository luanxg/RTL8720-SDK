#include "ameba_soc.h"
#include "platform_stdlib.h"
#include "basic_types.h"
#include "diag.h"
#include "rand.h"
#include "section_config.h"
#include "ameba_soc.h"
//#include "rtl8721d_lcdc_test.h"
#include "rtl8721d_lcdc.h"
#include "bsp_rgb_lcd.h"
#include "gt9147.h"
#include "touch.h"
#include "ctiic.h"


_lcd_dev_TP TP_LCD_Dev;

#define lcddev 	TP_LCD_Dev

/*configuration table*/
const u8 GT9147_CFG_TBL[]=
{ 
	0X60,0XE0,0X01,0X20,0X03,0X05,0X35,0X00,0X02,0X08,
	0X1E,0X08,0X50,0X3C,0X0F,0X05,0X00,0X00,0XFF,0X67,
	0X50,0X00,0X00,0X18,0X1A,0X1E,0X14,0X89,0X28,0X0A,
	0X30,0X2E,0XBB,0X0A,0X03,0X00,0X00,0X02,0X33,0X1D,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X32,0X00,0X00,
	0X2A,0X1C,0X5A,0X94,0XC5,0X02,0X07,0X00,0X00,0X00,
	0XB5,0X1F,0X00,0X90,0X28,0X00,0X77,0X32,0X00,0X62,
	0X3F,0X00,0X52,0X50,0X00,0X52,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X0F,
	0X0F,0X03,0X06,0X10,0X42,0XF8,0X0F,0X14,0X00,0X00,
	0X00,0X00,0X1A,0X18,0X16,0X14,0X12,0X10,0X0E,0X0C,
	0X0A,0X08,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X29,0X28,0X24,0X22,0X20,0X1F,0X1E,0X1D,
	0X0E,0X0C,0X0A,0X08,0X06,0X05,0X04,0X02,0X00,0XFF,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
	0XFF,0XFF,0XFF,0XFF,
};  

u8 GT9147_Send_Cfg(u8 mode)
{
	u8 buf[2];
	u8 i=0;
	buf[0]=0;
	buf[1]=mode;	
	for(i=0;i<sizeof(GT9147_CFG_TBL);i++)buf[0]+=GT9147_CFG_TBL[i];
    buf[0]=(~buf[0])+1;
	GT9147_WR_Reg(GT_CFGS_REG,(u8*)GT9147_CFG_TBL,sizeof(GT9147_CFG_TBL));
	GT9147_WR_Reg(GT_CHECK_REG,buf,2);
	return 0;
} 

u8 GT9147_WR_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i;
	u8 ret=0;
	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);
	CT_IIC_Wait_Ack();
	CT_IIC_Send_Byte(reg>>8);   
	CT_IIC_Wait_Ack(); 	 										  		   
	CT_IIC_Send_Byte(reg&0XFF);   
	CT_IIC_Wait_Ack();  
	for(i=0;i<len;i++)
	{	   
    	CT_IIC_Send_Byte(buf[i]);  	
		ret=CT_IIC_Wait_Ack();
		if(ret)break;  
	}
    CT_IIC_Stop();			
	return ret; 
}
	  
void GT9147_RD_Reg(u16 reg,u8 *buf,u8 len)
{
	u8 i; 
 	CT_IIC_Start();	
 	CT_IIC_Send_Byte(GT_CMD_WR);  
	CT_IIC_Wait_Ack();
 	CT_IIC_Send_Byte(reg>>8);   
	CT_IIC_Wait_Ack(); 	 										  		   
 	CT_IIC_Send_Byte(reg&0XFF);   
	CT_IIC_Wait_Ack();  
 	CT_IIC_Start();  	 	   
	CT_IIC_Send_Byte(GT_CMD_RD);	   
	CT_IIC_Wait_Ack();	   
	for(i=0;i<len;i++)
	{	   
    	buf[i]=CT_IIC_Read_Byte(i==(len-1)?0:1);
	} 
    CT_IIC_Stop();
} 

void GT9147_RST_OUT(level)
{
	GPIO_WriteBit(GT_RST_PIN, level);
}

void GT9147_INT_OUT(level)
{
	GPIO_WriteBit(GT_INT_PIN, level);
}

void CT_INT_IN_Func(void)
{
	/*initialize the GPIO*/
	u32 i = 0;
	GPIO_InitTypeDef GPIO_InitStruct_Temp;

	GPIO_InitStruct_Temp.GPIO_Pin = GT_INT_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(&GPIO_InitStruct_Temp);	
}

void CT_INT_OUT_Func(void)
{
	/*initialize the GPIO*/
	u32 i = 0;
	GPIO_InitTypeDef GPIO_InitStruct_Temp;

	GPIO_InitStruct_Temp.GPIO_Pin = GT_INT_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(&GPIO_InitStruct_Temp);	
	PAD_PullCtrl(GT_INT_PIN, GPIO_PuPd_UP);
}

/*9147 initialization*/
u8 GT9147_Init(void)
{
	u8 temp[5]; 
	lcddev.id = 0X4342;
	lcddev.width = 480;
	lcddev.height = 272;
	lcddev.dir = 1;

	tp_dev.touchtype|=0X80;
	tp_dev.touchtype|=lcddev.dir&0X01;
	
	CT_GPIO_Init();
	CT_IIC_Init();
	GT_RST(0);
	CT_INT_OUT_Func();
	GT9147_INT_OUT(0);
	delay_ms(10);
	GT9147_INT_OUT(1);
	delay_ms(10);
 	GT_RST(1);	    
	delay_ms(100); 
	CT_INT_IN_Func();
	delay_ms(500);
	GT9147_RD_Reg(GT_PID_REG,temp,4);
	temp[4]=0;
	printf("CTP ID:%s\r\n",temp);
	if(strcmp((char*)temp,"9147")==0)
	{
		temp[0]=0X02;			
		GT9147_WR_Reg(GT_CTRL_REG,temp,1);
		GT9147_RD_Reg(GT_CFGS_REG,temp,1);
		if(temp[0]<0X60)
		{
			printf("Default Ver:%d\r\n",temp[0]);
            if(lcddev.id==0X5510)GT9147_Send_Cfg(1);
		}
		delay_ms(10);
		temp[0]=0X00;	 
		GT9147_WR_Reg(GT_CTRL_REG,temp,1);
		return 0;
	} 
	return 1;
}
const u16 GT9147_TPX_TBL[5]={GT_TP1_REG,GT_TP2_REG,GT_TP3_REG,GT_TP4_REG,GT_TP5_REG};
u8 GT9147_Scan(u8 mode)
{
	u8 buf[4];
	u8 i=0;
	u8 res=0;
	u8 temp;
	u8 tempsta;
 	static u8 t=0;
	t++;
	if((t%10)==0||t<10)
	{
		GT9147_RD_Reg(GT_GSTID_REG,&mode,1);
 		if(mode&0X80&&((mode&0XF)<6))
		{
			temp=0;
			GT9147_WR_Reg(GT_GSTID_REG,&temp,1);		
		}		
		if((mode&0XF)&&((mode&0XF)<6))
		{
			temp=0XFF<<(mode&0XF);
			tempsta=tp_dev.sta;
			tp_dev.sta=(~temp)|TP_PRES_DOWN|TP_CATH_PRES; 
			tp_dev.x[4]=tp_dev.x[0];
			tp_dev.y[4]=tp_dev.y[0];
			for(i=0;i<5;i++)
			{
				if(tp_dev.sta&(1<<i))	
				{
					GT9147_RD_Reg(GT9147_TPX_TBL[i],buf,4);	
                    if(lcddev.id==0X5510)
                    {
                        if(tp_dev.touchtype&0X01)
                        {
                            tp_dev.y[i]=((u16)buf[1]<<8)+buf[0];
                            tp_dev.x[i]=800-(((u16)buf[3]<<8)+buf[2]);
                        }else
                        {
                            tp_dev.x[i]=((u16)buf[1]<<8)+buf[0];
                            tp_dev.y[i]=((u16)buf[3]<<8)+buf[2];
                        }  
                    }else if(lcddev.id==0X4342)
                    {
                        if(tp_dev.touchtype&0X01)
                        {
                            tp_dev.x[i]=(((u16)buf[1]<<8)+buf[0]);
                            tp_dev.y[i]=(((u16)buf[3]<<8)+buf[2]);
                        }else
                        {
                            tp_dev.y[i]=((u16)buf[1]<<8)+buf[0];
                            tp_dev.x[i]=272-(((u16)buf[3]<<8)+buf[2]);
                        }
                    }
				}			
			}  
			res=1;
			if(tp_dev.x[0]>lcddev.width||tp_dev.y[0]>lcddev.height)
			{ 
				if((mode&0XF)>1)
				{
					tp_dev.x[0]=tp_dev.x[1];
					tp_dev.y[0]=tp_dev.y[1];
					t=0;				
				}else					
				{
					tp_dev.x[0]=tp_dev.x[4];
					tp_dev.y[0]=tp_dev.y[4];
					mode=0X80;		
					tp_dev.sta=tempsta;	
				}
			}else t=0;					
		}
	}
	if((mode&0X8F)==0X80)
	{ 
		if(tp_dev.sta&TP_PRES_DOWN)
		{
			tp_dev.sta&=~(1<<7);	
		}else						
		{ 
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
			tp_dev.sta&=0XE0;	
		}	 
	} 	
	if(t>240)t=10;
	return res;
}
 

