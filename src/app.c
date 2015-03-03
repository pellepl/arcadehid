/*
 * app.c
 *
 *  Created on: Jan 2, 2014
 *      Author: petera
 */

#include "app.h"
#include "taskq.h"
#include "miniutils.h"

#include "gpio.h"
#include "io.h"
#include "miniutils.h"
#include <stdarg.h>

#include "gpio_map.h"

#include "def_config.h"

#include "usb_arcade.h"

#include "niffs_impl.h"

volatile u8_t print_io = IOSTD;


#define DEVICES         4
#define DEV_KB          0
#define DEV_MOUSE       1
#define DEV_JOY1        2
#define DEV_JOY2        3

#define PIN_MARK_KB     (1<<DEV_KB)
#define PIN_MARK_MOUSE  (1<<DEV_MOUSE)
#define PIN_MARK_JOY1   (1<<DEV_JOY1)
#define PIN_MARK_JOY2   (1<<DEV_JOY2)

typedef struct __attribute__ (( packed )) {
  bool pin_active : 1;
  u8_t same_state : 7;
} pin_debounce;

typedef enum {
  PIN_INACTIVE = 0,
  PIN_ACTIVE,
  PIN_ACTIVE_TERN
} app_pin_state;

typedef bool (* construct_report_f)(void *device_info, void *report);

typedef struct device_info_s {
  hid_id_type type;
  u8_t index;
  bool pending_change;    // if there are pending changes not sent over usb yet
  bool timer_started;
  task_timer timer;       // update poll timer
  task *timer_task;       // update poll task
  u16_t accelerator_1;    // current accelerator
  u16_t accelerator_2;    // current accelerator secondary
  bool report_filter;     // if same device report should be filtered away or not
  void *report;          // device dependent report
  void *report_prev;     // device dependent report, previous
  u32_t report_len;       // length of device report
  construct_report_f construct_report;
} device_info;

static struct {
  // config
  def_config pin_config[APP_CONFIG_PINS];
  u8_t debounce_valid_cycles;
  time mouse_delta;
  time joystick_delta;
  u16_t acc_pos_speed;
  u16_t acc_wheel_speed;
  u16_t acc_joystick_speed;

  // gpio states
  volatile bool dirty_gpio;
  volatile bool lock_gpio_sampling;
  pin_debounce irq_debounce_map[APP_CONFIG_PINS];
  volatile bool irq_cur_pin_active[APP_CONFIG_PINS];

  // app pin states
  app_pin_state pin_state[APP_CONFIG_PINS];
  app_pin_state pin_state_prev[APP_CONFIG_PINS];
  u8_t pin_mark[APP_CONFIG_PINS];

  // fs
  bool fs_mounted;

  // devices (1 keyboard, 1 mouse, 2 joysticks)
  device_info devs[DEVICES];
  usb_kb_report kb_report;
  usb_kb_report kb_report_prev;
  usb_mouse_report mouse_report;
  usb_mouse_report mouse_report_prev;
  usb_joystick_report joystick_report1;
  usb_joystick_report joystick_report1_prev;
  usb_joystick_report joystick_report2;
  usb_joystick_report joystick_report2_prev;
} app;



static int arc_memcmp(void *a, void *b, u32_t len) {
  u8_t *pa = (u8_t *)a;
  u8_t *pb = (u8_t *)b;
  while (len--) {
    if (*pa++ != *pb++) {
      return -1;
    }
  }
  return 0;
}

///////////////////////////////// USB HID REPORT CONSTRUCTS

static void app_get_def_boundary(int pin, int *def_start, int *def_end) {
  if (app.pin_config[pin].tern_pin) {
    if (app.pin_state[pin] == PIN_ACTIVE_TERN) {
      *def_start = app.pin_config[pin].tern_splice;
      *def_end = APP_CONFIG_DEFS_PER_PIN;
    } else {
      *def_start = 0;
      *def_end = app.pin_config[pin].tern_splice;
    }
  } else {
    *def_start = 0;
    *def_end = APP_CONFIG_DEFS_PER_PIN;
  }
}

