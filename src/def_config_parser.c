/*
 * def_config_parser.c
 *
 *  Created on: Jan 18, 2015
 *      Author: petera
 */

#include "def_config_parser.h"

#include "usb/usb_arc_codes.h"

#define MAX_LEX_SYM_LEN   (4+APP_CONFIG_DEFS_PER_PIN*2+1)

typedef enum {
  LEX_UNKNOWN = 0, LEX_PIN, LEX_DEF, LEX_NUM, LEX_ASSIGN, LEX_TERN, LEX_TERN_OPT
} lex_type;

typedef struct {
  lex_type type;
  u16_t offs_start;
  u16_t offs_end;
} lex_type_sym;

typedef enum {
  LEX_STATE_INIT = 0, LEX_STATE_SYM, LEX_STATE_NUM
} lex_state;

const char *ignore_chars = " \t";
const char *sym_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
const char *num_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
const char *numdef_chars = "()";
const char *assign_chars = "=";
const char *tern_chars = "?:";
const char *pin_sym = "pin";
const char *acc_sym = "acc";

static u8_t lex_sym_ix;
static lex_type_sym lex_syms[MAX_LEX_SYM_LEN];

static char to_lower(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

static bool is_pin_sym(const char *str, lex_type_sym *sym) {
  if (1 + sym->offs_end - sym->offs_start <= strlen(pin_sym)) {
    return FALSE;
  }
  int i;
  for (i = 0; i < strlen(pin_sym); i++) {
    if (to_lower(pin_sym[i]) != to_lower(str[sym->offs_start + i])) {
      return FALSE;
    }
  }
  return TRUE;
}

static bool is_acc_sym(const char *str, lex_type_sym *sym) {
  if (1 + sym->offs_end - sym->offs_start <= strlen(acc_sym)) {
    return FALSE;
  }
  int i;
  for (i = 0; i < strlen(acc_sym); i++) {
    if (to_lower(acc_sym[i]) != to_lower(str[sym->offs_start + i])) {
      return FALSE;
    }
  }
  return TRUE;
}

static bool parse_pin_nbr(const char *str, lex_type_sym *sym, u8_t *nbr) {
  if (!is_pin_sym(str, sym))
    return FALSE;
  *nbr = 0;
  int i;
  for (i = 0; i <= sym->offs_end - sym->offs_start - strlen(pin_sym); i++) {
    *nbr *= 10;
    char c = str[sym->offs_start + i + strlen(pin_sym)];
    if (c < '0' || c > '9')
      return FALSE;
    *nbr += c - '0';
  }
  return TRUE;
}

static bool parse_numerator_nbr(const char *str, lex_type_sym *sym, u32_t offs, s32_t *nbr) {
  *nbr = 0;
  int i = offs;
  bool sign = FALSE;
  if (str[sym->offs_start + i] == '+') {
    i++;
  } else if (str[sym->offs_start + i] == '-') {
    sign = TRUE;
    i++;
  }
  while (i <= sym->offs_end - sym->offs_start) {
    *nbr *= 10;
    char c = str[sym->offs_start + i];
    if (c < '0' || c > '9')
      return FALSE;
    *nbr += c - '0';
    i++;
  }
  if (sign) {
    *nbr = -*nbr;
  }
  return TRUE;
}

static void print_lex_sym(lex_type_sym *sym, const char *str) {
  int i;
  for (i = sym->offs_start; i < sym->offs_end + 1; i++) {
    KEYPARSERR("%c", str[i]);
  }
  KEYPARSERR(" @ index %i\n", sym->offs_start);
}

static void print_index_indicator(const char *str, u16_t ix) {
  KEYPARSERR("%s\n", str);
  while (ix--) {
    KEYPARSERR(" ");
  }
  KEYPARSERR("^\n");
}

static bool emit_lex_sym(lex_type_sym *sym, const char *str) {
  switch (sym->type) {
  case LEX_DEF: {
    if (is_pin_sym(str, sym)) {
      sym->type = LEX_PIN;
    }
    break;
  }
  case LEX_ASSIGN:
  case LEX_NUM:
  case LEX_TERN:
  case LEX_TERN_OPT:
    break;
  default:
    print_index_indicator(str, sym->offs_start);
    KEYPARSERR("Error: unknown symbol\n");
    return FALSE;
    break;
  }

  if (lex_sym_ix >= MAX_LEX_SYM_LEN) {
    KEYPARSERR("Error: too long definition\n");
    return FALSE;
  }

  lex_syms[lex_sym_ix].type = sym->type;
  lex_syms[lex_sym_ix].offs_start = sym->offs_start;
  lex_syms[lex_sym_ix].offs_end = sym->offs_end;
  lex_sym_ix++;

  sym->type = LEX_UNKNOWN;

  sym->offs_start = 0;
  sym->offs_end = 0;

  return TRUE;
}

static int sym_strcmp(const char *str, const char *sym_str, lex_type_sym *sym) {
  char c1, c2;
  const char *s2 = &sym_str[sym->offs_start];
  while ((((c1 = *str++) != 0) & ((c2 = *s2++) != 0)) && s2 <= &sym_str[sym->offs_end]) {
    if (to_lower(c1) != to_lower(c2)) {
      return -1;
    }
  }
  return to_lower(c1) - to_lower(c2);
}

static void lookup_def(const char *str, lex_type_sym *sym, hid_id *h_id, bool *numerator) {
  // test keyboard definitions
  enum kb_hid_code kb_code;
  for (kb_code = 0; kb_code < _KB_HID_CODE_MAX; kb_code++) {
    const keymap *kb_map = USB_ARC_get_keymap(kb_code);
    if (kb_map->name == NULL) continue;
    if (sym_strcmp(kb_map->name, str, sym) == 0) {
      h_id->type = HID_ID_TYPE_KEYBOARD;
      h_id->kb.kb_code = kb_code;
      *numerator = kb_map->numerator;
      return;
    }
  }

  // test mouse definitions
  enum mouse_code m_code;
  for (m_code = 0; m_code < _MOUSE_CODE_MAX; m_code++) {
    const keymap *m_map = USB_ARC_get_mousemap(m_code);
    if (m_map->name == NULL) continue;
    if (sym_strcmp(m_map->name, str, sym) == 0) {
      h_id->type = HID_ID_TYPE_MOUSE;
      h_id->mouse.mouse_code = m_code;
      *numerator = m_map->numerator;
      return;
    }
  }

  h_id->type = HID_ID_TYPE_NONE;
}

static bool lex(const char *str, u16_t len) {
  int i;
  lex_sym_ix = 0;
  memset(lex_syms, 0, sizeof(lex_syms));
  lex_state state = LEX_STATE_INIT;
  lex_type_sym sym = { LEX_UNKNOWN, 0, 0 };

  for (i = 0; i < len + 1; i++) {
    char c = i >= len ? ignore_chars[0] : str[i];
    bool is_ignore = strchr(ignore_chars, c) != 0;
    bool is_sym = strchr(sym_chars, c) != 0;
    bool is_num = strchr(num_chars, c) != 0;
    bool is_numdef = strchr(numdef_chars, c) != 0;
    bool is_assign = strchr(assign_chars, c) != 0;
    bool is_tern = strchr(tern_chars, c) != 0;
    if (!(is_ignore || is_sym || is_num || is_numdef || is_assign || is_tern)) {
      print_index_indicator(str, i);
      KEYPARSERR("Error: bad character [%c] @ index %i\n", c, i);
      return FALSE;
    }

    switch (state) {
    case LEX_STATE_INIT:
      if (is_ignore) {
        continue;
      } else if (is_sym) {
        sym.type = LEX_DEF;
        sym.offs_start = i;
        state = LEX_STATE_SYM;
      } else if (is_numdef && c == numdef_chars[0]) {
        sym.type = LEX_NUM;
        sym.offs_start = i + 1;
        state = LEX_STATE_NUM;
      } else if (is_assign) {
        sym.type = LEX_ASSIGN;
        sym.offs_start = sym.offs_end = i;
        if (!emit_lex_sym(&sym, str))
          return FALSE;
        state = LEX_STATE_INIT;
      } else if (is_tern) {
        sym.type = c == tern_chars[0] ? LEX_TERN : LEX_TERN_OPT;
        sym.offs_start = sym.offs_end = i;
        if (!emit_lex_sym(&sym, str))
          return FALSE;
        state = LEX_STATE_INIT;
      } else {
        print_index_indicator(str, i);
        KEYPARSERR("Error: unexpected character [%c] @ index %i\n", c, i);
        return FALSE;
      }
      break;
    case LEX_STATE_SYM:
      if (!is_sym) {
        sym.offs_end = i - 1;
        if (!emit_lex_sym(&sym, str))
          return FALSE;
        // reparse
        state = LEX_STATE_INIT;
        i--;
        continue;
      }
      break;
      case LEX_STATE_NUM:
      if (is_num) {
        // ok
      } else if (is_numdef && c == numdef_chars[1]) {
        sym.offs_end = i-1;
        if (!emit_lex_sym(&sym, str)) return FALSE;
        state = LEX_STATE_INIT;
      } else {
        print_index_indicator(str, i);
        KEYPARSERR("Error: unexpected character [%c] @ index %i\n", c, i);
        return FALSE;
      }
      break;
    }
  }
  return TRUE;
}

static bool parse_numerator(lex_type_sym *sym, const char *str, hid_id *id) {
  s32_t nbr = 0;
  u32_t offs = 0;
  if (is_acc_sym(str, sym)) {
    offs = strlen(acc_sym);
    id->mouse.mouse_acc = TRUE;
  }
  if (!parse_numerator_nbr(str, sym, offs, &nbr)) {
    print_index_indicator(str, sym->offs_start);
    KEYPARSERR("Syntax error: could not parse numerator ");
    print_lex_sym(sym, str);
    return FALSE;
  }
  if (nbr < -128 || nbr > 127 || nbr == 0) {
    print_index_indicator(str, sym->offs_start);
    KEYPARSERR("Error: numerator bad value ");
    print_lex_sym(sym, str);
    return FALSE;
  }
  if (nbr < 0) {
    id->mouse.mouse_sign = TRUE;
    nbr = -nbr;
  }
  id->mouse.mouse_data = nbr;
  return TRUE;
}

// syntax format:
//   PIN ASSIGN {def* | PIN TERN def* TERN_OPT def*}
//
//   def = SYM (NUM)
//

static bool parse(def_config *pindef, const char *str, lex_type_sym *syms, u8_t lex_sym_cnt) {
  int sym_ix;

  u8_t def_ix = 0;

  memset(pindef, 0, sizeof(def_config));

  // check common syntax
  int tern_ix = -1;
  int tern_opt_ix = -1;
  for (sym_ix = 0; sym_ix < lex_sym_cnt; sym_ix++) {
    lex_type_sym *sym = &syms[sym_ix];
    switch (sym->type) {
    case LEX_DEF:
      break;
    case LEX_NUM:
      if (sym_ix == 0) {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR(
            "Syntax error: unexpected numerator ");
        print_lex_sym(sym, str);
        return FALSE;
      } else if (syms[sym_ix - 1].type != LEX_DEF) {
        print_index_indicator(str, syms[sym_ix-1].offs_start);
        KEYPARSERR("Syntax error: numerator must follow a definition, found ");
        print_lex_sym(&syms[sym_ix - 1], str);
        return FALSE;
      }
      break;
    case LEX_PIN:
      break;
    case LEX_ASSIGN:
      break;
    case LEX_TERN:
      if (tern_ix >= 0) {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR(
            "Syntax error: ternary '%c' @ index %i already defined @ index %i\n", tern_chars[0], sym->offs_start, syms[tern_ix].offs_start);
        return FALSE;
      }
      if (sym_ix > 0 && syms[sym_ix - 1].type != LEX_PIN) {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR(
            "Syntax error: ternary '%c' @ index %i must follow a pin definition, found ", tern_chars[0], sym->offs_start);
        print_lex_sym(&syms[sym_ix - 1], str);
        return FALSE;
      }
      tern_ix = sym_ix;
      break;
    case LEX_TERN_OPT:
      if (tern_opt_ix >= 0) {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR(
            "Syntax error: ternary option '%c' @ index %i already defined @ index %i\n", tern_chars[1], sym->offs_start, syms[tern_opt_ix].offs_start);
        return FALSE;
      }
      tern_opt_ix = sym_ix;
      break;
    default:
      print_index_indicator(str, sym->offs_start);
      KEYPARSERR("Syntax error: unknown symbol ");
      print_lex_sym(sym, str);
      return FALSE;
    }
  }

  if (lex_sym_cnt == 0) {
    KEYPARSERR("Error: no input\n");
    return FALSE;
  }

  // ternary syntax
  if (tern_ix >= 0 && tern_opt_ix < 0) {
    print_index_indicator(str, syms[tern_ix].offs_start);
    KEYPARSERR(
        "Syntax error: found ternary '%c' @ index %i without ternary option '%c'\n", tern_chars[0], syms[tern_ix].offs_start, tern_chars[1]);
    return FALSE;
  }
  if (tern_ix >= 0 && tern_opt_ix < tern_ix) {
    print_index_indicator(str, syms[tern_opt_ix].offs_start);
    KEYPARSERR(
        "Syntax error: found ternary option '%c' @ index %i declared before ternary '%c'\n", tern_chars[1], syms[tern_opt_ix].offs_start, tern_chars[0]);
    return FALSE;
  }

  // assignment syntax
  if (syms[0].type != LEX_PIN) {
    print_index_indicator(str, syms[0].offs_start);
    KEYPARSERR("Syntax error: expected pin as first definition, found ");
    print_lex_sym(&syms[0], str);
    return FALSE;
  }

  if (!parse_pin_nbr(str, &syms[0], &pindef->pin)) {
    print_index_indicator(str, syms[0].offs_start);
    KEYPARSERR("Syntax error: bad pin number ");
    print_lex_sym(&syms[0], str);
    return FALSE;
  }

  if (pindef->pin <= 0 || pindef->pin > APP_CONFIG_PINS) {
    print_index_indicator(str, syms[0].offs_start);
    KEYPARSERR("Syntax error: pin number out of range ");
    print_lex_sym(&syms[0], str);
    return FALSE;
  }

  if (lex_sym_cnt < 2 || syms[1].type != LEX_ASSIGN) {
    if (lex_sym_cnt >= 1) print_index_indicator(str, syms[1].offs_start);
    KEYPARSERR(
        "Syntax error: expected assignment '%c' as second definition", assign_chars[0]);
    if (lex_sym_cnt > 1) {
      KEYPARSERR(", found ");
      print_lex_sym(&syms[1], str);
    } else {
      KEYPARSERR("\n");
    }
    return FALSE;
  }

  if (lex_sym_cnt >= 3) {
    if (syms[2].type == LEX_DEF) {
      pindef->tern_pin = 0;
    } else if (syms[2].type == LEX_PIN) {
      // ternary syntax
      if (!parse_pin_nbr(str, &syms[2], &pindef->tern_pin)) {
        print_index_indicator(str, syms[2].offs_start);
        KEYPARSERR("Syntax error: bad ternary pin number ");
        print_lex_sym(&syms[2], str);
        return FALSE;
      }
      if (pindef->tern_pin == 0 || pindef->tern_pin > APP_CONFIG_PINS) {
        print_index_indicator(str, syms[2].offs_start);
        KEYPARSERR("Syntax error: ternary pin number out of range ");
        print_lex_sym(&syms[2], str);
        return FALSE;
      }
      if (pindef->tern_pin == pindef->pin) {
        print_index_indicator(str, syms[2].offs_start);
        KEYPARSERR(
            "Syntax error: ternary pin cannot have same number as pin assignment ");
        print_lex_sym(&syms[2], str);
        return FALSE;
      }

      if (syms[3].type != LEX_TERN) {
        print_index_indicator(str, syms[3].offs_start);
        KEYPARSERR(
            "Syntax error: ternary character '%c' must follow pin ternary definition, found ", tern_chars[0]);
        print_lex_sym(&syms[3], str);
        return FALSE;
      }
    } else {
      print_index_indicator(str, syms[2].offs_start);
      KEYPARSERR("Syntax error: unexpected definition ");
      print_lex_sym(&syms[2], str);
      return FALSE;
    }
  }

  // build

  def_ix = 0;
  sym_ix = pindef->tern_pin > 0 ? 4 : 2;
  pindef->tern_splice = 0;
  bool numerator = FALSE;
  bool prev_numerator = FALSE;
  while (sym_ix < lex_sym_cnt) {
    numerator = FALSE;
    lex_type_sym *sym = &syms[sym_ix];
    if (prev_numerator && sym->type != LEX_NUM) {
      print_index_indicator(str, sym->offs_start);
      KEYPARSERR("Syntax error: expected numerator instead of ");
      print_lex_sym(sym, str);
      return FALSE;
    }
    if (sym->type == LEX_DEF) {
      hid_id h_id;
      lookup_def(str, sym, &h_id, &numerator);
      pindef->id[def_ix].type = h_id.type;
      pindef->id[def_ix].raw = h_id.raw;
      if (pindef->id[def_ix].type == HID_ID_TYPE_NONE) {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR("Syntax error: unknown definition ");
        print_lex_sym(sym, str);
        return FALSE;
      }
      int i;
      for (i = pindef->tern_splice; i < def_ix; i++) {
        if (pindef->id[i].type == h_id.type &&
            pindef->id[i].raw == h_id.raw) {
          print_index_indicator(str, sym->offs_start);
          KEYPARSERR("Error: identical definition ");
          print_lex_sym(sym, str);
          return FALSE;
        }
      }
      def_ix++;
      if (def_ix > APP_CONFIG_DEFS_PER_PIN) {
        print_index_indicator(str, syms[def_ix + 1].offs_start);
        KEYPARSERR("Error: definition overflow\n");
        return FALSE;
      }

    } else if (sym->type == LEX_NUM) {
      if (prev_numerator) {
        if (!parse_numerator(sym, str, &pindef->id[def_ix-1])) {
          return FALSE;
        }
      } else {
        print_index_indicator(str, sym->offs_start);
        KEYPARSERR("Syntax error: unexpected numerator ");
        print_lex_sym(sym, str);
        return FALSE;
      }

      int i;
      for (i = pindef->tern_splice; i < def_ix-1; i++) {
        if (pindef->id[i].type == pindef->id[def_ix-1].type &&
            pindef->id[i].raw == pindef->id[def_ix-1].raw) {
          print_index_indicator(str, sym->offs_start);
          KEYPARSERR("Error: identical definition ");
          print_lex_sym(sym, str);
          return FALSE;
        }
      }
    } else if (sym->type == LEX_TERN_OPT) {
      pindef->tern_splice = def_ix;
    } else {
      print_index_indicator(str, sym->offs_start);
      KEYPARSERR("Syntax error: unexpected symbol ");
      print_lex_sym(sym, str);
      return FALSE;
    }
    prev_numerator = numerator;
    sym_ix++;
  }

  if (numerator) {
    lex_type_sym *sym = &syms[lex_sym_cnt-1];
    print_index_indicator(str, sym->offs_end);
    KEYPARSERR("Syntax error: expected numerator ");
    print_lex_sym(sym, str);
    return FALSE;
  }

  return TRUE;
}

bool def_config_parse(def_config *pindef, const char *str, u16_t len) {
  if (lex(str, len)) {
    return parse(pindef, str, lex_syms, lex_sym_ix);
  }
  return FALSE;
}

void def_config_print(def_config *pindef) {
  int i;
  print("pin%i = ", pindef->pin);
  if (pindef->tern_pin > 0) {
    print("pin%i ? ", pindef->tern_pin);
  }
  for (i = 0; i < APP_CONFIG_DEFS_PER_PIN; i++) {
    hid_id id = pindef->id[i];
    if (pindef->tern_pin > 0 && i == pindef->tern_splice)
      print(": ");
    if (id.type != HID_ID_TYPE_NONE) {
      if (id.type == HID_ID_TYPE_KEYBOARD) {
        print("%s ", USB_ARC_get_keymap(id.kb.kb_code)->name);
      } else if (id.type == HID_ID_TYPE_MOUSE) {
        print("%s", USB_ARC_get_mousemap(id.mouse.mouse_code)->name);
        if (id.mouse.mouse_code == MOUSE_X ||
            id.mouse.mouse_code == MOUSE_Y ||
            id.mouse.mouse_code == MOUSE_WHEEL) {
          print("(");
          if (id.mouse.mouse_acc) {
            print("ACC");
          }
          print("%s%i", id.mouse.mouse_sign ? "-" : "+", id.mouse.mouse_data);
          print(")");
        }
        print(" ");
      }
    }
  }
  print("\n");
}

