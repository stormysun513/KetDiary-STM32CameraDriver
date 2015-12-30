#ifndef SCCB_H
#define SCCB_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h" 

/* PD11 : SIOD-C, PD10 : SIOD-D */
#define SCCB_SIC_BIT     GPIO_Pin_11  
#define SCCB_SID_BIT     GPIO_Pin_10  
  
//#define SCCB_SIC_H()     GPIOD->BSRRL = SCCB_SIC_BIT;  
//#define SCCB_SIC_L()     GPIOD->BSRRH =  SCCB_SIC_BIT;  

#define SCCB_SIC_H()     GPIOD->ODR |= SCCB_SIC_BIT;  
#define SCCB_SIC_L()     GPIOD->ODR &= ~SCCB_SIC_BIT;
  
#define SCCB_SID_H()     GPIOD->ODR |= SCCB_SID_BIT;  
#define SCCB_SID_L()     GPIOD->ODR &= ~SCCB_SID_BIT;  
/**/  
#define SCCB_SID_IN      SCCB_SID_GPIO_INPUT();  
#define SCCB_SID_OUT     SCCB_SID_GPIO_OUTPUT();  
  
#define SCCB_SID_STATE   (GPIOD->IDR & SCCB_SID_BIT) 
  
///////////////////////////////////////////  
void SCCB_GPIO_Config(void);  
void SCCB_SID_GPIO_OUTPUT(void);  
void SCCB_SID_GPIO_INPUT(void);  
void startSCCB(void);  
void stopSCCB(void);  
void noAck(void);  
uint8_t SCCBwriteByte(uint8_t m_data);  
uint8_t SCCBreadByte(void);  

uint8_t OV2640_WriteReg(uint8_t regID, uint8_t regDat);
uint8_t OV2640_ReadReg(uint8_t regID);

#endif