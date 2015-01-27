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

volatile u8_t print_io = IOSTD;

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
  u8_t debounce_valid_cycles;
  def_config pin_config[APP_CONFIG_PINS];
  u16_t mouse_timing_divisor;

  // gpio states
  volatile bool dirty_gpio;
  volatile bool lock_gpio_sampling;
  pin_debounce irq_debounce_map[APP_CONFIG_PINS];
  volatile bool irq_cur_pin_active[APP_CONFIG_PINS];

  // app pin states
  app_pin_state pin_state[APP_CONFIG_PINS];
  app_pin_state pin_state_prev[APP_CONFIG_PINS];
  bool pin_has_mouse[APP_CONFIG_PINS];

  // keyboard states
  volatile bool dirty_kb;
  usb_kb_report kb_report_prev;

  // mouse states
  volatile bool dirty_mouse;
  u8_t butt_mask_prev;
  volatile u16_t mouse_time;

} app;

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
} mouse_state;

static bool app_check_mouse_levels(mouse_state *ms) {
  s32_t mdx = 0;
  s32_t mdy = 0;
  s32_t mdw = 0;
  u8_t butt_mask = 0;
  int pin;
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
        // todo acc
        switch (app.pin_config[pin].id[def].mouse.mouse_code) {
        case MOUSE_X:
          if (mdx == 0) mdx += sign ? -data : data;
          break;
        case MOUSE_Y:
          if (mdy == 0) mdy += sign ? -data : data;
          break;
        case MOUSE_WHEEL:
          if (mdw == 0) mdw += sign ? -data : data;
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

  if (mdx != 0 || mdy != 0 || mdw != 0 || app.butt_mask_prev != butt_mask) {
    ms->dx = mdx;
    ms->dy = mdy;
    ms->dw = mdw;
    ms->butt_mask = butt_mask;
    return TRUE;
  }

  return FALSE;
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

  USB_ARC_MOUSE_tx(&report);

  app.butt_mask_prev = ms->butt_mask;
  app.dirty_mouse = FALSE;
  enter_critical();
  app.mouse_time = 0;
  exit_critical();
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
        break;
      }
    }
  }

  if (app.dirty_kb && USB_ARC_KB_can_tx()) {
    app_send_kb_report();
  }

  // mouse check, level triggered
  mouse_state ms;
  bool dirty_mouse_level = app_check_mouse_levels(&ms);
  if (dirty_mouse_level) print("mdi\n");
  if (app.mouse_time >= app.mouse_timing_divisor) {
    bool can_tx_mouse = USB_ARC_MOUSE_can_tx();
    if ((dirty_mouse_level || app.dirty_mouse) && can_tx_mouse) {
      app_send_mouse_report(&ms);
    } else if (dirty_mouse_level && !can_tx_mouse) {
      app.dirty_mouse = TRUE;
    }
  } else {
    if (dirty_mouse_level) {
      app.dirty_mouse = TRUE;
    } else {
      enter_critical();
      app.mouse_time = app.mouse_timing_divisor;
      exit_critical();
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
  bool dirty_mouse = app_check_mouse_levels(&ms);
  if (app.mouse_time >= app.mouse_timing_divisor && (dirty_mouse || app.dirty_mouse)) {
    app_send_mouse_report(&ms);
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

/////////////////////////////////// IFC

void APP_init(void) {
  //SYS_dbg_level(D_WARN);
  //SYS_dbg_mask_enable(D_ANY); // todo remove
  // common
  memset(&app, 0, sizeof(app));
  app.debounce_valid_cycles = 4;
  app.mouse_timing_divisor = 99;

  USB_ARC_set_kb_callback(app_kb_ready_irq);
  USB_ARC_set_mouse_callback(app_mouse_ready_irq);

}

void APP_define_pin(def_config *cfg) {
  memcpy(&app.pin_config[cfg->pin - 1], cfg, sizeof(def_config));
  app.pin_state[cfg->pin - 1] = PIN_INACTIVE;
  app.pin_state_prev[cfg->pin - 1] = PIN_INACTIVE;
  app.irq_cur_pin_active[cfg->pin - 1] = FALSE;

  int def;
  app.pin_has_mouse[cfg->pin - 1] = FALSE;
  for (def = 0; def < APP_CONFIG_DEFS_PER_PIN; def++) {
    if (cfg->id[def].type == HID_ID_TYPE_MOUSE) {
      app.pin_has_mouse[cfg->pin - 1] = TRUE;
      break;
    }
  }
}

void APP_timer(void) {
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

  // mouse time divisor
  if (app.mouse_time < app.mouse_timing_divisor) {
    app.mouse_time++;
  }

  // led blink
  const gpio_pin_map *led = GPIO_MAP_get_led_map();
  if (SYS_get_time_ms() % 1000 > 0) {
    gpio_disable(led->port, led->pin);
  } else {
    gpio_enable(led->port, led->pin);
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
