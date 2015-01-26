#include "usb_arc_codes.h"

const keymap keycode_map[] = {
    {.name=NULL, .keys=NULL, .numerator=FALSE},
    {.name=NULL, .keys=NULL, .numerator=FALSE},
    {.name=NULL, .keys=NULL, .numerator=FALSE},
    {.name=NULL, .keys=NULL, .numerator=FALSE},
    {.name="A", .keys="Aa", .numerator=FALSE},
    {.name="B", .keys="Bb", .numerator=FALSE},
    {.name="C", .keys="Cc", .numerator=FALSE},
    {.name="D", .keys="Dd", .numerator=FALSE},
    {.name="E", .keys="Ee", .numerator=FALSE},
    {.name="F", .keys="Ff", .numerator=FALSE},
    {.name="G", .keys="Gg", .numerator=FALSE},
    {.name="H", .keys="Hh", .numerator=FALSE},
    {.name="I", .keys="Ii", .numerator=FALSE},
    {.name="J", .keys="Jj", .numerator=FALSE},
    {.name="K", .keys="Kk", .numerator=FALSE},
    {.name="L", .keys="Ll", .numerator=FALSE},
    {.name="M", .keys="Mm", .numerator=FALSE}, // 0x10
    {.name="N", .keys="Nn", .numerator=FALSE},
    {.name="O", .keys="Oo", .numerator=FALSE},
    {.name="P", .keys="Pp", .numerator=FALSE},
    {.name="Q", .keys="Qq", .numerator=FALSE},
    {.name="R", .keys="Rr", .numerator=FALSE},
    {.name="S", .keys="Ss", .numerator=FALSE},
    {.name="T", .keys="Tt", .numerator=FALSE},
    {.name="U", .keys="Uu", .numerator=FALSE},
    {.name="V", .keys="Vv", .numerator=FALSE},
    {.name="W", .keys="Ww", .numerator=FALSE},
    {.name="X", .keys="Xx", .numerator=FALSE},
    {.name="Y", .keys="Yy", .numerator=FALSE},
    {.name="Z", .keys="Zz", .numerator=FALSE},
    {.name="1", .keys="1", .numerator=FALSE},
    {.name="2", .keys="2", .numerator=FALSE},
    {.name="3", .keys="3", .numerator=FALSE}, // 0x20
    {.name="4", .keys="4", .numerator=FALSE},
    {.name="5", .keys="5", .numerator=FALSE},
    {.name="6", .keys="6", .numerator=FALSE},
    {.name="7", .keys="7", .numerator=FALSE},
    {.name="8", .keys="8", .numerator=FALSE},
    {.name="9", .keys="9", .numerator=FALSE},
    {.name="0", .keys="0", .numerator=FALSE},
    {.name="ENTER", .keys="\n", .numerator=FALSE},
    {.name="ESCAPE", .keys=NULL, .numerator=FALSE},
    {.name="BACKSPACE", .keys="\b", .numerator=FALSE},
    {.name="TAB", .keys="\t", .numerator=FALSE},
    {.name="SPACE", .keys=" ", .numerator=FALSE},
    {.name="MINUS", .keys="-_", .numerator=FALSE}, //- _
    {.name="EQUAL", .keys="=", .numerator=FALSE},
    {.name="LEFTBRACKET", .keys="[", .numerator=FALSE},
    {.name="RIGHTBRACKET", .keys="]", .numerator=FALSE}, // 0x30
    {.name="BACKSLASH", .keys="\\", .numerator=FALSE}, // \ |
    {.name="NONUS_HASH", .keys="#~", .numerator=FALSE}, // Non-US # ~
    {.name="SEMICOLON", .keys=";:", .numerator=FALSE}, // ; :
    {.name="QUOTE", .keys="\"'", .numerator=FALSE}, // ' "
    {.name="GRAVE", .keys="~`", .numerator=FALSE}, // ~ `
    {.name="COMMA", .keys=",", .numerator=FALSE}, // < ,
    {.name="DOT", .keys=".", .numerator=FALSE},   // > .
    {.name="SLASH", .keys="?/", .numerator=FALSE},  // ? /
    {.name="CAPSLOCK", .keys=NULL, .numerator=FALSE},
    {.name="F1", .keys=NULL, .numerator=FALSE},
    {.name="F2", .keys=NULL, .numerator=FALSE},
    {.name="F3", .keys=NULL, .numerator=FALSE},
    {.name="F4", .keys=NULL, .numerator=FALSE},
    {.name="F5", .keys=NULL, .numerator=FALSE},
    {.name="F6", .keys=NULL, .numerator=FALSE},
    {.name="F7", .keys=NULL, .numerator=FALSE}, // 0x40
    {.name="F8", .keys=NULL, .numerator=FALSE},
    {.name="F9", .keys=NULL, .numerator=FALSE},
    {.name="F10", .keys=NULL, .numerator=FALSE},
    {.name="F11", .keys=NULL, .numerator=FALSE},
    {.name="F12", .keys=NULL, .numerator=FALSE},
    {.name="PRINTSCREEN", .keys=NULL, .numerator=FALSE},
    {.name="SCROLLLOCK", .keys=NULL, .numerator=FALSE},
    {.name="PAUSE", .keys=NULL, .numerator=FALSE},
    {.name="INSERT", .keys=NULL, .numerator=FALSE},
    {.name="HOME", .keys=NULL, .numerator=FALSE},
    {.name="PGUP", .keys=NULL, .numerator=FALSE},
    {.name="DELETE", .keys=NULL, .numerator=FALSE},
    {.name="END", .keys=NULL, .numerator=FALSE},
    {.name="PGDOWN", .keys=NULL, .numerator=FALSE},
    {.name="RIGHT", .keys=NULL, .numerator=FALSE},
    {.name="LEFT", .keys=NULL, .numerator=FALSE}, // 0x50
    {.name="DOWN", .keys=NULL, .numerator=FALSE},
    {.name="UP", .keys=NULL, .numerator=FALSE},
    {.name="NUMLOCK", .keys=NULL, .numerator=FALSE},
    {.name="KP_SLASH", .keys=NULL, .numerator=FALSE},
    {.name="KP_ASTERISK", .keys=NULL, .numerator=FALSE},
    {.name="KP_MINUS", .keys=NULL, .numerator=FALSE},
    {.name="KP_PLUS", .keys=NULL, .numerator=FALSE},
    {.name="KP_ENTER", .keys=NULL, .numerator=FALSE},
    {.name="KP_1", .keys=NULL, .numerator=FALSE},
    {.name="KP_2", .keys=NULL, .numerator=FALSE},
    {.name="KP_3", .keys=NULL, .numerator=FALSE},
    {.name="KP_4", .keys=NULL, .numerator=FALSE},
    {.name="KP_5", .keys=NULL, .numerator=FALSE},
    {.name="KP_6", .keys=NULL, .numerator=FALSE},
    {.name="KP_7", .keys=NULL, .numerator=FALSE},
    {.name="KP_8", .keys=NULL, .numerator=FALSE}, // 0x60
    {.name="KP_9", .keys=NULL, .numerator=FALSE},
    {.name="KP_0", .keys=NULL, .numerator=FALSE},
    {.name="KP_DOT", .keys=NULL, .numerator=FALSE},
    {.name="NONUS_BACKSLASH", .keys="\\|", .numerator=FALSE}, // Non-US \ |
    {.name="APPLICATION", .keys=NULL, .numerator=FALSE},
    {.name="POWER", .keys=NULL, .numerator=FALSE},
    {.name="KP_EQUAL", .keys=NULL, .numerator=FALSE},
    {.name="F13", .keys=NULL, .numerator=FALSE},
    {.name="F14", .keys=NULL, .numerator=FALSE},
    {.name="F15", .keys=NULL, .numerator=FALSE},
    {.name="F16", .keys=NULL, .numerator=FALSE},
    {.name="F17", .keys=NULL, .numerator=FALSE},
    {.name="F18", .keys=NULL, .numerator=FALSE},
    {.name="F19", .keys=NULL, .numerator=FALSE},
    {.name="F20", .keys=NULL, .numerator=FALSE},
    {.name="F21", .keys=NULL, .numerator=FALSE}, // 0x70
    {.name="F22", .keys=NULL, .numerator=FALSE},
    {.name="F23", .keys=NULL, .numerator=FALSE},
    {.name="F24", .keys=NULL, .numerator=FALSE},
    {.name="EXECUTE", .keys=NULL, .numerator=FALSE},
    {.name="HELP", .keys=NULL, .numerator=FALSE},
    {.name="MENU", .keys=NULL, .numerator=FALSE},
    {.name="SELECT", .keys=NULL, .numerator=FALSE},
    {.name="STOP", .keys=NULL, .numerator=FALSE},
    {.name="AGAIN", .keys=NULL, .numerator=FALSE},
    {.name="UNDO", .keys=NULL, .numerator=FALSE},
    {.name="CUT", .keys=NULL, .numerator=FALSE},
    {.name="COPY", .keys=NULL, .numerator=FALSE},
    {.name="PASTE", .keys=NULL, .numerator=FALSE},
    {.name="FIND", .keys=NULL, .numerator=FALSE},
    {.name="MUTE", .keys=NULL, .numerator=FALSE},
    {.name="VOLUME_UP", .keys=NULL, .numerator=FALSE}, // 0x80
    {.name="VOLUME_DOWN", .keys=NULL, .numerator=FALSE},
    {.name="LOCKING_CAPS", .keys=NULL, .numerator=FALSE},
    {.name="LOCKING_NUM", .keys=NULL, .numerator=FALSE},
    {.name="LOCKING_SCROLL", .keys=NULL, .numerator=FALSE},
    {.name="KP_COMMA", .keys=NULL, .numerator=FALSE},
    {.name="KP_EQUAL_AS400", .keys=NULL, .numerator=FALSE}, // = on AS/400
    {.name="INT1", .keys=NULL, .numerator=FALSE},
    {.name="INT2", .keys=NULL, .numerator=FALSE},
    {.name="INT3", .keys=NULL, .numerator=FALSE},
    {.name="INT4", .keys=NULL, .numerator=FALSE},
    {.name="INT5", .keys=NULL, .numerator=FALSE},
    {.name="INT6", .keys=NULL, .numerator=FALSE},
    {.name="INT7", .keys=NULL, .numerator=FALSE},
    {.name="INT8", .keys=NULL, .numerator=FALSE},
    {.name="INT9", .keys=NULL, .numerator=FALSE},
    {.name="LANG1", .keys=NULL, .numerator=FALSE}, // 0x90
    {.name="LANG2", .keys=NULL, .numerator=FALSE},
    {.name="LANG3", .keys=NULL, .numerator=FALSE},
    {.name="LANG4", .keys=NULL, .numerator=FALSE},
    {.name="LANG5", .keys=NULL, .numerator=FALSE},
    {.name="LANG6", .keys=NULL, .numerator=FALSE},
    {.name="LANG7", .keys=NULL, .numerator=FALSE},
    {.name="LANG8", .keys=NULL, .numerator=FALSE},
    {.name="LANG9", .keys=NULL, .numerator=FALSE},
    {.name="ALT_ERASE", .keys=NULL, .numerator=FALSE},
    {.name="SYSREQ", .keys=NULL, .numerator=FALSE},
    {.name="CANCEL", .keys=NULL, .numerator=FALSE},
    {.name="CLEAR", .keys=NULL, .numerator=FALSE},
    {.name="PRIOR", .keys=NULL, .numerator=FALSE},
    {.name="RETURN", .keys=NULL, .numerator=FALSE},
    {.name="SEPARATOR", .keys=NULL, .numerator=FALSE},
    {.name="OUT", .keys=NULL, .numerator=FALSE}, // 0xa0
    {.name="OPER", .keys=NULL, .numerator=FALSE},
    {.name="CLEAR_AGAIN", .keys=NULL, .numerator=FALSE},
    {.name="CRSEL", .keys=NULL, .numerator=FALSE},
    {.name="EXSEL", .keys=NULL, .numerator=FALSE},

    {.name="SYSTEM_POWER", .keys=NULL, .numerator=FALSE},
    {.name="SYSTEM_SLEEP", .keys=NULL, .numerator=FALSE},
    {.name="SYSTEM_WAKE", .keys=NULL, .numerator=FALSE},

    {.name="AUDIO_MUTE", .keys=NULL, .numerator=FALSE},
    {.name="AUDIO_VOL_UP", .keys=NULL, .numerator=FALSE},
    {.name="AUDIO_VOL_DOWN", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_NEXT_TRACK", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_PREV_TRACK", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_STOP", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_PLAY_PAUSE", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_SELECT", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_EJECT", .keys=NULL, .numerator=FALSE}, // 0xb0
    {.name="MAIL", .keys=NULL, .numerator=FALSE},
    {.name="CALCULATOR", .keys=NULL, .numerator=FALSE},
    {.name="MY_COMPUTER", .keys=NULL, .numerator=FALSE},
    {.name="WWW_SEARCH", .keys=NULL, .numerator=FALSE},
    {.name="WWW_HOME", .keys=NULL, .numerator=FALSE},
    {.name="WWW_BACK", .keys=NULL, .numerator=FALSE},
    {.name="WWW_FORWARD", .keys=NULL, .numerator=FALSE},
    {.name="WWW_STOP", .keys=NULL, .numerator=FALSE},
    {.name="WWW_REFRESH", .keys=NULL, .numerator=FALSE},
    {.name="WWW_FAVORITES", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_FAST_FORWARD", .keys=NULL, .numerator=FALSE},
    {.name="MEDIA_REWIND", .keys=NULL, .numerator=FALSE}, // 0xbc

    {.name="LEFT_CTRL", .keys=NULL, .numerator=FALSE}, // 0xbd
    {.name="LEFT_SHIFT", .keys=NULL, .numerator=FALSE}, // 0xbe
    {.name="LEFT_ALT", .keys=NULL, .numerator=FALSE}, // 0xbf
    {.name="LEFT_GUI", .keys=NULL, .numerator=FALSE}, // 0xc0
    {.name="RIGHT_CTRL", .keys=NULL, .numerator=FALSE}, // 0xc1
    {.name="RIGHT_SHIFT", .keys=NULL, .numerator=FALSE}, // 0xc2
    {.name="RIGHT_ALT", .keys=NULL, .numerator=FALSE}, // 0xc3
    {.name="RIGHT_GUI", .keys=NULL, .numerator=FALSE}, // 0xc4
};

const keymap mousecode_map[] = {
    {.name="MOUSE_X", .keys=NULL, .numerator=TRUE},
    {.name="MOUSE_Y", .keys=NULL, .numerator=TRUE},
    {.name="MOUSE_BUTTON1", .keys=NULL, .numerator=FALSE},
    {.name="MOUSE_BUTTON2", .keys=NULL, .numerator=FALSE},
    {.name="MOUSE_BUTTON3", .keys=NULL, .numerator=FALSE},
    {.name="MOUSE_WHEEL", .keys=NULL, .numerator=TRUE},
};

const keymap *USB_ARC_get_keymap(enum kb_hid_code code) {
  return &keycode_map[code];
}
const keymap *USB_ARC_get_mousemap(enum mouse_code code) {
  return &mousecode_map[code];
}