static bool kb_construct_report(void *d_v, void *r_v) {
  (void)d_v;
  usb_kb_report *r = (usb_kb_report *)r_v;
  int pin;
  int report_ix = 0;

  memset(r, 0, sizeof(usb_kb_report));
  bool active = FALSE;

  // for each pin..
  for (pin = 0; pin < APP_CONFIG_PINS && report_ix < USB_KB_REPORT_KEYMAP_SIZE; pin++) {
    // .. which has keyboard tag and is not inactive ..
    if ((app.pin_mark[pin] & PIN_MARK_KB)==0 || app.pin_state[pin] == PIN_INACTIVE) continue;

    // .. find out definitions group depending on ternary or not ..
    int def_start, def_end;
    app_get_def_boundary(pin, &def_start, &def_end);
    //DBG(D_APP, D_DEBUG, "pin %i, check defs %i--%i\n", pin+1, def_start, def_end);

    // .. and for each definition group ..
    int def;
    for (def = def_start; def < def_end; def++) {
      // .. find keyboard definitions ..
      if (report_ix >= USB_KB_REPORT_KEYMAP_SIZE) break;
      if (app.pin_config[pin].id[def].type == HID_ID_TYPE_KEYBOARD) {
        active = TRUE;
        enum kb_hid_code kb_code = app.pin_config[pin].id[def].kb.kb_code;
        if (kb_code >= MOD_LCTRL) {
          // shift, ctrl, alt or gui
          r->modifiers |= MOD_BIT(kb_code);
        } else {
          int i = 0;
          // .. and add all definitions that are not already in the report
          while (i <= report_ix && r->keymap[i++] != kb_code);
          if (i > report_ix) {
            r->keymap[report_ix] = kb_code;
            //DBG(D_APP, D_DEBUG, "add kb_code %02x to report ix %i\n", kb_code, report_ix);
            report_ix++;
          } else {
            //DBG(D_APP, D_DEBUG, "kb_code %02x already added to report ix %i\n", kb_code, i-1);
          }
        }
      }
    } // for each definition in pin
  } // for each pin
  return active;
}

static bool mouse_construct_report(void *d_v, void *r_v) {
  usb_mouse_report *r = (usb_mouse_report *)r_v;
  device_info *d = (device_info *)d_v;
  bool active = FALSE;
  s32_t mdx = 0;
  s32_t mdy = 0;
  s32_t mdw = 0;
  u8_t butt_mask = 0;
  int pin;

  memset(r, 0, sizeof(usb_mouse_report));

  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    if ((app.pin_mark[pin] & PIN_MARK_MOUSE)==0 || app.pin_state[pin] == PIN_INACTIVE)
      continue;
    int def_start, def_end;
    app_get_def_boundary(pin, &def_start, &def_end);

    int def;
    for (def = def_start; def < def_end; def++) {
      if (app.pin_config[pin].id[def].type == HID_ID_TYPE_MOUSE) {
        active = TRUE;
        bool sign = app.pin_config[pin].id[def].mouse.mouse_sign;
        u8_t data = app.pin_config[pin].id[def].mouse.mouse_data;

        u8_t displacement;
        if (app.pin_config[pin].id[def].mouse.mouse_acc) {
          u16_t acc = app.pin_config[pin].id[def].mouse.mouse_code == MOUSE_WHEEL ?
              d->accelerator_2 : d->accelerator_1;
          if (acc + data < 0xfff) {
            displacement = 1+(u8_t)(((u32_t)data * (u32_t)acc) >> 12);
            displacement = MIN(displacement, data);
          } else {
            displacement = data;
          }
        } else {
          displacement = data;
        }
        if (displacement == 0) displacement = 1;

        switch (app.pin_config[pin].id[def].mouse.mouse_code) {
        case MOUSE_X:
          if (mdx == 0) mdx += sign ? -displacement : displacement;
          break;
        case MOUSE_Y:
          if (mdy == 0) mdy += sign ? -displacement : displacement;
          break;
        case MOUSE_WHEEL:
          if (mdw == 0) mdw += sign ? -displacement : displacement;
          break;
        case MOUSE_BUTTON1:
          butt_mask |= (1<<2);
          break;
        case MOUSE_BUTTON2:
          butt_mask |= (1<<1);
          break;
        case MOUSE_BUTTON3:
          butt_mask |= (1<<0);
          break;
        default: break;
        }
      }
    }
  }

  r->dx = mdx < 0 ? MAX(-127, mdx) : MIN(127, mdx);
  r->dy = mdy < 0 ? MAX(-127, mdy) : MIN(127, mdy);
  r->wheel = mdw < 0 ? MAX(-127, mdw) : MIN(127, mdw);
  r->modifiers = butt_mask;

  return active;
}

