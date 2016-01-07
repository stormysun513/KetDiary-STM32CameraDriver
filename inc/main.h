#ifndef MAIN_H
#define MAIN_H
#include "stm32f4xx.h"

#define IMAGE_SIZE              (160*120)
#define MAX_BUF_SIZE             57600
#define UART_TX_SIZE             160*120*2       //19200 bytes

void DelayMs(__IO uint32_t time);
void DelayUs(__IO uint32_t time);

typedef enum{
  CAMERAIDLE,
  CAPTURECMD,
  CAPTURING,
  CAPPOSTPROCESSING,
  CAPTURED
}AppStateTypeDef;

extern char DebugString[50];
extern volatile AppStateTypeDef appStateTypeDef;
//extern volatile uint16_t image[IMAGE_SIZE];
extern volatile uint8_t image[MAX_BUF_SIZE];

#endif