#include <cstdio>

#include "stm32f4xx.h"
#include "stm32f4_discovery.h"

#include "main.h"
#include "serial_interface.h"
#include "sensor_address.h"
#include "application.h"

#define BAUDRATE                115200
#define I2C_CLOCK_SPEED         10000
#define OWN_ADDR                0x3C

extern __IO uint32_t TimmingDelay;

RCC_ClocksTypeDef RCC_Clocks;
char DebugString[50];

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
  NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStruct);
  
  //Systick Configuration
  SysTick_Config(SystemCoreClock/1000);
  
  //DEBUG PURPOSE
  RCC_GetClocksFreq(&RCC_Clocks); 
  UartPrint(USART2, "Initialized successfully.\n");
  
  uint8_t result[10]; 
  //LED Toggle
  while(1){
    //Insert a delay of 500ms
    Delay(500);
    
    //Toggle the LED
    GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
    
//    if(GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_15) == RESET){
//      if(I2CReadMulti(I2C2, (uint8_t)TMP102_ADDR, result, 2)){
//        float celcius = computeTemperature(result[0], result[1]);
//        sprintf(DebugString, "Temp : %4.2f\n", celcius);
//        UartPrint(USART2, DebugString);
//      }
//    }
  }

  return 0;
}

void Delay(__IO uint32_t time){
  TimmingDelay = time;
  while(TimmingDelay != 0){
    __WFI();
  }
}