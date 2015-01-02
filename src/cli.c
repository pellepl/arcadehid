/*
 * cli.c
 *
 *  Created on: Jul 24, 2012
 *      Author: petera
 */

#include "cli.h"
#include "uart_driver.h"
#include "taskq.h"
#include "miniutils.h"
#include "system.h"

#include "app.h"

#include "gpio.h"

#include "usb/usb_arcade.h"

#define CLI_PROMPT "> "
#define IS_STRING(s) ((u8_t*)(s) >= (u8_t*)in && (u8_t*)(s) < (u8_t*)in + sizeof(in))

typedef int (*func)(int a, ...);

typedef struct cmd_s {
  const char* name;
  const func fn;
  const char* help;
} cmd;

struct {
  uart_rx_callback prev_uart_rx_f;
  void *prev_uart_arg;
} cli_state;

static u8_t in[256];

static int _argc;
static void *_args[16];

static int f_usb_init(void);
static int f_usb_send(int c);
static int f_usb_test(void);
static int f_usb_test2(void);

static int f_uwrite(int uart, char* data);
static int f_uread(int uart, int numchars);
static int f_uconf(int uart, int speed);

static int f_rand();

static int f_reset();
static int f_time(int d, int h, int m, int s, int ms);
static int f_help(char *s);
static int f_dump();
static int f_dump_trace();
static int f_assert();
static int f_dbg();
static int f_build();

static int f_memfind(int hex);

static void cli_print_app_name(void);

/////////////////////////////////////////////////////////////////////////////////////////////

static cmd c_tbl[] = {
    { .name = "usb_init", .fn = (func) f_usb_init,
        .help = "Initializes usb\n"
    },
    { .name = "usb_send", .fn = (func) f_usb_send,
        .help = "Send over usb\n"
    },
    { .name = "usb_test", .fn = (func) f_usb_test,
        .help = "Send over usb\n"
    },
    { .name = "usb_test2", .fn = (func) f_usb_test2,
        .help = "Send over usb 2\n"
    },

    { .name = "dump", .fn = (func) f_dump,
        .help = "Dumps state of all system\n"
    },
    { .name = "dump_trace", .fn = (func) f_dump_trace, .help = "Dumps system trace\n"
    },
    { .name = "time", .fn = (func) f_time,
        .help = "Prints or sets time\n"
        "time or time <day> <hour> <minute> <second> <millisecond>\n"
    },
    { .name = "uwrite", .fn = (func) f_uwrite,
        .help = "Writes to uart\n"
        "uwrite <uart> <string>\n"
            "ex: uwrite 2 \"foo\"\n"
    },
    { .name = "uread", .fn = (func) f_uread,
        .help = "Reads from uart\n"
        "uread <uart> (<numchars>)\n"
            "numchars - number of chars to read, if omitted uart is drained\n"
            "ex: uread 2 10\n"
    },
    { .name = "uconf", .fn = (func) f_uconf,
        .help = "Configure uart\n"
        "uconf <uart> <speed>\n"
            "ex: uconf 2 9600\n"
    },


    { .name = "dbg", .fn = (func) f_dbg,
        .help = "Set debug filter and level\n"
        "dbg (level <dbg|info|warn|fatal>) (enable [x]*) (disable [x]*)\n"
        "x - <task|heap|comm|cnc|cli|nvs|spi|all>\n"
        "ex: dbg level info disable all enable cnc comm\n"
    },
    {.name = "memfind", .fn = (func) f_memfind,
        .help = "Searches for hex in memory\n"
            "memfind 0xnnnnnnnn\n"
    },
    { .name = "assert", .fn = (func) f_assert,
        .help = "Asserts system\n"
            "NOTE system will need to be rebooted\n"
    },
    { .name = "rand", .fn = (func) f_rand,
        .help = "Generates pseudo random sequence\n"
    },
    { .name = "reset", .fn = (func) f_reset,
        .help = "Resets system\n"
    },
    { .name = "build", .fn = (func) f_build,
        .help = "Outputs build info\n"
    },
    { .name = "help", .fn = (func) f_help,
        .help = "Prints help\n"
        "help or help <command>\n"
    },
    { .name = "?", .fn = (func) f_help,
        .help = "Prints help\n"
        "help or help <command>\n" },

    // menu end marker
    { .name = NULL, .fn = (func) 0, .help = NULL },
  };

