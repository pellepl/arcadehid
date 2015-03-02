/*
 * app.c
 *
 * Takes care of radio pairing and dispatches events further down
 * to specific instance (rover or controller)
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

// TODO redesign this

// KB (relative reporting, sticky report)
//   check keypresses on pin change
//   if a key is pressed, start timer
//     continuously check if depressed
//   handle own keypressed map - NB order is important
//   never send same keyboard report

// JOYSTICKS (relative reporting, sticky report)
//   check joystick on pin change
//   if a joystick is active, start timer
//     continuously check if depressed
//     in timer, update accelerator
//   handle own joystick pressed map
//   never send same joystick report

// MOUSE (absolute reporting, non-sticky report)
//   check mouse on pin change
//   if a mouse is active, start timer
//     continuously check if depressed
//     in timer, update accelerator
//   handle own mouse pressed map
//   do send same mouse report if necessary

typedef bool (* construct_active_map_f)(void *p);
typedef void (* construct_report_f)(void *p);

typedef struct __attribute__ (( packed )) {
  bool pin_active : 1;
  u8_t same_state : 7;
} pin_debounce;

typedef enum {
  PIN_INACTIVE = 0,
  PIN_ACTIVE,
  PIN_ACTIVE_TERN
} app_pin_state;

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
  bool pin_has_mouse[APP_CONFIG_PINS];
  u8_t pin_has_joystick[APP_CONFIG_PINS]; // bit 0 is joy1, bit 1 is joy2..

  // fs
  bool fs_mounted;

  // keyboard states
  volatile bool dirty_kb;
  usb_kb_report kb_report_prev;

  // mouse states
  task_timer mouse_timer;
  task *mouse_timer_task;
  volatile bool dirty_mouse;
  u8_t mouse_butt_mask_prev;

  u16_t acc_pos;
  u16_t acc_whe;

  // joystick states
  struct {
    task_timer timer;
    task *joystick_timer_task;
    volatile bool dirty;
    u16_t butt_mask_prev;
    u16_t acc_joy;
  } joystick[2];
} app;


typedef struct {
  hid_id_type type;
  u8_t index;
  bool pending_change;    // if there are pending changes not sent over usb yet
  task_timer timer;       // update poll timer
  task *timer_task;       // update poll task
  u16_t accelerator_1;    // current accelerator
  u16_t accelerator_2;    // current accelerator secondary
  bool report_filter;     // if same device report should be filtered away or not
  void *active_mask;     // device dependent state to synch over usb
  void *active_mask_cur; // device dependent state to synch over usb
  u32_t active_mask_len;  // length of device dependent state
  construct_active_map_f map_const;
  void *report;          // device dependent report
  void *report_prev;     // device dependent report, previous
  u32_t report_len;       // length of device report
  construct_report_f report_const;
} device_info;

typedef u8_t kb_active_mask[_KB_HID_CODE_MAX];
typedef u32_t mouse_active_mask;
typedef u32_t joy_active_mask;

static void device_start_timer(device_info *d) {
  time tim_delta;
  switch (d->type) {
  case HID_ID_TYPE_MOUSE: tim_delta = app.mouse_delta; break;
  case HID_ID_TYPE_JOYSTICK: tim_delta = app.joystick_delta; break;
  default: tim_delta = 10; break;
  }
  TASK_stop_timer(&d->timer);
  TASK_start_timer(d->timer_task, &d->timer, 0, d, tim_delta, tim_delta, "tim");
}

static void device_pincheck(device_info *d, bool *active) {
  *active = d->map_const(d->active_mask_cur);
  bool diff = arc_memcmp(d->active_mask, d->active_mask_cur, d->active_mask_len);
  d->pending_change = diff;
  memcpy(d->active_mask, d->active_mask_cur, d->active_mask_len);
}

static void device_handle_timer(device_info *d) {
  // get active map
  bool active;
  device_pincheck(d, &active);

  // update accelerators
  if (!active) {
    d->accelerator_1 = 0;
    d->accelerator_2 = 0;
  } else {
    switch (d->type) {
    case HID_ID_TYPE_MOUSE: {
      mouse_active_mask mask = *((mouse_active_mask *)d->active_mask);
      if (mask & ((1 << MOUSE_X) | (1 << MOUSE_Y))) {
        d->accelerator_1 = MIN(d->accelerator_1 + app.acc_pos_speed, 0xfff);
      } else {
        d->accelerator_1 = 0;
      }
      if (mask & (1 << MOUSE_WHEEL)) {
        d->accelerator_2 = MIN(d->accelerator_2 + app.acc_wheel_speed, 0xfff);
      } else {
        d->accelerator_2 = 0;
      }
      break;
    }
    case HID_ID_TYPE_JOYSTICK: {
      joy_active_mask mask = *((joy_active_mask *)d->active_mask);
      if ((mask & ((1 << JOYSTICK1_X) | (1 << JOYSTICK1_Y)))!=0 ||
          (mask & ((1 << JOYSTICK2_X) | (1 << JOYSTICK2_Y))) != 0) {
        d->accelerator_1 = MIN(d->accelerator_1 + app.acc_joystick_speed, 0xfff);
      } else {
        d->accelerator_1 = 0;
      }
      break;
    }
    default: break;
    }
  }

  // still have pending data needing to be sent, no update so far
  if (d->pending_change) return;

  // construct report
  d->report_const(d->report);

  if (d->report_filter) {
    // relative reporting, do not send same report twice
    if (arc_memcmp(d->report, d->report_prev, d->report_len) == 0) {
      d->pending_change = FALSE;
      return;
    } else {
      memcpy(d->report_prev, d->report, d->report_len);
    }
  } else {
    // absolute reporting, keep sending while pin is active
    d->pending_change = TRUE;
  }

  if (!active) {
    TASK_stop_timer(&d->timer);
  }
}



/////////////////////////////////////////////////

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

// keyboard handling, flank triggered

static void app_send_kb_report(void) {
  int pin;
  int report_ix = 0;
  usb_kb_report report;

  memset(&report, 0, sizeof(report));

  // for each pin..
  for (pin = 0; pin < APP_CONFIG_PINS && report_ix < USB_KB_REPORT_KEYMAP_SIZE; pin++) {
    // .. which is not inactive ..
    if (app.pin_state[pin] == PIN_INACTIVE) continue;

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
        enum kb_hid_code kb_code = app.pin_config[pin].id[def].kb.kb_code;
        if (kb_code >= MOD_LCTRL) {
          // shift, ctrl, alt or gui
          report.modifiers |= MOD_BIT(kb_code);
        } else {
          int i = 0;
          // .. and add all definitions that are not already in the report
          while (i <= report_ix && report.keymap[i++] != kb_code);
          if (i > report_ix) {
            report.keymap[report_ix] = kb_code;
            DBG(D_APP, D_DEBUG, "add kb_code %02x to report ix %i\n", kb_code, report_ix);
            report_ix++;
          } else {
            DBG(D_APP, D_DEBUG, "kb_code %02x already added to report ix %i\n", kb_code, i-1);
          }
        }
      }
    } // for each definition in pin
  } // for each pin

  // send keystrokes if report has changed since last time
  int i;
  bool diff = FALSE;
  for (i = 0; i < sizeof(usb_kb_report); i++) {
    if (report.raw[i] != app.kb_report_prev.raw[i]) {
      diff = TRUE;
      break;
    }
  }
  if (diff) {
    IF_DBG(D_APP, D_DEBUG) {
      print("K[");
      for (i = 0; i < sizeof(usb_kb_report); i++) print("%02x", report.raw[i]);
      print("]\n");
    }
    USB_ARC_KB_tx(&report);
    memcpy(&app.kb_report_prev, &report, sizeof(usb_kb_report));
  }

  app.dirty_kb = FALSE;
}

// mouse handling, level triggered

typedef struct {
  s32_t dx;
  s32_t dy;
  s32_t dw;
  u8_t butt_mask;
  bool update;
} mouse_state;

static void app_check_mouse_levels(mouse_state *ms) {
  s32_t mdx = 0;
  s32_t mdy = 0;
  s32_t mdw = 0;
  u8_t butt_mask = 0;
  int pin;
  bool pos_update = FALSE;
  bool wheel_update = FALSE;

  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    if (!app.pin_has_mouse[pin] || app.pin_state[pin] == PIN_INACTIVE)
      continue;
    int def_start, def_end;
    app_get_def_boundary(pin, &def_start, &def_end);

    int def;
    for (def = def_start; def < def_end; def++) {
      if (app.pin_config[pin].id[def].type == HID_ID_TYPE_MOUSE) {
        bool sign = app.pin_config[pin].id[def].mouse.mouse_sign;
        u8_t data = app.pin_config[pin].id[def].mouse.mouse_data;

        u8_t displacement;
        if (app.pin_config[pin].id[def].mouse.mouse_acc) {
          u16_t acc = app.pin_config[pin].id[def].mouse.mouse_code == MOUSE_WHEEL ?
              app.acc_whe : app.acc_pos;
          if (acc + data < 0xfff) {
            displacement = 1+(u8_t)(((u32_t)data * (u32_t)acc) >> 12);
            displacement = MIN(displacement, data);
          } else {
            displacement = data;
          }
        } else {
          displacement = data;
        }

        switch (app.pin_config[pin].id[def].mouse.mouse_code) {
        case MOUSE_X:
          if (mdx == 0) mdx += sign ? -displacement : displacement;
          pos_update = TRUE;
          break;
        case MOUSE_Y:
          if (mdy == 0) mdy += sign ? -displacement : displacement;
          pos_update = TRUE;
          break;
        case MOUSE_WHEEL:
          if (mdw == 0) mdw += sign ? -displacement : displacement;
          wheel_update = TRUE;
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

  if (pos_update) {
    ms->dx = mdx;
    ms->dy = mdy;

    app.acc_pos = MIN(app.acc_pos + app.acc_pos_speed, 0xfff);
  } else {
    ms->dx = 0;
    ms->dy = 0;
    app.acc_pos = 0;
  }

  if (wheel_update) {
    ms->dw = mdw;
    app.acc_whe = MIN(app.acc_whe + app.acc_wheel_speed, 0xfff);
  } else {
    ms->dw = 0;
    app.acc_whe = 0;
  }

  if (app.mouse_butt_mask_prev != butt_mask) {
    ms->butt_mask = butt_mask;
  } else {
    ms->butt_mask = app.mouse_butt_mask_prev;
  }

  if (pos_update || wheel_update || app.mouse_butt_mask_prev != butt_mask) {
    ms->update = TRUE;
  } else {
    ms->update = FALSE;
  }
}

static void app_send_mouse_report(mouse_state *ms) {
  usb_mouse_report report;

  memset(&report, 0, sizeof(report));

  if (ms->dx < -127) {
    report.dx = -127;
  } else if (ms->dx > 127) {
    report.dx = 127;
  } else {
    report.dx = ms->dx;
  }
  if (ms->dy < -127) {
    report.dy = -127;
  } else if (ms->dy > 127) {
    report.dy = 127;
  } else {
    report.dy = ms->dy;
  }
  if (ms->dw < -127) {
    report.wheel = -127;
  } else if (ms->dw > 127) {
    report.wheel = 127;
  } else {
    report.wheel = ms->dw;
  }

  report.modifiers = ms->butt_mask;

  DBG(D_APP, D_DEBUG, "M[x:%i y:%i w:%i b:%08b]\n", report.dx, report.dy, report.wheel, report.modifiers);
  USB_ARC_MOUSE_tx(&report);

  app.mouse_butt_mask_prev = ms->butt_mask;
  app.dirty_mouse = FALSE;
}

// joystick handling, level triggered

typedef struct {
  s32_t dx;
  s32_t dy;
  u16_t butt_mask;
  bool update;
} joystick_state;

static void app_check_joystick_levels(usb_joystick j_ix, joystick_state *js) {
  s32_t dx = 0;
  s32_t dy = 0;
  u16_t butt_mask = 0;
  bool dir_update = FALSE;

  int pin;

  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    if (!app.pin_has_joystick[pin] || app.pin_state[pin] == PIN_INACTIVE)
      continue;
    int def_start, def_end;
    app_get_def_boundary(pin, &def_start, &def_end);

    int def;
    for (def = def_start; def < def_end; def++) {
      if (app.pin_config[pin].id[def].type == HID_ID_TYPE_JOYSTICK) {
        usb_joystick j_def_ix = app.pin_config[pin].id[def].joy.joystick_code >= _JOYSTICK_IX_2 ? JOYSTICK2 : JOYSTICK1;
        if (j_def_ix != j_ix) continue;

        enum joystick_code mod_jcode =  app.pin_config[pin].id[def].joy.joystick_code -
            (j_def_ix == JOYSTICK2 ? _JOYSTICK_IX_2 : _JOYSTICK_IX_1);

        bool sign = app.pin_config[pin].id[def].joy.joystick_sign;
        u8_t data = app.pin_config[pin].id[def].joy.joystick_data;

        u8_t displacement;
        if (app.pin_config[pin].id[def].joy.joystick_acc) {
          u16_t acc = app.joystick[j_ix].acc_joy;
          if (acc + data < 0xfff) {
            displacement = 1+(u8_t)(((u32_t)data * (u32_t)acc) >> 12);
            displacement = MIN(displacement, data);
          } else {
            displacement = data;
          }
        } else {
          displacement = data;
        }

        switch (mod_jcode) {
        case JOYSTICK1_X:
          if (dx == 0) dx += sign ? -displacement : displacement;
          dir_update = TRUE;
          break;
        case JOYSTICK1_Y:
          if (dy == 0) dy += sign ? -displacement : displacement;
          dir_update = TRUE;
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

  if (dir_update) {
    js->dx = dx;
    js->dy = dy;
    app.joystick[j_ix].acc_joy = MIN(app.joystick[j_ix].acc_joy + app.acc_joystick_speed, 0xfff);
  } else {
    js->dx = 0;
    js->dy = 0;
    app.joystick[j_ix].acc_joy = 0;
  }

  if (app.joystick[j_ix].butt_mask_prev != butt_mask) {
    js->butt_mask = butt_mask;
  } else {
    js->butt_mask = app.joystick[j_ix].butt_mask_prev;
  }

  if (dir_update || app.joystick[j_ix].butt_mask_prev != butt_mask) {
    js->update = TRUE;
  } else {
    js->update = FALSE;
  }
}

static void app_send_joystick_report(usb_joystick j_ix, joystick_state *js) {
  usb_joystick_report report;

  memset(&report, 0, sizeof(report));

  if (js->dx < -127) {
    report.dx = -127;
  } else if (js->dx > 127) {
    report.dx = 127;
  } else {
    report.dx = js->dx;
  }
  if (js->dy < -127) {
    report.dy = -127;
  } else if (js->dy > 127) {
    report.dy = 127;
  } else {
    report.dy = js->dy;
  }
  report.buttons1 = (js->butt_mask >> 8) & 0xff;
  report.buttons2 = (js->butt_mask) & 0xff;

  DBG(D_APP, D_DEBUG, "J%i[x:%i y:%i b:%16b]\n", (j_ix+1), report.dx, report.dy, js->butt_mask);
  USB_ARC_JOYSTICK_tx(j_ix, &report);

  app.joystick[j_ix].butt_mask_prev = js->butt_mask;
  app.joystick[j_ix].dirty = FALSE;
}

// lowlevel pin handling

static void app_trigger_pin(u8_t pin, bool active) {
  DBG(D_APP, D_INFO, "pin %i %s\n", (pin+1), active ? "!":"-");
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


  // keyboard check, flank triggered
  if (!app.dirty_kb) {
    for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
      if (app.pin_state[pin] != app.pin_state_prev[pin]) {
        app.dirty_kb = TRUE;
        if (app.pin_has_joystick[pin]) {
          // TODO
        }
        break;
      }
    }
  }

  if (app.dirty_kb && USB_ARC_KB_can_tx()) {
    app_send_kb_report();
  }

  // mouse check, level triggered
  mouse_state ms;
  app_check_mouse_levels(&ms);
  if (ms.update) {
    DBG(D_APP, D_DEBUG, "M trig\n");
    TASK_stop_timer(&app.mouse_timer);
    TASK_start_timer(app.mouse_timer_task, &app.mouse_timer,
        0, NULL, app.mouse_delta, app.mouse_delta, "mtim");
  } else {
    TASK_stop_timer(&app.mouse_timer);
  }
  bool can_tx_mouse = USB_ARC_MOUSE_can_tx();
  if ((ms.update || app.dirty_mouse) && can_tx_mouse) {
    app_send_mouse_report(&ms);
  } else if (ms.update && !can_tx_mouse) {
    app.dirty_mouse = TRUE;
  }

  // joystick checks, level triggered
  usb_joystick j_ix;
  for (j_ix = JOYSTICK1; j_ix <= JOYSTICK2; j_ix++) {
    joystick_state js;
    app_check_joystick_levels(j_ix, &js);
    if (js.update) {
      DBG(D_APP, D_DEBUG, "J%i trig\n", j_ix);
      TASK_stop_timer(&app.joystick[j_ix].timer);
      TASK_start_timer(app.joystick[j_ix].joystick_timer_task, &app.joystick[j_ix].timer,
          j_ix, NULL, app.joystick_delta, app.joystick_delta, j_ix == JOYSTICK1 ? "jtim1" : "jtim2");
    } else {
      TASK_stop_timer(&app.joystick[j_ix].timer);
    }
    bool can_tx_joy = USB_ARC_JOYSTICK_can_tx(j_ix);
    if ((js.update || app.joystick[j_ix].dirty) && can_tx_joy) {
      app_send_joystick_report(j_ix, &js);
    } else if (js.update && !can_tx_joy) {
      app.joystick[j_ix].dirty = TRUE;
    }
  }

  // update app states
  memcpy(&app.pin_state_prev[0], &app.pin_state[0], sizeof(app.pin_state));

  app.dirty_gpio = FALSE;
}

///////////////////////////////// IRQ & EVENTS

static void app_kb_usb_ready_msg(u32_t ignore, void *ignore_p) {
  if (app.dirty_kb) {
    app_send_kb_report();
  }
}

static void app_mouse_usb_ready_msg(u32_t ignore, void *ignore_p) {
  mouse_state ms;
  app_check_mouse_levels(&ms);
  if (!ms.update) {
    TASK_stop_timer(&app.mouse_timer);
  }
  if (app.dirty_mouse) {
    app_send_mouse_report(&ms);
  }
}

static void app_joystick_usb_ready_msg(u32_t j, void *ignore_p) {
  usb_joystick j_ix = (usb_joystick)j;
  joystick_state js;
  app_check_joystick_levels(j_ix, &js);
  if (!js.update) {
    TASK_stop_timer(&app.joystick[j_ix].timer);
  }
  if (app.joystick[j_ix].dirty) {
    app_send_joystick_report(j_ix, &js);
  }
}

static void app_mouse_timer_msg(u32_t ignore, void *ignore_p) {
  mouse_state ms;
  app_check_mouse_levels(&ms);
  bool can_tx_mouse = USB_ARC_MOUSE_can_tx();
  if ((ms.update || app.dirty_mouse) && can_tx_mouse) {
    app_send_mouse_report(&ms);
  } else if (ms.update && !can_tx_mouse) {
    app.dirty_mouse = TRUE;
  } else if (!ms.update && !app.dirty_mouse) {
    app.acc_pos = 0;
    app.acc_whe = 0;
  }
}

static void app_joystick_timer_msg(u32_t j, void *ignore_p) {
  usb_joystick j_ix = (usb_joystick)j;
  joystick_state js;
  app_check_joystick_levels(j_ix, &js);
  bool can_tx_joy = USB_ARC_JOYSTICK_can_tx(j_ix);
  if ((js.update || app.joystick[j_ix].dirty) && can_tx_joy) {
    app_send_joystick_report(j_ix, &js);
  } else if (js.update && !can_tx_joy) {
    app.joystick[j_ix].dirty = TRUE;
  } else if (!js.update && !app.joystick[j_ix].dirty) {
    app.joystick[j_ix].acc_joy = 0;
  }
}

static void app_pins_dirty_msg(u32_t ignore, void *ignore_p) {
  app_pins_update();
}

static void app_kb_ready_irq() {
  if (app.dirty_gpio) {
    task *t = TASK_create(app_kb_usb_ready_msg, 0);
    ASSERT(t);
    TASK_run(t, 0, NULL);
  }
}

static void app_mouse_ready_irq() {
  task *t = TASK_create(app_mouse_usb_ready_msg, 0);
  ASSERT(t);
  TASK_run(t, 0, NULL);
}

static void app_joystick_ready_irq(usb_joystick j_ix) {
  task *t = TASK_create(app_joystick_usb_ready_msg, 0);
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
  // pin13 = JOYSTICK2_Y(127)
  cfg.pin = 13;
  cfg.id[0].joy.joystick_sign = 0;
  APP_cfg_set_pin(&cfg);
  // pin14 = JOYSTICK2_X(-127)
  cfg.pin = 14;
  cfg.id[0].joy.joystick_code = JOYSTICK2_X;
  cfg.id[0].joy.joystick_sign = 1;
  APP_cfg_set_pin(&cfg);
  // pin15 = JOYSTICK2_X(127)
  cfg.pin = 15;
  cfg.id[0].joy.joystick_sign = 0;
  APP_cfg_set_pin(&cfg);
  // pin16 = JOYSTICK2_BUTTON1
  cfg.pin = 16;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON1;
  APP_cfg_set_pin(&cfg);
  // pin17 = JOYSTICK2_BUTTON2
  cfg.pin = 17;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON2;
  APP_cfg_set_pin(&cfg);
  // pin18 = JOYSTICK2_BUTTON3
  cfg.pin = 18;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON3;
  APP_cfg_set_pin(&cfg);
  // pin19 = JOYSTICK2_BUTTON4
  cfg.pin = 19;
  cfg.id[0].joy.joystick_code = JOYSTICK2_BUTTON4;
  APP_cfg_set_pin(&cfg);
}

/////////////////////////////////// IFC

volatile static bool app_init = FALSE;
void APP_init(void) {
  memset(&app, 0, sizeof(app));


  app.mouse_timer_task = TASK_create(app_mouse_timer_msg, TASK_STATIC);
  app.joystick[JOYSTICK1].joystick_timer_task = TASK_create(app_joystick_timer_msg, TASK_STATIC);
  app.joystick[JOYSTICK2].joystick_timer_task = TASK_create(app_joystick_timer_msg, TASK_STATIC);

  USB_ARC_set_kb_callback(app_kb_ready_irq);
  USB_ARC_set_mouse_callback(app_mouse_ready_irq);
  USB_ARC_set_joystick_callback(app_joystick_ready_irq);

  int res = FS_mount();
  if (res == NIFFS_OK) {
    app.fs_mounted = TRUE;
  } else {
    DBG(D_APP, D_WARN, "could not mount fs- error %i\n", res);
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

  app_init = TRUE;
}

void APP_cfg_set_pin(def_config *cfg) {
  memcpy(&app.pin_config[cfg->pin - 1], cfg, sizeof(def_config));
  app.pin_state[cfg->pin - 1] = PIN_INACTIVE;
  app.pin_state_prev[cfg->pin - 1] = PIN_INACTIVE;
  app.irq_cur_pin_active[cfg->pin - 1] = FALSE;

  int def;
  app.pin_has_mouse[cfg->pin - 1] = FALSE;
  app.pin_has_joystick[cfg->pin - 1] = 0;
  for (def = 0; def < APP_CONFIG_DEFS_PER_PIN; def++) {
    if (cfg->id[def].type == HID_ID_TYPE_MOUSE) {
      app.pin_has_mouse[cfg->pin - 1] = TRUE;
      DBG(D_APP, D_DEBUG, "pin %i is a mouse pin\n", cfg->pin);
    } else if (cfg->id[def].type == HID_ID_TYPE_JOYSTICK) {
      if (cfg->id[def].joy.joystick_code < _JOYSTICK_IX_2) {
        app.pin_has_joystick[cfg->pin - 1] |= 1<<0;
      } else {
        app.pin_has_joystick[cfg->pin - 1] |= 1<<1;
      }
      DBG(D_APP, D_DEBUG, "pin %i is a joystick pin\n", cfg->pin);
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