static bool joystick_construct_report(void *d_v, void *r_v) {
  usb_joystick_report *r = (usb_joystick_report *)r_v;
  device_info *d = (device_info *)d_v;
  bool active = FALSE;
  s32_t dx = 0;
  s32_t dy = 0;
  u16_t butt_mask = 0;

  memset(r, 0, sizeof(usb_joystick_report));

  int pin;
  u8_t mark = d->index == JOYSTICK1 ? PIN_MARK_JOY1 : PIN_MARK_JOY2;

  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    if ((app.pin_mark[pin] & mark)==0 || app.pin_state[pin] == PIN_INACTIVE)
      continue;
    int def_start, def_end;
    app_get_def_boundary(pin, &def_start, &def_end);

    int def;
    for (def = def_start; def < def_end; def++) {
      if (app.pin_config[pin].id[def].type == HID_ID_TYPE_JOYSTICK) {
        u8_t j_def_ix = app.pin_config[pin].id[def].joy.joystick_code >= _JOYSTICK_IX_2 ? JOYSTICK2 : JOYSTICK1;
        if (j_def_ix != d->index) continue;

        active = TRUE;
        enum joystick_code mod_jcode =  app.pin_config[pin].id[def].joy.joystick_code -
            (j_def_ix == JOYSTICK2 ? _JOYSTICK_IX_2 : _JOYSTICK_IX_1);

        bool sign = app.pin_config[pin].id[def].joy.joystick_sign;
        u8_t data = app.pin_config[pin].id[def].joy.joystick_data;

        u8_t displacement;
        if (app.pin_config[pin].id[def].joy.joystick_acc) {
          u16_t acc = d->accelerator_1;
          if (acc + data < 0xfff) {
            displacement = 1+(u8_t)(((u32_t)data * (u32_t)acc) >> 12);
            displacement = MIN(displacement, data);
          } else {
            displacement = data;
          }
        } else {
          displacement = data;
        }
        if (displacement == 0) displacement = 1;

        switch (mod_jcode) {
        case JOYSTICK1_X:
          if (dx == 0) dx += sign ? -displacement : displacement;
          break;
        case JOYSTICK1_Y:
          if (dy == 0) dy += sign ? -displacement : displacement;
          break;
        case JOYSTICK1_BUTTON1:
        case JOYSTICK1_BUTTON2:
        case JOYSTICK1_BUTTON3:
        case JOYSTICK1_BUTTON4:
        case JOYSTICK1_BUTTON5:
        case JOYSTICK1_BUTTON6:
        case JOYSTICK1_BUTTON7:
        case JOYSTICK1_BUTTON8:
        case JOYSTICK1_BUTTON9:
        case JOYSTICK1_BUTTON10:
        case JOYSTICK1_BUTTON11:
        case JOYSTICK1_BUTTON12:
        case JOYSTICK1_BUTTON13:
        case JOYSTICK1_BUTTON14:
          butt_mask |= (1<<(mod_jcode - JOYSTICK1_BUTTON1));
          break;
        default: break;
        }
      }
    }
  }

  r->dx = dx < 0 ? MAX(-127, dx) : MIN(127, dx);
  r->dy = dy < 0 ? MAX(-127, dy) : MIN(127, dy);

  r->buttons1 = (butt_mask) & 0xff;
  r->buttons2 = (butt_mask>>8) & 0xff;

  return active;
}

///////////////////////////////// DEVICE STUFF

static void device_start_timer(device_info *d) {
  time tim_delta;
  switch (d->type) {
  case HID_ID_TYPE_MOUSE: tim_delta = app.mouse_delta; break;
  case HID_ID_TYPE_JOYSTICK: tim_delta = app.joystick_delta; break;
  default: tim_delta = 10; break;
  }
  TASK_stop_timer(&d->timer);
  TASK_start_timer(d->timer_task, &d->timer, 0, d, tim_delta, tim_delta, "tim");
  d->timer_started = TRUE;
}

