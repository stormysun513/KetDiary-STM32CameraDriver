#include <cstring>
#include "stm32f4xx_usart.h"

#include "main.h"
#include "ring_buffer.h"
#include "serial_interface.h"
#include "camera_api.h"

#define FIRST_PREAMBLE		0xFF
#define SECOND_PREAMBLE		0x7F


#define MAX_TIMEOUT		2000
#define TIMED(A)		while(A){if(Timeout == 0)return false;}\
                                Timeout = max_timeout;

static volatile bool isFirstUartPreamble = false;
static volatile bool isSecondUartPreamble = false;

const char BLE_REQUEST_IMAGE_INFO = 0x03;               //BLE_REQUEST_IMAGE_INFO
const char BLE_REQUEST_IMAGE_BY_INDEX = 0x04;           //BLE_REQUEST_IMAGE_BY_INDEX
const char BLE_TAKE_PHOTO = 0x07;
const char BLE_REQUEST_ENTIRE_IMAGE = 0x08;

const char UART_REPLY_IMAGE_INFO = 0x00;                //UART_REPLY_IMAGE_INFO
const char UART_REPLY_IMAGE_PACKET = 0x03;              //UART_REPLY_IMAGE_PACKET

const char TYPE1 = 0xF1;      
const char TYPE2 = 0xF2;      //ACC
const char TYPE3 = 0xF3;      //TMP
char preamble[] = {0xFF, 0xFF, 0x7F};
char data[] = {0, 0, 0, 0};

uint8_t isChecksum = 0;
uint8_t checksum = 0;
const uint32_t uartPacketSize = 111;

static RingBuffer uart_rx_buf;
USARTTransStateTypeDef usartTransStateTypeDef = USART_IDLE;
uint32_t totalTransmitSize = 0;
uint32_t alreadyTransmitSize = 0;


extern __IO int32_t Timeout;
extern int32_t max_timeout;

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
  
  static char waitHeaderType;
  static bool waitMore = false;
  static uint8_t waitNum = 0;
  static char data[] = {0, 0, 0, 0};
  while(UartAvailableBytes() > 0){
    /* Modified after adjust baudrate to 14400 */
//    DelayUs(400);
    /* end */
    
    char byte;
    RingBuffer_readbyte(&uart_rx_buf, &byte);
		
    if(!isFirstUartPreamble && byte == FIRST_PREAMBLE){
//      UartPrint(USART2, "ACK1!\n");
      isFirstUartPreamble = true;
    }
    else if(isFirstUartPreamble){
      if(isSecondUartPreamble == false){
        if(byte == SECOND_PREAMBLE){
//          UartPrint(USART2, "ACK2!\n");
          isSecondUartPreamble = true;
        }
        else if(byte != FIRST_PREAMBLE){
//          UartPrint(USART2, "NACK2!\n");
          isFirstUartPreamble = false;
        }
        else{
//          UartPrint(USART2, "WAIT ACK2!\n");
          // Else keep waiting for second preamble
        }
      }
      else{
//        UartPrint(USART2, "DATA!\n");
        //Uart Packet with accurate header
        char header = byte;
        if(waitMore == true)
          header = waitHeaderType;
          
        switch (header){
        case BLE_REQUEST_ENTIRE_IMAGE:                          // 0x08
        {
          if(appStateTypeDef == CAPTURED)
            appStateTypeDef = CAMERAIDLE;
          data[0] = (totalTransmitSize >> 24) & 0xFF;
          data[1] = (totalTransmitSize >> 16) & 0xFF;
          data[2] = (totalTransmitSize >> 8) & 0xFF;
          data[3] = totalTransmitSize & 0xFF;
          isChecksum = 0;
          UartPrintBuf(USART2, preamble, 3);
          UartPrintBuf(USART2, data, 4);
          UartDMASendFromBeginning();
          break;
        }
        case BLE_REQUEST_IMAGE_INFO:                            // 0x03
        {
          switch (appStateTypeDef){
          case CAPTURED:
          case CAMERAIDLE:
            appStateTypeDef = CAPTURECMD;
            break;
          default:
            break;
          }
//          notifyImageInfo();
          break;
        }
        case BLE_TAKE_PHOTO:                                    // 0x07
        {
//          UartPrint(USART2, "OK!\n");
          switch (appStateTypeDef){
          case CAPTURED:
            appStateTypeDef = CAMERAIDLE;
            break;
          case CAMERAIDLE:
            appStateTypeDef = CAPTURECMD;
            break;
          default:
            break;
          }
          break;
        }
        case BLE_REQUEST_IMAGE_BY_INDEX:                        // 0x04
        {
          if(waitMore == false){
            waitNum = 2;
            waitMore = true;
            waitHeaderType = byte;
          }
          else{
            data[--waitNum] = byte;
            GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
            if(waitNum == 0){
              waitMore = false;
              /* TODO */
              isChecksum = 0;
              // Not that the order is opposite to both nordic and android
              uint32_t currentId = (data[1] << 8) +  data[0] & 0xFF;
              uint32_t beginning = currentId*uartPacketSize;
              uint32_t remainPacketSize = totalTransmitSize - beginning;
              if(remainPacketSize > uartPacketSize)
                remainPacketSize = uartPacketSize;
              char tempBuf[3] = {0};
              tempBuf[0] = UART_REPLY_IMAGE_PACKET;
              tempBuf[1] = data[1];
              tempBuf[2] = data[0];
              
              UartPrintBuf(USART2, preamble+1, 2);
              UartPrintBuf(USART2, tempBuf, 3);
              UartDMASendPartial(beginning, remainPacketSize);
              checksum = 0;
              for(int i = 0; i < remainPacketSize; i++)
                checksum += image[i+beginning];
              
              if(isChecksum == 2)
                UartPrintBuf(USART2, (char*)&checksum, 1);
              else
                isChecksum = 1;
            }
          }
          break;
        }
        case TYPE1:
          UartPrint(USART2, "OK!\n");
          switch (appStateTypeDef){
          case CAPTURED:
            appStateTypeDef = CAMERAIDLE;
            break;
          case CAMERAIDLE:
            appStateTypeDef = CAPTURECMD;
            break;
          default:
            break;
          }
          break;
        case TYPE2:
          notifyImageInfo();
          break;
        default:
          break;
        }
        
        if(waitMore == false){
          isFirstUartPreamble = false;
          isSecondUartPreamble = false;
          break;
        }
      }
    }
  }
}

