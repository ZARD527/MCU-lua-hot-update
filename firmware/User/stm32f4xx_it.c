/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

 
#include "stm32f4xx_it.h"
#include "include.h"
#include "Config.h"

 



 
 
 
 
 
 

 
 
 

 




void NMI_Handler(void)
{
}

 




void HardFault_Handler(void)
{
   
  while (1)
  {
  }
}

 




void MemManage_Handler(void)
{
   
  while (1)
  {
  }
}

 




void BusFault_Handler(void)
{
   
  while (1)
  {
  }
}

 




void UsageFault_Handler(void)
{
   
  while (1)
  {
  }
}

 




void SVC_Handler(void)
{
}

 




void DebugMon_Handler(void)
{
}

 




void PendSV_Handler(void)
{
}

 




static volatile uint32_t g_sysMs;
void SysTick_Handler(void)
{
  g_sysMs++;
}

uint32_t SysTick_GetMs(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t ms = g_sysMs;
    __set_PRIMASK(primask);
    return ms;
}

void RTC_WKUP_IRQHandler(void)
{
    Timer_RTCWakeupIRQHandler();
}

void USART1_IRQHandler(void)
{
    UART_H_IRQHandler();
}

 
 
 
 
 
 

 




 



 

 


