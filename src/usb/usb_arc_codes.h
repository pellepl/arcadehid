/*
 * usb_arc_codes.h
 *
 *  Created on: Dec 28, 2014
 *      Author: petera
 */

#ifndef USB_ARC_CODES_H_
#define USB_ARC_CODES_H_

#include "system.h"

#define MOD_BIT(c) (1 << ((c-MOD_LCTRL) & 0x7))

#define KB_MOD_NONE           0
#define KB_MOD_LEFT_CTRL      (1<<0)
#define KB_MOD_LEFT_SHIFT     (1<<1)
#define KB_MOD_LEFT_ALT       (1<<2)
#define KB_MOD_LEFT_GUI       (1<<3)
#define KB_MOD_RIGHT_CTRL     (1<<4)
#define KB_MOD_RIGHT_SHIFT    (1<<5)
#define KB_MOD_RIGHT_ALT      (1<<6)
#define KB_MOD_RIGHT_GUI      (1<<7)

enum kb_hid_code {
    KC_NO = 0,
    KC_ROLL_OVER,
    KC_POST_FAIL,
    KC_UNDEFINED,
    KC_A,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M, // 0x10
    KC_N,
    KC_O,
    KC_P,
    KC_Q,
    KC_R,
    KC_S,
    KC_T,
    KC_U,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_1,
    KC_2,
    KC_3, // 0x20
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,
    KC_ENTER,
    KC_ESCAPE,
    KC_BSPACE,
    KC_TAB,
    KC_SPACE,
    KC_MINUS, //- _
    KC_EQUAL,
    KC_LBRACKET,
    KC_RBRACKET, // 0x30
    KC_BSLASH, // \ |
    KC_NONUS_HASH, // Non-US # ~
    KC_SCOLON, // ; :
    KC_QUOTE, // ' {.name="
    KC_GRAVE, // ~ `
    KC_COMMA, // < ,
    KC_DOT,   // > .
    KC_SLASH,  // ? /
    KC_CAPSLOCK,
    KC_F1,
    KC_F2,
    KC_F3,
    KC_F4,
    KC_F5,
    KC_F6,
    KC_F7, // 0x40
    KC_F8,
    KC_F9,
    KC_F10,
    KC_F11,
    KC_F12,
    KC_PSCREEN,
    KC_SCROLLLOCK,
    KC_PAUSE,
    KC_INSERT,
    KC_HOME,
    KC_PGUP,
    KC_DELETE,
    KC_END,
    KC_PGDOWN,
    KC_RIGHT,
    KC_LEFT, // 0x50
    KC_DOWN,
    KC_UP,
    KC_NUMLOCK,
    KC_KP_SLASH,
    KC_KP_ASTERISK,
    KC_KP_MINUS,
    KC_KP_PLUS,
    KC_KP_ENTER,
    KC_KP_1,
    KC_KP_2,
    KC_KP_3,
    KC_KP_4,
    KC_KP_5,
    KC_KP_6,
    KC_KP_7,
    KC_KP_8, // 0x60
    KC_KP_9,
    KC_KP_0,
    KC_KP_DOT,
    KC_NONUS_BSLASH, // Non-US \ |
    KC_APPLICATION,
    KC_POWER,
    KC_KP_EQUAL,
    KC_F13,
    KC_F14,
    KC_F15,
    KC_F16,
    KC_F17,
    KC_F18,
    KC_F19,
    KC_F20,
    KC_F21, // 0x70
    KC_F22,
    KC_F23,
    KC_F24,
    KC_EXECUTE,
    KC_HELP,
    KC_MENU,
    KC_SELECT,
    KC_STOP,
    KC_AGAIN,
    KC_UNDO,
    KC_CUT,
    KC_COPY,
    KC_PASTE,
    KC_FIND,
    KC__MUTE,
    KC__VOLUP, // 0x80
    KC__VOLDOWN,
    KC_LOCKING_CAPS,
    KC_LOCKING_NUM,
    KC_LOCKING_SCROLL,
    KC_KP_COMMA,
    KC_KP_EQUAL_AS400, // = on AS/400
    KC_INT1,
    KC_INT2,
    KC_INT3,
    KC_INT4,
    KC_INT5,
    KC_INT6,
    KC_INT7,
    KC_INT8,
    KC_INT9,
    KC_LANG1, // 0x90
    KC_LANG2,
    KC_LANG3,
    KC_LANG4,
    KC_LANG5,
    KC_LANG6,
    KC_LANG7,
    KC_LANG8,
    KC_LANG9,
    KC_ALT_ERASE,
    KC_SYSREQ,
    KC_CANCEL,
    KC_CLEAR,
    KC_PRIOR,
    KC_RETURN,
    KC_SEPARATOR,
    KC_OUT, // 0xa0
    KC_OPER,
    KC_CLEAR_AGAIN,
    KC_CRSEL,
    KC_EXSEL,

