/*
 * system_config.h
 *
 *  Created on: Jul 24, 2012
 *      Author: petera
 */

#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

#include "config_header.h"
#include "types.h"
#include "stm32f10x.h"


#define APP_NAME "ARCADEHID"


/****************************************/
/***** Functionality block settings *****/
/****************************************/


// enable uart2
#define CONFIG_UART2

#define CONFIG_UART_CNT   1 // update according to enabled uarts


/*********************************************/
/***** Hardware build time configuration *****/
/*********************************************/

/** Processor specifics **/

#ifndef USER_HARDFAULT
// enable user hardfault handler
#define USER_HARDFAULT 1
#endif

// hardware debug (blinking leds etc)
#define HW_DEBUG


/** General **/

// internal flash start address
#define FLASH_START       FLASH_BASE
// internal flash page erase size
#define FLASH_PAGE_SIZE   0x400 // md
// internal flash protection/unprotection for firmware
#define FLASH_PROTECT     FLASH_WRProt_AllPages
// internal flash total size in bytes
#define FLASH_TOTAL_SIZE  (128*1024) // md

/** UART **/

#ifdef CONFIG_UART2
#define UART2_GPIO_PORT       GPIOA
#define UART2_GPIO_RX         GPIO_Pin_3
#define UART2_GPIO_TX         GPIO_Pin_2
#endif

/** USB STUFF **/
#define ID1                                 (0x1FFFF7E8)
#define ID2                                 (0x1FFFF7EC)
#define ID3                                 (0x1FFFF7F0)

/** I2C **/
#define I2C_MAX_ID            1
#define I2C1_PORT             I2C1

/****************************************************/
/******** Application build time configuration ******/
/****************************************************/

/** TICKER **/
// STM32 system timer
#define CONFIG_STM32_SYSTEM_TIMER   2
// system timer frequency
#define SYS_MAIN_TIMER_FREQ   40000
// system timer counter type
typedef u16_t system_counter_type;
// system tick frequency
#define SYS_TIMER_TICK_FREQ   1000
// os ticker cpu clock div
#define SYS_OS_TICK_DIV       8

/** PRINT **/
void arcprint(const char* f, ...);
void set_print_output(u8_t io);
u8_t get_print_output(void);

#define print(...) arcprint(__VA_ARGS__)

/** UART **/

#define UARTSTDIN       0
#define UARTSTDOUT      0

#define UART2_SPEED 460800

#define USE_COLOR_CODING

#define CONFIG_ARCHID_VCD

/** IO **/
#define CONFIG_IO_MAX   2

#define IOSTD        0
#define IODBG        IOSTD
#define IOUSB        1

/** MATH **/
#define CONFIG_TRIGQ_TABLE

/** TASK KERNEL **/
#define CONFIG_TASK_POOL 32
//#define CONFIG_TASK_NONCRITICAL_TIMER
//#define CONFIG_TASKQ_DBG_CRITICAL
#define CONFIG_TASKQ_MUTEX

/** USB **/

#define USB_KB_REPORT_KEYMAP_SIZE     32 /* 6 */

/** APP CONFIG **/

#ifdef CONFIG_HY_TEST_BOARD
#define APP_CONFIG_PINS               4
#else
#define APP_CONFIG_PINS               26
#endif
#define APP_CONFIG_DEFS_PER_PIN       8


/** DEBUG **/

#define DBG_ATTRIBUTE __attribute__(( section(".shmem") ))

// disable all asserts
//#define ASSERT_OFF

// disable all debug output
//#define DBG_OFF

#define CONFIG_DEFAULT_DEBUG_MASK     (0xffffffff)

// enable or disable tracing
#define DBG_TRACE_MON
#define TRACE_SIZE            (64)

#define VALID_RAM(x) \
  (((void*)(x) >= RAM_BEGIN && (void*)(x) < RAM_END))

#define VALID_FLASH(x) \
  ((void*)(x) >= (void*)FLASH_BEGIN && (void*)(x) < (void*)(FLASH_END))

#define VALID_DATA(x) \
  (VALID_RAM(x) || VALID_FLASH(x))

#define OS_DBG_BADR(x) \
    (!VALID_RAM(x))


#endif /* SYSTEM_CONFIG_H_ */