/////////////////////////////////////////////////////////////////////////////////////////////

static int f_usb_init(void) {
  USB_ARC_init();
  return 0;
}

static int f_usb_send(int b) {
  usb_kb_report r;
  memset(&r, 0, sizeof(r));
  r.modifiers = 0;
  r.keymap[0] = b;
  USB_ARC_KB_tx(&r);
  return 0;
}

static int f_usb_test(void) {
  usb_kb_report r;
  int k = USB_KB_REPORT_KEYMAP_SIZE;
  r.modifiers = 0;
  int i;
  for (i = 0; i < k; i++)
    r.keymap[i] = i+4;
  USB_ARC_KB_tx(&r);
  for (i = 0; i < k; i++)
    r.keymap[i] = 0;
  USB_ARC_KB_tx(&r);
  return 0;
}
static int f_usb_test2(void) {
  usb_mouse_report r;
  r.modifiers = 0;
  r.dx = -8;
  r.dy = 8;
  r.wheel = 0;
  USB_ARC_MOUSE_tx(&r);
  return 0;
}

static int f_rand() {
  print("%08x\n", rand_next());
  return 0;
}

static int f_reset() {
  SYS_reboot(REBOOT_USER);
  return 0;
}

static int f_time(int ad, int ah, int am, int as, int ams) {
  if (_argc == 0) {
    u16_t d, ms;
    u8_t h, m, s;
    SYS_get_time(&d, &h, &m, &s, &ms);
    print("day:%i time:%02i:%02i:%02i.%03i\n", d, h, m, s, ms);
  } else if (_argc == 5) {
    SYS_set_time(ad, ah, am, as, ams);
  } else {
    return -1;
  }
  return 0;
}

#ifdef DBG_OFF
static int f_dbg() {
  print("Debug disabled compile-time\n");
  return 0;
}
#else
const char* DBG_BIT_NAME[] = _DBG_BIT_NAMES;

static void print_debug_setting() {
  print("DBG level: %i\n", SYS_dbg_get_level());
  int d;
  for (d = 0; d < sizeof(DBG_BIT_NAME) / sizeof(const char*); d++) {
    print("DBG mask %s: %s\n", DBG_BIT_NAME[d],
        SYS_dbg_get_mask() & (1 << d) ? "ON" : "OFF");
  }
}

static int f_dbg() {
  enum state {
    NONE, LEVEL, ENABLE, DISABLE
  } st = NONE;
  int a;
  if (_argc == 0) {
    print_debug_setting();
    return 0;
  }
  for (a = 0; a < _argc; a++) {
    u32_t f = 0;
    char *s = (char*) _args[a];
    if (!IS_STRING(s)) {
      return -1;
    }
    if (strcmp("level", s) == 0) {
      st = LEVEL;
    } else if (strcmp("enable", s) == 0) {
      st = ENABLE;
    } else if (strcmp("disable", s) == 0) {
      st = DISABLE;
    } else {
      switch (st) {
      case LEVEL:
        if (strcmp("dbg", s) == 0) {
          SYS_dbg_level(D_DEBUG);
        } else if (strcmp("info", s) == 0) {
          SYS_dbg_level(D_INFO);
        } else if (strcmp("warn", s) == 0) {
          SYS_dbg_level(D_WARN);
        } else if (strcmp("fatal", s) == 0) {
          SYS_dbg_level(D_FATAL);
        } else {
          return -1;
        }
        break;
      case ENABLE:
      case DISABLE: {
        int d;
        for (d = 0; f == 0 && d < sizeof(DBG_BIT_NAME) / sizeof(const char*);
            d++) {
          if (strcmp(DBG_BIT_NAME[d], s) == 0) {
            f = (1 << d);
          }
        }
        if (strcmp("all", s) == 0) {
          f = D_ANY;
        }
        if (f == 0) {
          return -1;
        }
        if (st == ENABLE) {
          SYS_dbg_mask_enable(f);
        } else {
          SYS_dbg_mask_disable(f);
        }
        break;
      }
      default:
        return -1;
      }
    }
  }
  print_debug_setting();
  return 0;
}
#endif

