#include "system.h"
#include "stm32f10x_it.h"
#include "uart_driver.h"
#include "timer.h"
#include "usb_istr.h"

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
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
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}


/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

#ifdef CONFIG_UART1
void USART1_IRQHandler(void)
{
  TRACE_IRQ_ENTER(USART1_IRQn);
  UART_irq(&__uart_vec[1]);
  TRACE_IRQ_EXIT(USART1_IRQn);
}
#endif

#ifdef CONFIG_UART2
void USART2_IRQHandler(void)
{
  //TRACE_IRQ_ENTER(USART2_IRQn);
  UART_irq(&__uart_vec[0]);
  //TRACE_IRQ_EXIT(USART2_IRQn);
}
#endif

#ifdef CONFIG_UART3
void USART3_IRQHandler(void)
{
  TRACE_IRQ_ENTER(USART3_IRQn);
  UART_irq(&__uart_vec[2]);
  TRACE_IRQ_EXIT(USART3_IRQn);
}
#endif

#ifdef CONFIG_UART4
void UART4_IRQHandler(void)
{
  TRACE_IRQ_ENTER(UART4_IRQn);
  UART_irq(&__uart_vec[3]);
  TRACE_IRQ_EXIT(UART4_IRQn);
}
#endif

void STM32_SYSTEM_TIMER_IRQ_FN(void)
{
  //TRACE_IRQ_ENTER(STM32_SYSTEM_TIMER_IRQn);
  TIMER_irq();
  //TRACE_IRQ_EXIT(STM32_SYSTEM_TIMER_IRQn);
}

// usb
void USBWakeUp_IRQHandler(void)
{
  TRACE_IRQ_ENTER(USBWakeUp_IRQn);
  EXTI_ClearITPendingBit(EXTI_Line18);
  TRACE_IRQ_EXIT(USBWakeUp_IRQn);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
// Called once every ms
//  TRACE_IRQ_ENTER(USB_LP_CAN1_RX0_IRQn);
  USB_Istr();
//  TRACE_IRQ_EXIT(USB_LP_CAN1_RX0_IRQn);
}

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