static bool device_can_send(device_info *d) {
  bool can_send = FALSE;
  switch (d->type) {
  case HID_ID_TYPE_KEYBOARD:
    can_send = USB_ARC_KB_can_tx();
    break;
  case HID_ID_TYPE_MOUSE:
    can_send = USB_ARC_MOUSE_can_tx();
    break;
  case HID_ID_TYPE_JOYSTICK:
    can_send = USB_ARC_JOYSTICK_can_tx(d->index ? JOYSTICK2 : JOYSTICK1);
    break;
  default:
    ASSERT(FALSE);
    break;
  }
  return can_send;
}

static void device_send_report(device_info *d) {
  switch (d->type) {
  case HID_ID_TYPE_KEYBOARD:
    DBG(D_APP, D_DEBUG, "kb report\n");
    USB_ARC_KB_tx((usb_kb_report *)d->report);
    break;
  case HID_ID_TYPE_MOUSE:
    DBG(D_APP, D_DEBUG, "mouse report dx:%i dy:%i dw:%i mod:%08b\n",
        ((usb_mouse_report *)d->report)->dx,
        ((usb_mouse_report *)d->report)->dy,
        ((usb_mouse_report *)d->report)->wheel,
        ((usb_mouse_report *)d->report)->modifiers);
    USB_ARC_MOUSE_tx((usb_mouse_report *)d->report);
    break;
  case HID_ID_TYPE_JOYSTICK:
    DBG(D_APP, D_DEBUG, "joy report %i\n", d->index);
    USB_ARC_JOYSTICK_tx(d->index ? JOYSTICK2 : JOYSTICK1, (usb_joystick_report *)d->report);
    break;
  default:
    ASSERT(FALSE);
    break;
  }
  d->pending_change = FALSE;
  memcpy(d->report_prev, d->report, d->report_len);
}

// lowlevel pin handling

static void app_trigger_pin(u8_t pin, bool active) {
  DBG(D_APP, D_DEBUG, "pin %i %s\n", (pin+1), active ? "!":"-");
  if (active) {
    if (app.pin_config[pin].tern_pin > 0) {
      if (app.irq_cur_pin_active[app.pin_config[pin].tern_pin-1]) {
        app.pin_state[pin] = PIN_ACTIVE_TERN;
      } else {
        app.pin_state[pin] = PIN_ACTIVE;
      }
    } else {
      app.pin_state[pin] = PIN_ACTIVE;
    }
  } else {
    app.pin_state[pin] = PIN_INACTIVE;
  }
}

// app pins have changed
static void app_pins_update(void) {
  int pin;

  app.lock_gpio_sampling = TRUE;
  __DMB();

  // trigger changed pins
  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    if (app.pin_state[pin] == PIN_INACTIVE && app.irq_cur_pin_active[pin]) {
      app_trigger_pin(pin, TRUE);
    } else if (app.pin_state[pin] != PIN_INACTIVE && !app.irq_cur_pin_active[pin]) {
      app_trigger_pin(pin, FALSE);
    }
  }

  app.lock_gpio_sampling = FALSE;
  __DMB();

  int i;
  for (i = 0; i < DEVICES; i++) {
    device_info *d = &app.devs[i];
    // already have pending data needing to be sent, no update so far
    if (d->pending_change) continue;

    bool active = d->construct_report(d, d->report);
    bool can_send = device_can_send(d);
    if (active) {
      DBG(D_APP, D_DEBUG, "device %i:%i active\n", d->type, d->index);
    } else {
      DBG(D_APP, D_DEBUG, "device %i:%i inactive\n", d->type, d->index);
    }

    if (d->report_filter) {
      // relative reporting, do not send same report twice
      if (arc_memcmp(d->report, d->report_prev, d->report_len) == 0) {
        // report same as previous, do not send
        d->pending_change = FALSE;
      } else {
        // report changed, send
        if (can_send) {
          device_send_report(d);
        } else {
          DBG(D_APP, D_DEBUG, "device %i:%i pending report\n", d->type, d->index);
          d->pending_change = TRUE;
        }
      }
    } else {
      if (active) {
        // absolute reporting, keep sending while any related pin is active
        if (can_send) {
          device_send_report(d);
        } else {
          d->pending_change = TRUE;
        }
      }
    }

    if (active && !d->timer_started) {
      // device pin pressed, start polling timer
      DBG(D_APP, D_DEBUG, "start timer for device %i:%i\n", d->type, d->index);
      device_start_timer(d);
    }
  }

  // update app states
  memcpy(&app.pin_state_prev[0], &app.pin_state[0], sizeof(app.pin_state));

  app.dirty_gpio = FALSE;
}

