#ifndef MAIN_H
#define MAIN_H
#include "stm32f4xx.h"

#define IMAGE_SIZE              (160*120)

void DelayMs(__IO uint32_t time);
void DelayUs(__IO uint32_t time);

typedef enum{
  CAMERAIDLE,
  CAPTURECMD,
  CAPTURING,
  CAPTURED
}AppStateTypeDef;

extern volatile AppStateTypeDef appStateTypeDef;

#endif