/*
 * usb_kb.h
 *
 *  Created on: Dec 29, 2014
 *      Author: petera
 */

#ifndef USB_KB_H_
#define USB_KB_H_

#include "system.h"

typedef struct {
  union {
    u8_t raw[1 + 1 + USB_REPORT_KEYMAP_SIZE];
    struct {
      u8_t modifiers;
      u8_t reserved;
      u8_t keymap[USB_REPORT_KEYMAP_SIZE];
    };
  };
} usb_report;


void USB_KB_tx(usb_report *report);
void USB_KB_init(void);

#endif /* USB_KB_H_ */