static int f_assert() {
  ASSERT(FALSE);
  return 0;
}

static int f_build(void) {
  cli_print_app_name();
  print("\n");

  print("SYS_MAIN_TIMER_FREQ %i\n", SYS_MAIN_TIMER_FREQ);
  print("SYS_TIMER_TICK_FREQ %i\n", SYS_TIMER_TICK_FREQ);
  print("UART2_SPEED %i\n", UART2_SPEED);
  print("CONFIG_TASK_POOL %i\n", CONFIG_TASK_POOL);

  return 0;
}

static int f_uwrite(int uart, char* data) {
  if (_argc != 2 || !IS_STRING(data)) {
    return -1;
  }
  if (uart < 0 || uart >= CONFIG_UART_CNT) {
    return -1;
  }
  char c;
  while ((c = *data++) != 0) {
    UART_put_char(_UART(uart), c);
  }
  return 0;
}

static int f_uread(int uart, int numchars) {
  if (_argc < 1 || _argc > 2) {
    return -1;
  }
  if (uart < 0 || uart >= CONFIG_UART_CNT) {
    return -1;
  }
  if (_argc == 1) {
    numchars = 0x7fffffff;
  }
  int l = UART_rx_available(_UART(uart));
  l = MIN(l, numchars);
  int ix = 0;
  while (ix++ < l) {
    print("%c", UART_get_char(_UART(uart)));
  }
  print("\n%i bytes read\n", l);
  return 0;
}

static int f_uconf(int uart, int speed) {
  if (_argc != 2) {
    return -1;
  }
  if (IS_STRING(uart) || IS_STRING(speed) || uart < 0 || uart >= CONFIG_UART_CNT) {
    return -1;
  }
  UART_config(_UART(uart), speed,
      UART_DATABITS_8, UART_STOPBITS_1, UART_PARITY_NONE, UART_FLOWCONTROL_NONE, TRUE);

  return 0;
}

static int f_help(char *s) {
  if (IS_STRING(s)) {
    int i = 0;
    while (c_tbl[i].name != NULL ) {
      if (strcmp(s, c_tbl[i].name) == 0) {
        print("%s\t%s", c_tbl[i].name, c_tbl[i].help);
        return 0;
      }
      i++;
    }
    print("%s\tno such command\n", s);
  } else {
    print ("  ");
    cli_print_app_name();
    print("\n");
    int i = 0;
    while (c_tbl[i].name != NULL ) {
      int len = strpbrk(c_tbl[i].help, "\n") - c_tbl[i].help;
      char tmp[64];
      strncpy(tmp, c_tbl[i].help, len + 1);
      tmp[len + 1] = 0;
      char fill[24];
      int fill_len = sizeof(fill) - strlen(c_tbl[i].name);
      memset(fill, ' ', sizeof(fill));
      fill[fill_len] = 0;
      print("  %s%s%s", c_tbl[i].name, fill, tmp);
      i++;
    }
  }
  return 0;
}

static int f_dump() {
  return 0;
}

static int f_dump_trace() {
#ifdef DBG_TRACE_MON
  SYS_dump_trace(IOSTD);
#else
  print("trace not enabled\n");
#endif
  return 0;
}

