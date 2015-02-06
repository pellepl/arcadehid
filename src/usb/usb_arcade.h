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

typedef struct {
  union {
    u8_t raw[4];
    struct {
      s8_t dx;
      s8_t dy;
      u8_t buttons1;
      u8_t buttons2;
    };
  };
} usb_joystick_report;

typedef enum {
  JOYSTICK1 = 0,
  JOYSTICK2
} usb_joystick;

typedef void (*usb_kb_report_ready_cb_f)(void);
typedef void (*usb_mouse_report_ready_cb_f)(void);
typedef void (*usb_joy_report_ready_cb_f)(usb_joystick joystick);

bool USB_ARC_KB_can_tx(void);
bool USB_ARC_MOUSE_can_tx(void);
bool USB_ARC_JOYSTICK_can_tx(usb_joystick joystick);
void USB_ARC_KB_tx(usb_kb_report *report);
void USB_ARC_MOUSE_tx(usb_mouse_report *report);
void USB_ARC_JOYSTICK_tx(usb_joystick joystick, usb_joystick_report *report);
void USB_ARC_set_kb_callback(usb_kb_report_ready_cb_f cb);
void USB_ARC_set_mouse_callback(usb_mouse_report_ready_cb_f cb);
void USB_ARC_set_joystick_callback(usb_joy_report_ready_cb_f cb);

void USB_ARC_init(void);

#endif /* USB_ARC_H_ */
