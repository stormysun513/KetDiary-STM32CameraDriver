#include <cstdio>
#include "stm32f4_discovery.h"

#include "main.h"
#include "sccb.h"
#include "serial_interface.h"
#include "sensor_address.h"
#include "application.h"
#include "camera_api.h"
#include "dcmi_ov2640.h"

#define BAUDRATE                14400
#define I2C_CLOCK_SPEED         100000
#define OWN_ADDR                0x3C

extern __IO uint32_t TimmingDelay;

RCC_ClocksTypeDef RCC_Clocks;
char DebugString[50];
uint8_t result[5];
volatile uint8_t image[MAX_BUF_SIZE] = {0};
volatile AppStateTypeDef appStateTypeDef = CAMERAIDLE;
bool initialized = false; 

int main()
{ 
  //Enable the GPIOD Clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|
                         RCC_AHB1Periph_GPIOB|
                         RCC_AHB1Periph_GPIOD, 
                         ENABLE);
  
  //LED Configuration
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12|
                             GPIO_Pin_13|
                             GPIO_Pin_14|
                             GPIO_Pin_15;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;  
  GPIO_Init(GPIOD, &GPIO_InitStruct);
  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);
  
  /* I2C2 SDA and SCL configuration for PB10 and PB11*/
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  //GPIOB Configuration for I2C
  /* I2C2 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
 
  I2C_InitTypeDef  I2C_InitStruct;
  
  /* Configure I2C2 */
  I2C_StructInit(&I2C_InitStruct);
  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStruct.I2C_OwnAddress1 = OWN_ADDR;
  I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStruct.I2C_ClockSpeed = I2C_CLOCK_SPEED;

  I2C_Init(I2C2, &I2C_InitStruct);
  I2C_Cmd(I2C2, ENABLE);
  
  //GPIOA Configuration for USART2
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
  
  //Initialize pins as alternating function
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2| GPIO_Pin_3;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  //UART Initialization
  USART_InitTypeDef USART_InitStruct;
  NVIC_InitTypeDef NVIC_InitStruct;
 
  /**
  * Enable clock for USART1 peripheral
  */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
 
  /**
  * Set Baudrate to value you pass to function
  * Disable Hardware Flow control
  * Set Mode To TX and RX, so USART will work in full-duplex mode
  * Disable parity bit
  * Set 1 stop bit
  * Set Data bits to 8
  *
  * Initialize USART2
  * Activate USART2
  */
  USART_InitStruct.USART_BaudRate = BAUDRATE;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = USART_Mode_Tx| USART_Mode_Rx;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_Init(USART2, &USART_InitStruct);
  USART_Cmd(USART2, ENABLE);
  
  /**
  * Enable RX interrupt
  */
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
 
  /**
  * Set Channel to USART2
  * Set Channel Cmd to enable. That will enable USART1 channel in NVIC
  * Set Both priorities to 0. This means high priority
  *
  * Initialize NVIC
  */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStruct);
  
  /* DMA1 clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  
  /* DMA1 Configuration for USART2*/
  DMA_InitTypeDef  DMA_InitStructure;
 
  DMA_DeInit(DMA1_Stream6);
 
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; // Transmit
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)image;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)UART_TX_SIZE;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART2->DR);
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
 
  DMA_Init(DMA1_Stream6, &DMA_InitStructure);
 
//  /* Enable the USART Tx DMA request */
//  USART_DMACmd(UART5, USART_DMAReq_Tx, ENABLE);
 
  /* Enable DMA Stream Transfer Complete interrupt */
  DMA_ITConfig(DMA1_Stream6, DMA_IT_TC, ENABLE);
 
//  /* Enable the DMA RX Stream */
//  DMA_Cmd(DMA1_Stream6, ENABLE);
  
  //Systick Configuration
  SysTick_Config(SystemCoreClock/10000);        // Minimum delay unit: 100us
  
  //DEBUG PURPOSE
  RCC_GetClocksFreq(&RCC_Clocks); 
//  UartPrint(USART2, "Peripherals are initialized successfully.\n");
  
//  if(I2CReadMulti(I2C2, (uint8_t)TMP102_ADDR, (uint8_t*)DebugString, 2)){
//    float celcius = computeTemperature(DebugString[0], DebugString[1]);
//    sprintf(DebugString, "Temp : %4.2f\n", celcius);
//    UartPrint(USART2, DebugString);
//  }
//  else{
//    UartPrint(USART2, "I2C failed.\n");
//  }
  
  //Initialize OV2640
  SCCB_GPIO_Config(); 
  DelayMs(1);
  
  OV2640_IDTypeDef OV2640ID;
  OV2640_ReadID(&OV2640ID);
  Camera_Config();
  if(OV2640ID.PIDH == OV2640_ID){
//    UartPrint(USART2, "Camera is initialized.\n");
  }
  else{
//    UartPrint(USART2, "Camera failed.\n");
  }
  
  initialized = true;
//  CameraInterfaceReset(); 
  appStateTypeDef = CAPTURECMD;
   
  int i = 0;
  //LED Toggle
  while(1){
//    //Insert a delay of 500ms
//    DelayMs(500);
    
//    //Toggle the LED
//    GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
    
    uint8_t available = UartAvailableBytes();
    if(available > 0){
      UartPktParse();
    }
    
    switch (appStateTypeDef) {
    case CAPTURECMD:
      CameraCapture();
      appStateTypeDef = CAPTURING;
      i = 0;
      break;
    case CAPPOSTPROCESSING:
      for(i = totalTransmitSize-1; i > 0; i--){
        if( image[i] != 0 ){
          if(image[i] == 0xD9 && image[i-1] == 0xFF){
            setTransmitSize(i+1);
//            sprintf(DebugString, "PostProcessed size: %d.\n", i+1);
//            UartPrint(USART2, DebugString);
            GPIO_ResetBits(GPIOD, GPIO_Pin_12);
            notifyImageInfo();
          }
          else{
            setTransmitSize(0);
//            UartPrint(USART2, "Err while checking JPEG format!\n");
          }
          i = 0;
          break;
        } 
      }
      appStateTypeDef = CAPTURED;
      break;
    case CAPTURED:
    case CAPTURING:
    case CAMERAIDLE:
    default:
      break;
    }
  }
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds
  * @retval None
  */
void FlashLED(void){
   GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
}

void DelayMs(__IO uint32_t time){
  TimmingDelay = time*10;
  while(TimmingDelay != 0){
    __WFI();
  }
}

void DelayUs(__IO uint32_t time){
  TimmingDelay = time/100;
  while(TimmingDelay != 0){
    __WFI();
  }
}