///////////////////////////////// IRQ & EVENTS

static void app_device_timer_task(u32_t ignore, void *d_v) {
  device_info *d = (device_info *)d_v;
  // construct report
  bool active = d->construct_report(d, d->report);

  // update accelerators
  if (!active) {
    d->accelerator_1 = 0;
    d->accelerator_2 = 0;
  } else {
    switch (d->type) {
    case HID_ID_TYPE_MOUSE: {
      usb_mouse_report *r = (usb_mouse_report *)d->report;
      if (r->dx != 0 || r->dy != 0) {
        d->accelerator_1 = MIN(d->accelerator_1 + app.acc_pos_speed, 0xfff);
      } else {
        d->accelerator_1 = 0;
      }
      if (r->wheel != 0) {
        d->accelerator_2 = MIN(d->accelerator_2 + app.acc_wheel_speed, 0xfff);
      } else {
        d->accelerator_2 = 0;
      }
      break;
    }
    case HID_ID_TYPE_JOYSTICK: {
      usb_joystick_report *r = (usb_joystick_report *)d->report;
      if (r->dx != 0 || r->dy != 0) {
        d->accelerator_1 = MIN(d->accelerator_1 + app.acc_joystick_speed, 0xfff);
      } else {
        d->accelerator_1 = 0;
      }
      break;
    }
    default: break;
    }
  }

  bool can_send = device_can_send(d);

  if (d->report_filter) {
    // relative reporting, do not send same report twice
    if (arc_memcmp(d->report, d->report_prev, d->report_len) == 0) {
      // report same as previous, do not send
      d->pending_change = FALSE;
    } else {
      // report changed, send
      if (can_send) {
        device_send_report(d);
      } else {
        d->pending_change = TRUE;
      }
    }
  } else {
    // absolute reporting, keep sending while any related pin is active
    if (active) {
      if (can_send) {
        device_send_report(d);
      } else {
        d->pending_change = TRUE;
      }
    }
  }

  if (!active) {
    // no pins pressed, so stop polling this device
    TASK_stop_timer(&d->timer);
    DBG(D_APP, D_DEBUG, "stop timer for device %i:%i\n", d->type, d->index);
    d->timer_started = FALSE;
  }
}

static void app_kb_usb_cts_msg(u32_t ignore, void *ignore_p) {
  device_info *d = &app.devs[DEV_KB];
  if (d->pending_change) {
    device_send_report(d);
  }
  DBG(D_APP, D_DEBUG, "device %i:%i cts\n", d->type, d->index);
}

static void app_mouse_usb_cts_msg(u32_t ignore, void *ignore_p) {
  device_info *d = &app.devs[DEV_MOUSE];
  if (d->pending_change) {
    device_send_report(d);
  }
  DBG(D_APP, D_DEBUG, "device %i:%i cts\n", d->type, d->index);
}

static void app_joystick_usb_cts_msg(u32_t j, void *ignore_p) {
  usb_joystick j_ix = j ? JOYSTICK2 : JOYSTICK1;
  device_info *d = &app.devs[j_ix == JOYSTICK1 ? DEV_JOY1 : DEV_JOY2];
  if (d->pending_change) {
    device_send_report(d);
  }
  DBG(D_APP, D_DEBUG, "device %i:%i cts\n", d->type, d->index);
}

static void app_pins_dirty_msg(u32_t ignore, void *ignore_p) {
  app_pins_update();
}

static void app_kb_usb_cts_irq() {
  task *t = TASK_create(app_kb_usb_cts_msg, 0);
  ASSERT(t);
  TASK_run(t, 0, NULL);
}

static void app_mouse_usb_cts_irq() {
  task *t = TASK_create(app_mouse_usb_cts_msg, 0);
  ASSERT(t);
  TASK_run(t, 0, NULL);
}

static void app_joystick_usb_cts_irq(usb_joystick j_ix) {
  task *t = TASK_create(app_joystick_usb_cts_msg, 0);
  ASSERT(t);
  TASK_run(t, j_ix, NULL);
}

/////////////////////////////////// DEF CFG

