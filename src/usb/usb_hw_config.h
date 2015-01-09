#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

#include "system.h"
#include "usb_type.h"

#ifdef CONFIG_ARCHID_VCD
#include "ringbuf.h"
#include "usb_serial.h"

#define USB_VCD_TX_BUF_SIZE   1024
#define USB_VCD_RX_BUF_SIZE   512

extern ringbuf tx_rb;
extern ringbuf rx_rb;
extern usb_serial_rx_cb rx_cb;

void Handle_USBAsynchXfer(void);
#endif

extern volatile uint8_t kb_tx_complete;
extern volatile uint8_t mouse_tx_complete;

extern uint8_t kb_led_state;

void USB_Cable_Config (FunctionalState NewState);
void Get_SerialNum(void);


#endif  /*__HW_CONFIG_H*/
