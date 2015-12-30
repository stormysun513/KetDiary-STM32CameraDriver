/* 
GUANFU_WANG 
*/  
#include "sccb.h"  
#include "main.h"  
 
/* 
----------------------------------------------- 
   功能: 初始化模拟SCCB接口 
   参数: 无 
 返回值: 无 
----------------------------------------------- 
*/  
void SCCB_GPIO_Config(void)  
{  
  GPIO_InitTypeDef GPIO_InitStruct;  
   /* Enable GPIOD  clock */  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  
  GPIO_InitStruct.GPIO_Pin =  SCCB_SIC_BIT|SCCB_SID_BIT;  
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;  
  GPIO_InitStruct.GPIO_Speed =GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;  
  GPIO_Init(GPIOD, &GPIO_InitStruct);  
} 

void SCCB_SID_GPIO_OUTPUT(void)  
{  
  GPIO_InitTypeDef GPIO_InitStruct;  
   /* Enable GPIOD  clock */  
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  
  GPIO_InitStruct.GPIO_Pin =  SCCB_SID_BIT;  
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;  
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;   
  GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void SCCB_SID_GPIO_INPUT(void)  
{  
  GPIO_InitTypeDef GPIO_InitStruct;  
   /* Enable GPIOC  clock */  
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  
  GPIO_InitStruct.GPIO_Pin =  SCCB_SID_BIT;  
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;  
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;   
  GPIO_Init(GPIOD, &GPIO_InitStruct);  
}  
/* 
----------------------------------------------- 
   功能: start命令,SCCB的起始信号 
   参数: 无 
 返回值: 无 
----------------------------------------------- 
*/  
void startSCCB(void)  
{
  SCCB_SID_H();     //数据线高电平
  DelayUs(500);

  SCCB_SIC_H();      //在时钟线高的时候数据线由高至低
  DelayUs(500);

  SCCB_SID_L();
  DelayUs(500);

  SCCB_SIC_L();    //数据线恢复低电平，单操作函数必要
  DelayUs(500);  
}  
/* 
----------------------------------------------- 
   功能: stop命令,SCCB的停止信号 
   参数: 无 
 返回值: 无 
----------------------------------------------- 
*/  
void stopSCCB(void)  
{
  SCCB_SID_L();
  DelayUs(500);

  SCCB_SIC_H();
  DelayUs(500);

  SCCB_SID_H();
  DelayUs(500);   
}  

/* 
----------------------------------------------- 
   功能: noAck,用于连续读取中的最后一个结束周期 
   参数: 无 
 返回值: 无 
----------------------------------------------- 
*/  
void noAck(void)  
{
  SCCB_SID_H();
  DelayUs(500);

  SCCB_SIC_H();
  DelayUs(500);

  SCCB_SIC_L();
  DelayUs(500);

  SCCB_SID_L();
  DelayUs(500);
}  
  
/* 
----------------------------------------------- 
   功能: 写入一个字节的数据到SCCB 
   参数: 写入数据 
 返回值: 发送成功返回1，发送失败返回0 
----------------------------------------------- 
*/  

uint8_t SCCBwriteByte(uint8_t m_data)  
{
  uint8_t j,tem;  
  for(j = 0; j < 8; j++){
    if((m_data >> (7 - j))&0x01){
      SCCB_SID_H();
    }
    else{
      SCCB_SID_L(); 
    }  
    DelayUs(500); 
    SCCB_SIC_H();
    DelayUs(500);  
    SCCB_SIC_L();   
    DelayUs(500);  
  }  
  DelayUs(100);  
  SCCB_SID_IN;/*设置SDA为输入*/  
  DelayUs(500);  
  SCCB_SIC_H();     
  DelayUs(500);

  if(SCCB_SID_STATE){
    tem=0;                  //SDA=1发送失败，返回0}
    //while(1); //Error happened
  }
  else{
    tem=1;
  }
                            //SDA=0发送成功，返回1  
  SCCB_SIC_L();     
  DelayUs(500);    
  SCCB_SID_OUT;/*设置SDA为输出*/  
  
  return (tem);    
}  

/* 
----------------------------------------------- 
   功能: 一个字节数据读取并且返回 
   参数: 无 
 返回值: 读取到的数据 
----------------------------------------------- 
*/  
uint8_t SCCBreadByte(void)  
{
  uint8_t read,j;
  read=0x00;  

  SCCB_SID_IN;/*设置SDA为输入*/
  DelayUs(500);
  for(j=8;j>0;j--) //循环8次接收数据
  {
    DelayUs(500);
    SCCB_SIC_H();
    DelayUs(500);  
    read = read << 1;
    if(SCCB_SID_STATE)
    {
      read=read+1;  
    }
    SCCB_SIC_L();
    DelayUs(500);  
  }     
  SCCB_SID_OUT;/*设置SDA为输出*/  
  return(read);
}  


////////////////////////////  
//功能：写OV7660寄存器  
//返回：1-成功   0-失败  
uint8_t OV2640_WriteReg(uint8_t regID, uint8_t regDat)  
{  
//  GPIO_ToggleBits(GPIOD, GPIO_Pin_13); 
    startSCCB();  
    if(0==SCCBwriteByte(0x60))  
    {  
        stopSCCB();  
        return(0xFF);  
    }  
//    DelayUs(100);  
    if(0==SCCBwriteByte(regID))  
    {  
        stopSCCB();  
        return(0xFF);  
    }  
//    DelayUs(100);  
    if(0==SCCBwriteByte(regDat))  
    {  
        stopSCCB();  
        return(0xFF);  
    }  
    stopSCCB();  

//  GPIO_ToggleBits(GPIOD, GPIO_Pin_13); 
  return 0;  
}

////////////////////////////  
//功能：读OV7660寄存器  
//返回：1-成功   0-失败  
uint8_t OV2640_ReadReg(uint8_t regID)  
{
  DelayUs(100);
  uint8_t regDat = 0xFF;
  
  //通过写操作设置寄存器地址
  SCCB_SID_OUT;
  startSCCB();
  if(0==SCCBwriteByte(0x60))
  {
    stopSCCB();
    return 0xFF;
  }
  
  DelayUs(100);
  if(0==SCCBwriteByte(regID))
  {
    stopSCCB();
    return 0xFF;  
  }
  stopSCCB();
       
  //设置寄存器地址后，才是读
  SCCB_SID_OUT;
  startSCCB();  
  if(0==SCCBwriteByte(0x61))
  {
    stopSCCB();
    return 0xFF;  
  }

  regDat=SCCBreadByte();
  noAck();
  stopSCCB();  
  return regDat;  
}  