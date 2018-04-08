#include "stm32f10x.h"
#include "system.h"
#include "uart_driver.h"
#include "io.h"
#include "timer.h"
#include "miniutils.h"
#include "taskq.h"
#include "cli.h"
#include "processor.h"
#include "linker_symaccess.h"
#include "app.h"
#include "gpio.h"
#include "usb/usb_arcade.h"

static void assert_cb(void) {
  set_print_output(IOSTD);
//  uint32_t ipsr;
//  asm volatile ("MRS %0, ipsr" : "=r" (ipsr) );
//  print("IPSR 0x%02x\nBPRI 0x%02x\n", ipsr, __get_BASEPRI());
}

// main entry from bootstrap

int main(void) {
  enter_critical();
  PROC_base_init();
  SYS_init();
  UART_init();
  UART_assure_tx(_UART(0), TRUE);
  //UART_sync_tx(_UART(0), TRUE);
  PROC_periph_init();
  exit_critical();

  SYS_set_assert_callback(assert_cb);
  SYS_set_assert_behaviour(ASSERT_RESET);

  IO_define(IOSTD, io_uart, UARTSTDIN);
#ifndef CONFIG_ANNOYATRON
  IO_define(IOUSB, io_usb, -1);
#endif

  USB_ARC_init();

  print("\n\n\nHardware initialization done\n");

  print("Stack 0x%08x -- 0x%08x\n", STACK_START, STACK_END);

  print("Subsystem initialization done\n");

  TASK_init();

  CLI_init();

  rand_seed(0xd0decaed ^ SYS_get_tick());

  APP_init();

  while (1) {
    while (TASK_tick());
    TASK_wait();
  }

  return 0;
}

void assert_failed(uint8_t* file, uint32_t line) {
  SYS_assert((char*)file, (s32_t)line);
}
