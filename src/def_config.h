/*
 * key_config.h
 *
 *  Created on: Jan 18, 2015
 *      Author: petera
 */

#ifndef SRC_DEF_CONFIG_H_
#define SRC_DEF_CONFIG_H_

#include "system.h"
#include "usb/usb_arc_codes.h"

typedef enum {
  HID_ID_TYPE_NONE = 0,
  HID_ID_TYPE_KEYBOARD,
  HID_ID_TYPE_MOUSE,
  HID_ID_TYPE_JOYSTICK,
} hid_id_type;

typedef struct {
  union {
    struct {
      hid_id_type type : 2;
      u16_t raw : 14;
    };
    struct {
      hid_id_type type : 2;
      u8_t kb_reserved : 6;
      enum kb_hid_code kb_code : 8;
    } kb;
    struct {
      hid_id_type type : 2;
      enum mouse_code mouse_code : 4;
      u8_t reserved : 1;
      u8_t mouse_sign : 1;
      u8_t mouse_acc : 1;
      u8_t mouse_data : 7;
    } mouse;
    struct {
      hid_id_type type : 2;
      enum joystick_code joystick_code : 5;
      u8_t joystick_sign : 1;
      u8_t joystick_acc : 1;
      u8_t joystick_data : 7;
    } joy;
  };
} __attribute__ (( packed )) hid_id;

typedef struct {
  u8_t pin;
  u8_t tern_pin;
  u8_t tern_splice;
  hid_id id[APP_CONFIG_DEFS_PER_PIN];
} def_config;

#endif /* SRC_DEF_CONFIG_H_ */