static void app_config_default(void) {
  // default config
  app.debounce_valid_cycles = 8;
  app.mouse_delta = 7;
  app.acc_pos_speed = 4;
  app.acc_wheel_speed = 4;
  app.joystick_delta = 7;
  app.acc_joystick_speed = 4;
  memset(&app.pin_config, 0x00, sizeof(app.pin_config));

  def_config cfg;
  memset(&cfg, 0x00, sizeof(def_config));

  // pin1 = JOYSTICK1_Y(-127) UP
  cfg.pin = 1;
  cfg.id[0].type = HID_ID_TYPE_JOYSTICK;
  cfg.id[0].joy.joystick_code = JOYSTICK1_Y;
  cfg.id[0].joy.joystick_sign = 1;
  cfg.id[0].joy.joystick_data = 127;
  cfg.id[1].type = HID_ID_TYPE_KEYBOARD;
  cfg.id[1].kb.kb_code  = KC_UP;
  APP_cfg_set_pin(&cfg);
  // pin2 = JOYSTICK1_Y(127) DOWN
  cfg.pin = 2;
  cfg.id[0].joy.joystick_sign = 0;
  cfg.id[1].kb.kb_code  = KC_DOWN;
  APP_cfg_set_pin(&cfg);
  // pin3 = JOYSTICK1_X(-127) LEFT
  cfg.pin = 3;
  cfg.id[0].joy.joystick_code = JOYSTICK1_X;
  cfg.id[0].joy.joystick_sign = 1;
  cfg.id[1].kb.kb_code  = KC_LEFT;
  APP_cfg_set_pin(&cfg);
  // pin4 = JOYSTICK1_X(127) RIGHT
  cfg.pin = 4;
  cfg.id[0].joy.joystick_sign = 0;
  cfg.id[1].kb.kb_code  = KC_RIGHT;
  APP_cfg_set_pin(&cfg);
  cfg.id[1].type = HID_ID_TYPE_NONE;
  // pin4 = JOYSTICK1_BUTTON1
  cfg.pin = 5;
  cfg.id[0].joy.joystick_code = JOYSTICK1_BUTTON1;
  APP_cfg_set_pin(&cfg);
  // pin5 = JOYSTICK1_BUTTON2
  cfg.pin = 6;
  cfg.id[0].joy.joystick_code = JOYSTICK1_BUTTON2;
  APP_cfg_set_pin(&cfg);
  // pin6 = JOYSTICK1_BUTTON3
  cfg.pin = 7;
  cfg.id[0].joy.joystick_code = JOYSTICK1_BUTTON3;
  APP_cfg_set_pin(&cfg);
  // pin7 = JOYSTICK1_BUTTON4
  cfg.pin = 8;
  cfg.id[0].joy.joystick_code = JOYSTICK1_BUTTON4;
  APP_cfg_set_pin(&cfg);

  cfg.id[0].joy.joystick_code = JOYSTICK2_Y;
  cfg.id[0].joy.joystick_sign = 1;
  cfg.id[0].joy.joystick_data = 127;
  APP_cfg_set_pin(&cfg);
  // pin14 = JOYSTICK2_Y(127)
  cfg.pin = 14;
  cfg.id[0].joy.joystick_sign = 0;
  APP_cfg_set_pin(&cfg);
  // pin15 = JOYSTICK2_X(-127)
  cfg.pin = 15;
  cfg.id[0].joy.joystick_code = JOYSTICK2_X;
  cfg.id[0].joy.joystick_sign = 1;
  APP_cfg_set_pin(&cfg);
  // pin16 = JOYSTICK2_X(127)
  cfg.pin = 16;
  cfg.id[0].joy.joystick_sign = 0;
  APP_cfg_set_pin(&cfg);
  // pin17 = JOYSTICK2_BUTTON1
  cfg.pin = 17;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON1;
  APP_cfg_set_pin(&cfg);
  // pin18 = JOYSTICK2_BUTTON2
  cfg.pin = 18;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON2;
  APP_cfg_set_pin(&cfg);
  // pin19 = JOYSTICK2_BUTTON3
  cfg.pin = 19;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON3;
  APP_cfg_set_pin(&cfg);
  // pin20 = JOYSTICK2_BUTTON4
  cfg.pin = 20;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON4;
  APP_cfg_set_pin(&cfg);
}

/////////////////////////////////// IFC

