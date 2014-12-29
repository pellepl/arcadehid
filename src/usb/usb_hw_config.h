#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

#include "system.h"
#include "usb_type.h"

extern volatile uint8_t kb_tx_complete;

void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Cable_Config (FunctionalState NewState);
void Get_SerialNum(void);

void USB_KB_tx(usb_report *report);
void USB_KB_init(void);

#endif  /*__HW_CONFIG_H*/
