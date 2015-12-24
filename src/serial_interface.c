#include <cstring>

#include "stm32f4xx_usart.h"
#include "ring_buffer.h"
#include "serial_interface.h"

#define FIRST_PREAMBLE		0xFF
#define SECOND_PREAMBLE		0x7F

static volatile bool isFirstUartPreamble = false;
static volatile bool isSecondUartPreamble = false;

const char TYPE1 = 0x01;      //ECG
//extern const BYTE TYPE2;      //ACC
//extern const BYTE TYPE3;      //TMP

static RingBuffer uart_rx_buf;

extern __IO int32_t Timeout;
static int32_t max_timeout = MAX_TIMEOUT;

void SerialInterfaceInit(){
	RingBuffer_init(&uart_rx_buf);
}

bool UartInsertByte(char byte){
  return RingBuffer_writebyte(&uart_rx_buf, byte);
}

int UartAvailableBytes(){
  return RingBuffer_available_data(&uart_rx_buf);
}

void UartPktParse(){
  while(UartAvailableBytes() > 0){
    char byte;
    RingBuffer_readbyte(&uart_rx_buf, &byte);
		
    if(isFirstUartPreamble == false && byte == FIRST_PREAMBLE)
      isFirstUartPreamble = true;

    if(isFirstUartPreamble){
      if(isSecondUartPreamble == false){
        if(byte == SECOND_PREAMBLE)
          isSecondUartPreamble = true;
        else if(byte != FIRST_PREAMBLE)
          isFirstUartPreamble = false;
      }
      else{
        //Uart Packet with accurate header
        if(byte == TYPE1)
          UartPrint(USART2, "OK!\n");
        isFirstUartPreamble = false;
        isSecondUartPreamble = false;
        break;
      }
    }
  }
}

void UartPrint(USART_TypeDef* USARTx, const char* buf){
  int length = strlen(buf);
  int i;
  for(i = 0; i < length; i++){
    /* Wait until Tx data register is empty, not really 
    * required for this example but put in here anyway.
    */
    // Chech RTE9.pdf for more information
    // Check whether TXE is set
    while(!(USARTx->SR & USART_FLAG_TXE) );
    USART_SendData(USARTx, buf[i]);
    // Check whether TC is set
    while(!(USARTx->SR & USART_FLAG_TC));
    USARTx->SR |= USART_FLAG_TC;
  }
}

void UartPrintBuf(const char* buf, int len){
}

bool I2CStart(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction){
  // Set timeout value
  SetTimeout(1000);

  // wait until I2C1 is not busy anymore
  //while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));
  //TIMED((I2Cx->SR2&I2C_FLAG_BUSY));
  TIMED(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));

  // Send I2C1 START condition 
  I2C_GenerateSTART(I2Cx, ENABLE);
  // I2Cx->CR1 |= I2C_CR1_START;

  // wait for I2C1 EV5 --> Slave has acknowledged start condition
  TIMED(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
  //while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
  //while(!(I2Cx->SR1&I2C_SR1_SB));

  // Send slave Address for write 
  I2C_Send7bitAddress(I2Cx, address, direction);

  /* wait for I2C1 EV6, check if 
  * either Slave has acknowledged Master transmitter or
  * Master receiver mode, depending on the transmission
  * direction
  */ 
  if(direction == I2C_Direction_Transmitter){
    //while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    TIMED(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  } else if(direction == I2C_Direction_Receiver){
    //while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    TIMED(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
  }

  DisableTimer();
  return true;
}

bool I2CReadByte(I2C_TypeDef* I2Cx, uint8_t *pByte, bool isAck){
  // Set timeout value
  SetTimeout(1000);

  if(isAck){
    // enable acknowledge of recieved data
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
  }
  else{
    // disabe acknowledge of received data
    // nack also generates stop condition after last byte received
    // see reference manual for more info
    I2C_AcknowledgeConfig(I2Cx, DISABLE);
    I2C_GenerateSTOP(I2Cx, ENABLE);
  }
  // wait until one byte has been received
  // while( !I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) );
  TIMED( !I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) );

  // read data from I2C data register and return data byte
  *pByte = I2C_ReceiveData(I2Cx);
  DisableTimer();
  return true;
}

bool I2CReadMulti(I2C_TypeDef* I2Cx, uint8_t slaveAddr, uint8_t* buf, uint8_t length){
  uint8_t i;
  bool state = I2CStart(I2Cx, slaveAddr, I2C_Direction_Receiver);
  for (i = 0; i < length; i++) {
    if(state == false)
      return false;
    if (i == (length - 1)) {
      /* Last byte */
      state = I2CReadByte(I2Cx, buf + i, false);
    }else{
      state = I2CReadByte(I2Cx, buf + i, true);
    }
  }
  return true;
}

void SetTimeout(int16_t timeout){
  // Timeout after "timeout" ms.
  if(timeout < 0)
    return;
  max_timeout = timeout;
  Timeout = timeout;
}

void DisableTimer(){
  Timeout = -1;
}
