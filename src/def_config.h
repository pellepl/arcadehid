/*
 * key_config.h
 *
 *  Created on: Jan 18, 2015
 *      Author: petera
 */

#ifndef SRC_DEF_CONFIG_H_
#define SRC_DEF_CONFIG_H_

#include "system.h"
#include "usb_kb_codes.h"

typedef enum {
  HID_ID_TYPE_NONE = 0,
  HID_ID_TYPE_KEYBOARD,
  HID_ID_TYPE_MOUSE,
  HID_ID_TYPE_RESERVED,
} hid_id_type;

typedef enum {
  HID_ID_MOUSE_X = 0,
  HID_ID_MOUSE_Y,
  HID_ID_MOUSE_BUTTONS,
  HID_ID_MOUSE_WHEEL,
} hid_id_mouse;

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
      hid_id_mouse mouse_type : 2;
      u8_t mouse_sign : 1;
      u8_t mouse_acc : 1;
      u8_t mouse_res : 2;
      u8_t mouse_data : 8;
    } mouse;
  };
} __attribute__ (( packed )) hid_id;

typedef struct {
  u8_t pin;
  u8_t tern_pin;
  u8_t tern_splice;
  hid_id id[APP_CONFIG_DEFS_PER_PIN];
} def_config;

#endif /* SRC_DEF_CONFIG_H_ */