volatile static bool app_init = FALSE;
void APP_init(void) {
  memset(&app, 0, sizeof(app));


  USB_ARC_set_kb_callback(app_kb_usb_cts_irq);
  USB_ARC_set_mouse_callback(app_mouse_usb_cts_irq);
  USB_ARC_set_joystick_callback(app_joystick_usb_cts_irq);

  int res = FS_mount();
  if (res == NIFFS_OK) {
    app.fs_mounted = TRUE;
  } else {
    DBG(D_APP, D_WARN, "could not mount fs - error %i\n", res);
  }

  if (app.fs_mounted) {
    res = FS_load_config("default");
    if (res == ERR_NIFFS_FILE_NOT_FOUND) {
      app_config_default();
      DBG(D_APP, D_INFO, "no default config found, saving factory default");
      res = FS_save_config("default");
    }

    if (res != NIFFS_OK) {
      DBG(D_APP, D_WARN, "fs error %i\n", res);
    }
  }

  // setup devices

  // keyboard device
  app.devs[DEV_KB].type = HID_ID_TYPE_KEYBOARD;
  app.devs[DEV_KB].index = 0;
  app.devs[DEV_KB].construct_report = kb_construct_report;
  app.devs[DEV_KB].report = &app.kb_report;
  app.devs[DEV_KB].report_prev = &app.kb_report_prev;
  app.devs[DEV_KB].report_len = sizeof(app.kb_report);
  app.devs[DEV_KB].report_filter = TRUE;
  app.devs[DEV_KB].timer_task = TASK_create(app_device_timer_task, TASK_STATIC);
  // mouse device
  app.devs[DEV_MOUSE].type = HID_ID_TYPE_MOUSE;
  app.devs[DEV_MOUSE].index = 0;
  app.devs[DEV_MOUSE].construct_report = mouse_construct_report;
  app.devs[DEV_MOUSE].report = &app.mouse_report;
  app.devs[DEV_MOUSE].report_prev = &app.mouse_report_prev;
  app.devs[DEV_MOUSE].report_len = sizeof(app.mouse_report);
  app.devs[DEV_MOUSE].report_filter = FALSE;
  app.devs[DEV_MOUSE].timer_task = TASK_create(app_device_timer_task, TASK_STATIC);
  // joystick1 device
  app.devs[DEV_JOY1].type = HID_ID_TYPE_JOYSTICK;
  app.devs[DEV_JOY1].index = (u8_t)JOYSTICK1;
  app.devs[DEV_JOY1].construct_report = joystick_construct_report;
  app.devs[DEV_JOY1].report = &app.joystick_report1;
  app.devs[DEV_JOY1].report_prev = &app.joystick_report1_prev;
  app.devs[DEV_JOY1].report_len = sizeof(app.joystick_report1);
  app.devs[DEV_JOY1].report_filter = TRUE;
  app.devs[DEV_JOY1].timer_task = TASK_create(app_device_timer_task, TASK_STATIC);
  // joystick2 device
  app.devs[DEV_JOY2].type = HID_ID_TYPE_JOYSTICK;
  app.devs[DEV_JOY2].index = (u8_t)JOYSTICK2;
  app.devs[DEV_JOY2].construct_report = joystick_construct_report;
  app.devs[DEV_JOY2].report = &app.joystick_report2;
  app.devs[DEV_JOY2].report_prev = &app.joystick_report2_prev;
  app.devs[DEV_JOY2].report_len = sizeof(app.joystick_report2);
  app.devs[DEV_JOY2].report_filter = TRUE;
  app.devs[DEV_JOY2].timer_task = TASK_create(app_device_timer_task, TASK_STATIC);

  app_init = TRUE;
}

