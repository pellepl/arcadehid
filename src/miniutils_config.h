/*
 * miniutils_config.h
 *
 *  Created on: Sep 7, 2013
 *      Author: petera
 */

#ifndef MINIUTILS_CONFIG_H_
#define MINIUTILS_CONFIG_H_

#include "system.h"
#include "uart_driver.h"
#include "usb_serial.h"

#ifdef CONFIG_ARCHID_VCD
#define PUTC(p, c)  \
  if ((int)(p) < 256) {\
    if (p == IOSTD) UART_put_char(_UART((int)(p)), (c)); \
    else if (p == IOUSB) USB_SER_tx_char((c)); \
  } else { \
    *((char*)(p)++) = (c); \
  }
#define PUTB(p, b, l)  \
  if ((int)(p) < 256) {\
    if (p == IOSTD) UART_put_buf(_UART((int)(p)), (u8_t*)(b), (int)(l)); \
    else if (p == IOUSB) USB_SER_tx_buf((u8_t*)(b), (int)(l)); \
  } else { \
    int ____l = (l); \
    memcpy((char*)(p),(b),____l); \
    (p)+=____l; \
  }
#else
#define PUTC(p, c)  \
  if ((int)(p) < 256) {\
    if (p == IOSTD) UART_put_char(_UART((int)(p)), (c)); \
  } else { \
    *((char*)(p)++) = (c); \
  }
#define PUTB(p, b, l)  \
  if ((int)(p) < 256) {\
    if (p == IOSTD) UART_put_buf(_UART((int)(p)), (u8_t*)(b), (int)(l)); \
  } else { \
    int ____l = (l); \
    memcpy((char*)(p),(b),____l); \
    (p)+=____l; \
  }
#endif
#endif /* MINIUTILS_CONFIG_H_ */
