arcadehid
=========

STM32 HID device for arcade builds

Compound HID device containing keyboard, mouse, two joysticks and a virtual com port. Plug into a retropie or something and wire cool buttons and joysticks and whatnot.

See /cad - here you'll find a reference design with 26 pins for an STM32F1 128kB flash variant. Any STM32F1 board would do, as long as pin-headers are broken out.

Each pin can be configured with combinations of a keyboard keypress, mouse movement or click, or joystick movement or button press.

Every pin can hold up to eight different combinations. There is also ternary support meaning a pin's config can change depending on if another pin is active or not.

Accelerators for mouse and joystick are supported.

Everything is configured in the command line interface, either via UART pins or via virtual com port.

Different configurations can be saved and loaded, using the internal flash of the STM32F1 (128 kb variant) - no extra storage chip needed.

Depends on pellepl/generic_embedded and pellepl/niffs.

Now also with the Annoyatron support, drive your friends mad...

CLI pin definition examples
===========================
´´´
// define pin 1 to send keyboard character A\n"
def pin1 = a
// define pin 2 to move mouse up
def pin2 = mouse_y(-1)
// define pin 3 to move mouse right if pin 4 is pressed else left
def pin3 = pin4 ? mouse_x(1) : mouse_x(-1)
// define pin 5 to send keyboard sequence CTRL+ALT+DEL
def pin5 = LEFT_CTRL LEFT_ALT DELETE\n"
´´´

For certain definitions, numerators and accelerators are possible.
A numerator ´´´<num>´´´ is defined as  ´´´(ACC)[(+)|-][1..127]´´´.

´´´
// move joystick 10 steps right on x axis
def pin7 = joy1_x(10)
// move joystick accelerating from 1 to 10 steps right on x axis
def pin8 = joy1_x(ACC10)
// and left
def pin8 = joy1_x(ACC-10)
´´´

Accelerator speed is controlled by cli commands
´´´
set_mouse_pos_acc
set_mouse_wheel_acc
set_joy_acc
´´´

Following is a list of all definitions possible.

´´´
KEYBOARD SYMBOLS:
  A                   B                   C                   D                   
  E                   F                   G                   H                   
  I                   J                   K                   L                   
  M                   N                   O                   P                   
  Q                   R                   S                   T                   
  U                   V                   W                   X                   
  Y                   Z                   1                   2                   
  3                   4                   5                   6                   
  7                   8                   9                   0                   
  ENTER               ESCAPE              BACKSPACE           TAB                 
  SPACE               MINUS               EQUAL               LEFTBRACKET         
  RIGHTBRACKET        BACKSLASH           NONUS_HASH          SEMICOLON           
  QUOTE               GRAVE               COMMA               DOT                 
  SLASH               CAPSLOCK            F1                  F2                  
  F3                  F4                  F5                  F6                  
  F7                  F8                  F9                  F10                 
  F11                 F12                 PRINTSCREEN         SCROLLLOCK          
  PAUSE               INSERT              HOME                PGUP                
  DELETE              END                 PGDOWN              RIGHT               
  LEFT                DOWN                UP                  NUMLOCK             
  KP_SLASH            KP_ASTERISK         KP_MINUS            KP_PLUS             
  KP_ENTER            KP_1                KP_2                KP_3                
  KP_4                KP_5                KP_6                KP_7                
  KP_8                KP_9                KP_0                KP_DOT              
  NONUS_BACKSLASH     APPLICATION         POWER               KP_EQUAL            
  F13                 F14                 F15                 F16                 
  F17                 F18                 F19                 F20                 
  F21                 F22                 F23                 F24                 
  EXECUTE             HELP                MENU                SELECT              
  STOP                AGAIN               UNDO                CUT                 
  COPY                PASTE               FIND                MUTE                
  VOLUME_UP           VOLUME_DOWN         LOCKING_CAPS        LOCKING_NUM         
  LOCKING_SCROLL      KP_COMMA            KP_EQUAL_AS400      INT1                
  INT2                INT3                INT4                INT5                
  INT6                INT7                INT8                INT9                
  LANG1               LANG2               LANG3               LANG4               
  LANG5               LANG6               LANG7               LANG8               
  LANG9               ALT_ERASE           SYSREQ              CANCEL              
  CLEAR               PRIOR               RETURN              SEPARATOR           
  OUT                 OPER                CLEAR_AGAIN         CRSEL               
  EXSEL               SYSTEM_POWER        SYSTEM_SLEEP        SYSTEM_WAKE         
  AUDIO_MUTE          AUDIO_VOL_UP        AUDIO_VOL_DOWN      MEDIA_NEXT_TRACK    
  MEDIA_PREV_TRACK    MEDIA_STOP          MEDIA_PLAY_PAUSE    MEDIA_SELECT        
  MEDIA_EJECT         MAIL                CALCULATOR          MY_COMPUTER         
  WWW_SEARCH          WWW_HOME            WWW_BACK            WWW_FORWARD         
  WWW_STOP            WWW_REFRESH         WWW_FAVORITES       MEDIA_FAST_FORWARD  
  MEDIA_REWIND        LEFT_CTRL           LEFT_SHIFT          LEFT_ALT            
  LEFT_GUI            RIGHT_CTRL          RIGHT_SHIFT         RIGHT_ALT           
  RIGHT_GUI           

MOUSE SYMBOLS:
  MOUSE_X(<num>)      MOUSE_Y(<num>)      MOUSE_BUTTON1       MOUSE_BUTTON2       
  MOUSE_BUTTON3       MOUSE_WHEEL(<num>)  

JOYSTICK SYMBOLS:
  JOY1_X(<num>)       JOY1_Y(<num>)       JOY1_BUTTON1        JOY1_BUTTON2        
  JOY1_BUTTON3        JOY1_BUTTON4        JOY1_BUTTON5        JOY1_BUTTON6        
  JOY1_BUTTON7        JOY1_BUTTON8        JOY1_BUTTON9        JOY1_BUTTON10       
  JOY1_BUTTON11       JOY1_BUTTON12       JOY1_BUTTON13       JOY1_BUTTON14       
  JOY2_X(<num>)       JOY2_Y(<num>)       JOY2_BUTTON1        JOY2_BUTTON2        
  JOY2_BUTTON3        JOY2_BUTTON4        JOY2_BUTTON5        JOY2_BUTTON6        
  JOY2_BUTTON7        JOY2_BUTTON8        JOY2_BUTTON9        JOY2_BUTTON10       
  JOY2_BUTTON11       JOY2_BUTTON12       JOY2_BUTTON13       JOY2_BUTTON14
´´´
