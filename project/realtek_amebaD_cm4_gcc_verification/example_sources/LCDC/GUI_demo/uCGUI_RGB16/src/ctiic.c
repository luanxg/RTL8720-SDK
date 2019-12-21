#include "ctiic.h"


void CT_SDA_IN_Func(void)
{
	/*initialize the GPIO*/
	u32 i = 0;
	GPIO_InitTypeDef GPIO_InitStruct_Temp;

	GPIO_InitStruct_Temp.GPIO_Pin = GT9147_I2C_SDA_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(&GPIO_InitStruct_Temp);	
}

void CT_SDA_OUT_Func(void)
{
	/*initialize the GPIO*/
	u32 i = 0;
	GPIO_InitTypeDef GPIO_InitStruct_Temp;

	GPIO_InitStruct_Temp.GPIO_Pin = GT9147_I2C_SDA_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(&GPIO_InitStruct_Temp);	
	PAD_PullCtrl(GT9147_I2C_SDA_PIN, GPIO_PuPd_UP);
}

void CT_IIC_SCL_PIN(int level)
{
	GPIO_WriteBit(GT9147_I2C_SCL_PIN, level);
}

void CT_IIC_SDA_PIN(int level)
{
	GPIO_WriteBit(GT9147_I2C_SDA_PIN, level);
}

void CT_Delay(void)
{
	delay_us(2);
} 

void CT_IIC_Init(void)
{					     
	/*initialize the GPIO*/
	u32 i = 0;
	GPIO_InitTypeDef GPIO_InitStruct_Temp;

	GPIO_InitStruct_Temp.GPIO_Pin = GT9147_I2C_SCL_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_OUT;
	PAD_PullCtrl(GT9147_I2C_SCL_PIN, GPIO_PuPd_UP);
	GPIO_Init(&GPIO_InitStruct_Temp);
	PAD_PullCtrl(GT9147_I2C_SCL_PIN, GPIO_PuPd_UP);


	GPIO_InitStruct_Temp.GPIO_Pin = GT9147_I2C_SDA_PIN;
	GPIO_InitStruct_Temp.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct_Temp.GPIO_Mode = GPIO_Mode_OUT;
	PAD_PullCtrl(GT9147_I2C_SDA_PIN, GPIO_PuPd_UP);
	GPIO_Init(&GPIO_InitStruct_Temp);
	PAD_PullCtrl(GT9147_I2C_SDA_PIN, GPIO_PuPd_UP);
}

void CT_IIC_Start(void)
{
	CT_SDA_OUT(); 
	CT_IIC_SDA(1);	  	  
	CT_IIC_SCL(1);
	delay_us(30);
 	CT_IIC_SDA(0);
	CT_Delay();
	CT_IIC_SCL(0);
}	  
void CT_IIC_Stop(void)
{
	CT_SDA_OUT();
	CT_IIC_SCL(1);
	delay_us(30);
	CT_IIC_SDA(0);
	CT_Delay();
	CT_IIC_SDA(1);
}

u8 CT_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	CT_SDA_IN();
	CT_IIC_SDA(1);	   
	CT_IIC_SCL(1);
	CT_Delay();

	while(CT_READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			CT_IIC_Stop();
			return 1;
		} 
		CT_Delay();
	}
	CT_IIC_SCL(0);// ±÷” ‰≥ˆ0 	   
	return 0;  
} 

void CT_IIC_Ack(void)
{
	CT_IIC_SCL(0);
	CT_SDA_OUT();
	CT_Delay();
	CT_IIC_SDA(0);
	CT_Delay();
	CT_IIC_SCL(1);
	CT_Delay();
	CT_IIC_SCL(0);
}

void CT_IIC_NAck(void)
{
	CT_IIC_SCL(0);
	CT_SDA_OUT();
	CT_Delay();
	CT_IIC_SDA(1);
	CT_Delay();
	CT_IIC_SCL(1);
	CT_Delay();
	CT_IIC_SCL(0);
}					 				     
		  
void CT_IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	CT_SDA_OUT(); 	    
    CT_IIC_SCL(0);
	CT_Delay();
	for(t=0;t<8;t++)
    {              
        CT_IIC_SDA((txd&0x80)>>7);
        txd<<=1; 	
		CT_Delay(); 
		CT_IIC_SCL(1); 
		CT_Delay();
		CT_Delay();
		CT_IIC_SCL(0);
		CT_Delay();
    }	 
} 	    

u8 CT_IIC_Read_Byte(unsigned char ack)
{
	u8 i,receive=0;
 	CT_SDA_IN();
	delay_us(100);
	for(i=0;i<8;i++ )
	{ 
		CT_Delay();
		CT_IIC_SCL(0); 	    	   
		CT_Delay();
		CT_Delay();
		CT_IIC_SCL(1);
		delay_us(1);
		receive<<=1;
		if(CT_READ_SDA)receive++;  
		delay_us(2);
	}
	if (!ack)CT_IIC_NAck();
	else CT_IIC_Ack();
 	return receive;
}



