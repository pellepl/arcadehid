/*
 * usb_arcade.h
 *
 *  Created on: Dec 29, 2014
 *      Author: petera
 */

#ifndef USB_ARC_H_
#define USB_ARC_H_

#include "system.h"

typedef struct {
  union {
    u8_t raw[1 + 1 + USB_KB_REPORT_KEYMAP_SIZE];
    struct {
      u8_t modifiers;
      u8_t reserved;
      u8_t keymap[USB_KB_REPORT_KEYMAP_SIZE];
    };
  };
} usb_kb_report;

typedef struct {
  union {
    u8_t raw[4];
    struct {
      u8_t modifiers;
      s8_t dx;
      s8_t dy;
      s8_t wheel;
    };
  };
} usb_mouse_report;


void USB_ARC_KB_tx(usb_kb_report *report);
void USB_ARC_MOUSE_tx(usb_mouse_report *report);
void USB_ARC_init(void);

#endif /* USB_ARC_H_ */