void APP_cfg_set_pin(def_config *cfg) {
  memcpy(&app.pin_config[cfg->pin - 1], cfg, sizeof(def_config));
  app.pin_state[cfg->pin - 1] = PIN_INACTIVE;
  app.pin_state_prev[cfg->pin - 1] = PIN_INACTIVE;
  app.irq_cur_pin_active[cfg->pin - 1] = FALSE;

  int def;
  app.pin_mark[cfg->pin - 1] = 0;
  for (def = 0; def < APP_CONFIG_DEFS_PER_PIN; def++) {
    if (cfg->id[def].type == HID_ID_TYPE_KEYBOARD) {
      app.pin_mark[cfg->pin - 1] |= PIN_MARK_KB;
    } else if (cfg->id[def].type == HID_ID_TYPE_MOUSE) {
      app.pin_mark[cfg->pin - 1] |= PIN_MARK_MOUSE;
    } else if (cfg->id[def].type == HID_ID_TYPE_JOYSTICK) {
      if (cfg->id[def].joy.joystick_code < _JOYSTICK_IX_2) {
        app.pin_mark[cfg->pin - 1] |= PIN_MARK_JOY1;
      } else {
        app.pin_mark[cfg->pin - 1] |= PIN_MARK_JOY2;
      }
    }
  }
}
def_config *APP_cfg_get_pin(u8_t pin) {
  return &app.pin_config[pin];
}
void APP_cfg_set_debounce_cycles(u8_t cycles) {
  app.debounce_valid_cycles = cycles;
}
u8_t APP_cfg_get_debounce_cycles(void) {
  return app.debounce_valid_cycles;
}
void APP_cfg_set_mouse_delta_ms(time ms) {
  app.mouse_delta = ms;
}
time APP_cfg_get_mouse_delta_ms(void) {
  return app.mouse_delta;
}
void APP_cfg_set_acc_pos_speed(u16_t speed) {
  app.acc_pos_speed = speed;
}
u16_t APP_cfg_get_acc_pos_speed(void) {
  return app.acc_pos_speed;
}
void APP_cfg_set_acc_wheel_speed(u16_t speed) {
  app.acc_wheel_speed = speed;
}
u16_t APP_cfg_get_acc_wheel_speed(void) {
  return app.acc_wheel_speed;
}
void APP_cfg_set_joystick_delta_ms(time ms) {
  app.joystick_delta = ms;
}
time APP_cfg_get_joystick_delta_ms(void) {
  return app.joystick_delta;
}
void APP_cfg_set_joystick_acc_speed(u16_t speed) {
  app.acc_joystick_speed = speed;
}
u16_t APP_cfg_get_joystick_acc_speed(void) {
  return app.acc_joystick_speed;
}

void APP_timer(void) {
  if (app_init) {
    // input read
    if (!app.lock_gpio_sampling) {
      int pin;
      const gpio_pin_map *map = GPIO_MAP_get_pin_map();

      // debouncer
      bool any_changes = FALSE;
      for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
        bool pin_active = gpio_get(map[pin].port, map[pin].pin) == 0;

        if (pin_active == app.irq_debounce_map[pin].pin_active) {
          if (app.irq_debounce_map[pin].same_state < app.debounce_valid_cycles) {
            app.irq_debounce_map[pin].same_state++;
          } else {
            if (app.irq_cur_pin_active[pin] != pin_active) {
              // pin same state given nbr of cycles, now triggered
              app.irq_cur_pin_active[pin] = pin_active;
            }
          }
        } else {
          app.irq_debounce_map[pin].pin_active = pin_active;
          app.irq_debounce_map[pin].same_state = 0;
        }

        if (app.irq_cur_pin_active[pin] != (app.pin_state[pin] != PIN_INACTIVE)) {
          any_changes = TRUE;
          //print("pin %i trig\n", pin);
        }
      }

      // post change
      if (!app.dirty_gpio && any_changes) {
        app.dirty_gpio = TRUE;
        task *t = TASK_create(app_pins_dirty_msg, 0);
        ASSERT(t);
        TASK_run(t, 0, NULL);
      }
    }
  }

  // led blink
  const gpio_pin_map *led = GPIO_MAP_get_led_map();
  if (SYS_get_time_ms() % 1000 > 0) {
#ifdef CONFIG_HY_TEST_BOARD
    gpio_disable(led->port, led->pin);
#else
    gpio_enable(led->port, led->pin);
#endif
  } else {
#ifdef CONFIG_HY_TEST_BOARD
    gpio_enable(led->port, led->pin);
#else
    gpio_disable(led->port, led->pin);
#endif
  }
}

// redirected printing

void set_print_output(u8_t io) {
  print_io = io;
}

u8_t get_print_output(void) {
  return print_io;
}

void arcprint(const char* f, ...) {
  va_list arg_p;
  va_start(arg_p, f);
  v_printf(print_io, f, arg_p);
  va_end(arg_p);
}
