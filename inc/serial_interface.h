#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include "stm32f4xx_i2c.h"
#include "stm32f4xx_usart.h"


void SerialInterfaceInit();

bool UartInsertByte(char byte);
int UartAvailableBytes();
void UartPktParse();

void UartPrint(USART_TypeDef* USARTx, const char* buf);
void UartPrintBuf(const char* buf, int len);

bool I2CStart(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction);
bool I2CReadByte(I2C_TypeDef* I2Cx, uint8_t *pByte, bool isAck);

bool I2CReadMulti(I2C_TypeDef* I2Cx, uint8_t slaveAddr, uint8_t* buf, uint8_t length);

void SetTimeout(int16_t timeout);
void DisableTimer();
  
  
#endif