static int f_memfind(int hex) {
  u8_t *addr = (u8_t*)SRAM_BASE;
  int i;
  print("finding 0x%08x...\n", hex);
  for (i = 0; i < 20*1024 - 4; i++) {
    u32_t m =
        (addr[i]) |
        (addr[i+1]<<8) |
        (addr[i+2]<<16) |
        (addr[i+3]<<24);
    u32_t rm =
        (addr[i+3]) |
        (addr[i+2]<<8) |
        (addr[i+1]<<16) |
        (addr[i]<<24);
    if (m == hex) {
      print("match found @ 0x%08x\n", i + addr);
    }
    if (rm == hex) {
      print("reverse match found @ 0x%08x\n", i + addr);
    }
  }
  print("finished\n");
  return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////

void CLI_TASK_on_input(u32_t len, void *p) {
  if (len > sizeof(in)) {
    DBG(D_CLI, D_WARN, "CONS input overflow\n");
    print(CLI_PROMPT);
    return;
  }
  u32_t rlen = UART_get_buf(_UART(UARTSTDIN), in, MIN(len, sizeof(in)));
  if (rlen != len) {
    DBG(D_CLI, D_WARN, "CONS length mismatch\n");
    print(CLI_PROMPT);
    return;
  }
  cursor cursor;
  strarg_init(&cursor, (char*) in, rlen);
  strarg arg;
  _argc = 0;
  func fn = NULL;
  int ix = 0;

  // parse command and args
  while (strarg_next(&cursor, &arg)) {
    if (arg.type == INT) {
      //DBG(D_CLI, D_DEBUG, "CONS arg %i:\tlen:%i\tint:%i\n",arg_c, arg.len, arg.val);
    } else if (arg.type == STR) {
      //DBG(D_CLI, D_DEBUG, "CONS arg %i:\tlen:%i\tstr:\"%s\"\n", arg_c, arg.len, arg.str);
    }
    if (_argc == 0) {
      // first argument, look for command function
      if (arg.type != STR) {
        break;
      } else {
        while (c_tbl[ix].name != NULL ) {
          if (strcmp(arg.str, c_tbl[ix].name) == 0) {
            fn = c_tbl[ix].fn;
            break;
          }
          ix++;
        }
        if (fn == NULL ) {
          break;
        }
      }
    } else {
      // succeeding arguments¸ store them in global vector
      if (_argc - 1 >= 16) {
        DBG(D_CLI, D_WARN, "CONS too many args\n");
        fn = NULL;
        break;
      }
      _args[_argc - 1] = (void*) arg.val;
    }
    _argc++;
  }

  // execute command
  if (fn) {
    _argc--;
    DBG(D_CLI, D_DEBUG, "CONS calling [%p] with %i args\n", fn, _argc);
    int res = (int) _variadic_call(fn, _argc, _args);
    if (res == -1) {
      print("%s", c_tbl[ix].help);
    } else {
      print("OK\n");
    }
  } else {
    print("unknown command - try help\n");
  }
  print(CLI_PROMPT);
}

void CLI_timer() {
}

void CLI_uart_check_char(void *a, u8_t c) {
  if (c == '\n' || c == '\r') {
    task *t = TASK_create(CLI_TASK_on_input, 0);
    TASK_run(t, UART_rx_available(_UART(UARTSTDIN)), NULL);
  }
}

DBG_ATTRIBUTE static u32_t __dbg_magic;

void CLI_init() {
  if (__dbg_magic != 0x43215678) {
    __dbg_magic = 0x43215678;
    SYS_dbg_level(D_WARN);
    SYS_dbg_mask_set(0);
  }
  memset(&cli_state, 0, sizeof(cli_state));
  DBG(D_CLI, D_DEBUG, "CLI init\n");
  UART_set_callback(_UART(UARTSTDIN), CLI_uart_check_char, NULL);
  print ("\n");
  cli_print_app_name();
  print("\n\n");
  print("build     : %i\n", SYS_build_number());
  print("build date: %i\n", SYS_build_date());
  print("\ntype '?' or 'help' for list of commands\n\n");
  print(CLI_PROMPT);
}

static void cli_print_app_name(void) {
  print (APP_NAME);
}