bool UartPrint(USART_TypeDef* USARTx, const char* buf){
  if(usartTransStateTypeDef == USART_TRANSFERING)
    return false;
  
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
  return true;
}

bool UartPrintBuf(USART_TypeDef* USARTx, char* buf, int length){
  if(usartTransStateTypeDef == USART_TRANSFERING)
    return false;
   
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
  
  return true;
}
    
void DMA1_Interrupt_Enable(void){
  NVIC_InitTypeDef NVIC_InitStruct;
  
  /* Configure the Priority Group to 2 bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
 
  /* Enable the UART2 RX DMA Interrupt */
  NVIC_InitStruct.NVIC_IRQChannel = DMA1_Stream6_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStruct);
}

void DMA1_Interrupt_Disable(void){
  NVIC_DisableIRQ(DMA1_Stream6_IRQn);
}

bool UartDMASendFromBeginning(void){
  if(totalTransmitSize == 0)
    return false;
  
  if(usartTransStateTypeDef == USART_IDLE){
    DMA1_Stream6->M0AR = (uint32_t)image;
    DMA1_Stream6->NDTR = totalTransmitSize;
    
    DMA_Cmd(DMA1_Stream6, ENABLE);
    DMA1_Interrupt_Enable();
    
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    usartTransStateTypeDef = USART_TRANSFERING;
    return true;
  }
  else{
    return false;
  }
}

bool UartDMASendContinue(void){
  if(usartTransStateTypeDef == USART_IDLE){
    DMA1_Stream6->M0AR = (uint32_t)(image+alreadyTransmitSize);
    DMA1_Stream6->NDTR = totalTransmitSize-alreadyTransmitSize;
    DMA_Cmd(DMA1_Stream6, ENABLE);
    DMA1_Interrupt_Enable();
    
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    usartTransStateTypeDef = USART_TRANSFERING;
    return true;
  }
  else{
    return false;
  }
}

bool UartDMASendPartial(uint32_t beginning, uint32_t length){
  if(usartTransStateTypeDef == USART_IDLE){
    DMA1_Stream6->M0AR = (uint32_t)((uint8_t*)image+beginning);
    DMA1_Stream6->NDTR = length;
    
    DMA_Cmd(DMA1_Stream6, ENABLE);
    DMA1_Interrupt_Enable();
    
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
    usartTransStateTypeDef = USART_TRANSFERING;
    return true;
  }
  else{
    return false;
  }
}

void setTransmitSize(uint32_t size){
  alreadyTransmitSize = 0;
  totalTransmitSize = size;
}

void notifyImageInfo(void){
  char data[] = {0, 0, 0};
  data[0] = UART_REPLY_IMAGE_INFO;
  data[1] = (totalTransmitSize >> 8) & 0xFF;
  data[2] = totalTransmitSize & 0xFF;
  UartPrintBuf(USART2, preamble+1, 2);
  UartPrintBuf(USART2, data, 3);
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
