#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include "stm32f4xx.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_usart.h"

typedef enum{
  USART_IDLE,
  USART_TRANSFERING
}USARTTransStateTypeDef;

extern USARTTransStateTypeDef usartTransStateTypeDef;
extern uint32_t totalTransmitSize;
extern uint32_t alreadyTransmitSize;

extern uint8_t isChecksum;
extern uint8_t checksum;

void SerialInterfaceInit();
bool UartInsertByte(char byte);
int UartAvailableBytes();
void UartPktParse();

bool UartPrint(USART_TypeDef* USARTx, const char* buf);
bool UartPrintBuf(USART_TypeDef* USARTx, char* buf, int length);
void DMA1_Interrupt_Enable(void);
void DMA1_Interrupt_Disable(void);
bool UartDMASendFromBeginning(void);
bool UartDMASendContinue(void);
bool UartDMASendPartial(uint32_t beginning, uint32_t length);
void setTransmitSize(uint32_t size);
void notifyImageInfo(void);

bool I2CStart(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction);
bool I2CReadByte(I2C_TypeDef* I2Cx, uint8_t *pByte, bool isAck);
bool I2CReadMulti(I2C_TypeDef* I2Cx, uint8_t slaveAddr, uint8_t* buf, uint8_t length);

void SetTimeout(int16_t timeout);
void DisableTimer();
  
  
#endif