    KC_SYSTEM_POWER,
    KC_SYSTEM_SLEEP,
    KC_SYSTEM_WAKE,

    KC_AUDIO_MUTE,
    KC_AUDIO_VOL_UP,
    KC_AUDIO_VOL_DOWN,
    KC_MEDIA_NEXT_TRACK,
    KC_MEDIA_PREV_TRACK,
    KC_MEDIA_STOP,
    KC_MEDIA_PLAY_PAUSE,
    KC_MEDIA_SELECT,
    KC_MEDIA_EJECT, // 0xb0
    KC_MAIL,
    KC_CALCULATOR,
    KC_MY_COMPUTER,
    KC_WWW_SEARCH,
    KC_WWW_HOME,
    KC_WWW_BACK,
    KC_WWW_FORWARD,
    KC_WWW_STOP,
    KC_WWW_REFRESH,
    KC_WWW_FAVORITES,
    KC_MEDIA_FAST_FORWARD,
    KC_MEDIA_REWIND, // 0xbc

    MOD_LCTRL,
    MOD_LSHIFT,
    MOD_LALT,
    MOD_LGUI,      // 0xc0
    MOD_RCTRL,
    MOD_RSHIFT,
    MOD_RALT,
    MOD_RGUI,

    _KB_HID_CODE_MAX //0xc5
};

enum mouse_code {
  MOUSE_X = 0,
  MOUSE_Y,
  MOUSE_BUTTON1,
  MOUSE_BUTTON2,
  MOUSE_BUTTON3,
  MOUSE_WHEEL,

  _MOUSE_CODE_MAX
};

enum joystick_code {
  _JOYSTICK_IX_1 = 0,
  JOYSTICK1_X = 0,
  JOYSTICK1_Y = 1,
  JOYSTICK1_BUTTON1 = 2,
  JOYSTICK1_BUTTON2 = 3,
  JOYSTICK1_BUTTON3 = 4,
  JOYSTICK1_BUTTON4 = 5,
  JOYSTICK1_BUTTON5 = 6,
  JOYSTICK1_BUTTON6 = 7,
  JOYSTICK1_BUTTON7 = 8,
  JOYSTICK1_BUTTON8 = 9,
  JOYSTICK1_BUTTON9 = 10,
  JOYSTICK1_BUTTON10 = 11,
  JOYSTICK1_BUTTON11 = 12,
  JOYSTICK1_BUTTON12 = 13,
  JOYSTICK1_BUTTON13 = 14,
  JOYSTICK1_BUTTON14 = 15,
  _JOYSTICK_IX_2 = 16,
  JOYSTICK2_X = 16,
  JOYSTICK2_Y = 17,
  JOYSTICK2_BUTTON1 = 18,
  JOYSTICK2_BUTTON2 = 19,
  JOYSTICK2_BUTTON3 = 20,
  JOYSTICK2_BUTTON4 = 21,
  JOYSTICK2_BUTTON5 = 22,
  JOYSTICK2_BUTTON6 = 23,
  JOYSTICK2_BUTTON7 = 24,
  JOYSTICK2_BUTTON8 = 25,
  JOYSTICK2_BUTTON9 = 26,
  JOYSTICK2_BUTTON10 = 27,
  JOYSTICK2_BUTTON11 = 28,
  JOYSTICK2_BUTTON12 = 29,
  JOYSTICK2_BUTTON13 = 30,
  JOYSTICK2_BUTTON14 = 31,
};
// ugly but want to get rid of bit warning in bfield @ def_config.h
#define _JOYSTICK_CODE_MAX (JOYSTICK2_BUTTON14+1)


typedef struct {
  const char *name;
  const char *keys;
  const bool numerator;
} keymap;

const keymap *USB_ARC_get_keymap(enum kb_hid_code code);
const keymap *USB_ARC_get_mousemap(enum mouse_code code);
const keymap *USB_ARC_get_joystickmap(enum joystick_code code);

#endif /* USB_ARC_CODES_H_ */
