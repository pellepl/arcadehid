/*
 * timer.c
 *
 *  Created on: Jun 23, 2012
 *      Author: petera
 */

#include "timer.h"
#include "system.h"
#include "taskq.h"
#include "cli.h"
#include "app.h"

void TIMER_irq() {
  if (TIM_GetITStatus(STM32_SYSTEM_TIMER, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(STM32_SYSTEM_TIMER, TIM_IT_Update);

    bool ms_update = SYS_timer();
    if (ms_update) {
      TRACE_MS_TICK(SYS_get_time_ms() & 0xff);
    }
    TASK_timer();
    APP_timer();
    CLI_timer();
  }
}
