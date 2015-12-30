/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.6.1
  * @date    21-October-2015
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f4xx_dcmi.h"
#include "main.h"
#include "camera_api.h"
#include "serial_interface.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
__IO uint32_t TimmingDelay;
__IO int32_t Timeout;
int32_t max_timeout;
void SysTick_Handler(void)
{
  if(TimmingDelay != 0)
    TimmingDelay--;
  
  if(Timeout > 0)
    Timeout--;
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**********************************************************
 * USART2 interrupt request handler: on reception of a 
 * character 't', toggle LED and transmit a character 'T'
 *********************************************************/
#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)
void USART2_IRQHandler(void)
{
  /* RXNE handler */
  if(USART2->SR & (USART_FLAG_RXNE|USART_FLAG_ERRORS))
  {
    char byte = (char)USART_ReceiveData(USART2);
    if(!(USART2->SR & USART_FLAG_ERRORS))
      UartInsertByte(byte);
    
    uint8_t available = UartAvailableBytes();
    if(available >= 3){
      UartPktParse();
    }
    USART_ClearITPendingBit(USART2, USART_IT_RXNE);
  }
  /* ------------------------------------------------------------ */
  /* Other USART2 interrupts handler can go here ...   
  */
}

int count = 0;
int start = 0;
void DCMI_IRQHandler(void)
{
  if(DCMI_GetFlagStatus(DCMI_FLAG_VSYNCRI) == SET)
  {
    if(start == 0)
    {
      start = 1;
    }
    else
    {
      start = 0;
    }
    DCMI_ClearFlag(DCMI_FLAG_VSYNCRI);
  }
  else if(DCMI_GetFlagStatus(DCMI_FLAG_LINERI) == SET)
  {
    if(start == 1)
    {
      count++;
    }
    else
    {
      if(count != 0)
      {
        //printf("count: %d \n\n", count); //just dor counting the number of line
      }
      count = 0;
    }
    DCMI_ClearFlag(DCMI_FLAG_LINERI);
  }
  else if(DCMI_GetFlagStatus(DCMI_FLAG_FRAMERI) == SET)
  {
    UartPrint(USART2, "DCMI_FLAG_FRAMERI \n\n");
    appStateTypeDef = CAPTURED;
    DCMI_CaptureCmd(DISABLE);
    CameraInterfaceReset(); 
    DCMI_ClearFlag(DCMI_FLAG_FRAMERI);
  }
  else if(DCMI_GetFlagStatus(DCMI_FLAG_ERRRI) == SET)
  {
    UartPrint(USART2,"DCMI_FLAG_ERRRI \n\n");
    DCMI_ClearFlag(DCMI_FLAG_ERRRI);
  }
  else if(DCMI_GetFlagStatus(DCMI_FLAG_OVFRI) == SET)
  {
    UartPrint(USART2,"DCMI_FLAG_OVFRI \n\n");  //********** Unfortunately.. my code always comes here
    DCMI_ClearFlag(DCMI_FLAG_OVFRI);
  }
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
