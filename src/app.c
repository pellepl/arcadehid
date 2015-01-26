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

  // gpio states
  volatile bool dirty_gpio;
  volatile bool lock_gpio_sampling;
  pin_debounce irq_debounce_map[APP_CONFIG_PINS];
  volatile bool irq_cur_pin_active[APP_CONFIG_PINS];

  app_pin_state pin_state[APP_CONFIG_PINS];
  app_pin_state pin_state_prev[APP_CONFIG_PINS];

  // keyboard states
  usb_kb_report last_usb_keyboard_report;
  volatile bool dirty_kb;

  // mouse states
  usb_mouse_report last_usb_mouse_report;

} app;

/////////////////////////////////////////////////

// keyboard handling

void app_send_kb_report(void) {
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
    if (app.pin_config[pin].tern_pin) {
      if (app.pin_state[pin] == PIN_ACTIVE_TERN) {
        def_start = app.pin_config[pin].tern_splice;
        def_end = APP_CONFIG_DEFS_PER_PIN;
      } else {
        def_start = 0;
        def_end = app.pin_config[pin].tern_splice;
      }
    } else {
      def_start = 0;
      def_end = APP_CONFIG_DEFS_PER_PIN;
    }
    DBG(D_APP, D_DEBUG, "pin %i, check defs %i--%i\n", pin+1, def_start, def_end);

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

  // send keystrokes
  USB_ARC_KB_tx(&report);

  app.dirty_kb = FALSE;
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

  if (!app.dirty_kb) {
    for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
      if (app.pin_state[pin] != app.pin_state_prev[pin]) {
        app.dirty_kb = TRUE;
        break;
      }
    }
  }

  if (app.dirty_kb && USB_ARC_KB_can_tx()) {
    DBG(D_APP, D_DEBUG, "send kb report from pins_update\n");
    app_send_kb_report();
  }

  // update app states
  memcpy(&app.pin_state_prev[0], &app.pin_state[0], sizeof(app.pin_state));

  app.dirty_gpio = FALSE;
}

///////////////////////////////// IRQ & EVENTS

static void app_kb_usb_ready_msg(u32_t ignore, void *ignore_p) {
  if (app.dirty_kb) {
    DBG(D_APP, D_DEBUG, "send kb report from kb_usb_ready\n");
    app_send_kb_report();
  }
}

static void app_mouse_usb_ready_msg(u32_t ignore, void *ignore_p) {
  app_pins_update();
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
  if (app.dirty_gpio) {
    task *t = TASK_create(app_mouse_usb_ready_msg, 0);
    ASSERT(t);
    TASK_run(t, 0, NULL);
  }
}

/////////////////////////////////// IFC

void APP_init(void) {
  //SYS_dbg_level(D_WARN);
  //SYS_dbg_mask_enable(D_ANY); // todo remove
  // common
  memset(&app, 0, sizeof(app));
  app.debounce_valid_cycles = 4;

  USB_ARC_set_kb_callback(app_kb_ready_irq);
  USB_ARC_set_mouse_callback(app_mouse_ready_irq);

}

void APP_define_pin(def_config *cfg) {
  memcpy(&app.pin_config[cfg->pin - 1], cfg, sizeof(def_config));
  app.pin_state[cfg->pin - 1] = PIN_INACTIVE;
  app.pin_state_prev[cfg->pin - 1] = PIN_INACTIVE;
  app.irq_cur_pin_active[cfg->pin - 1] = FALSE;
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

      if (app.irq_debounce_map[pin].pin_active != (app.pin_state[pin] != PIN_INACTIVE)) {
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
