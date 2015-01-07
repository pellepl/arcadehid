#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

#include "system.h"
#include "usb_type.h"

extern volatile uint8_t kb_tx_complete;
extern volatile uint8_t mouse_tx_complete;

extern uint8_t kb_led_state;

void USB_Cable_Config (FunctionalState NewState);
void Get_SerialNum(void);


#endif  /*__HW_CONFIG_H*/
