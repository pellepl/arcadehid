/*
 * timer.c
 *
 *  Created on: Jun 23, 2012
 *      Author: petera
 */

#include "timer.h"
#include "system.h"
#include "stm32f10x.h"
#include "miniutils.h"
#include "uart_driver.h"
#include "cli.h"

void TIMER_irq() {
  if (TIM_GetITStatus(STM32_SYSTEM_TIMER, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(STM32_SYSTEM_TIMER, TIM_IT_Update);

    bool ms_update = SYS_timer();
    if (ms_update) {
      TRACE_MS_TICK(SYS_get_time_ms() & 0xff);
    }
    TASK_timer();
    CLI_timer();
  }
}
