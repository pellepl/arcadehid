#include "system_config.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_hw_config.h"
#include "ringbuf.h"

#include "usb_arcade.h"
#include "usb_pwr.h"

#include "usb_serial.h"

#include "gpio.h"

ErrorStatus HSEStartUpStatus;
/* Extern variables ----------------------------------------------------------*/
volatile uint8_t kb_tx_complete = 1;
volatile uint8_t mouse_tx_complete = 1;
volatile uint8_t joy1_tx_complete = 1;
volatile uint8_t joy2_tx_complete = 1;

usb_kb_report_ready_cb_f kb_report_ready_cb = NULL;
usb_mouse_report_ready_cb_f mouse_report_ready_cb = NULL;
usb_joy_report_ready_cb_f joy_report_ready_cb = NULL;

uint8_t kb_led_state = 0;

#ifdef CONFIG_ARCHID_VCD

uint8_t tx_buf[USB_VCD_TX_BUF_SIZE];
uint8_t rx_buf[USB_VCD_RX_BUF_SIZE];
ringbuf tx_rb;
ringbuf rx_rb;
usb_serial_rx_cb rx_cb = NULL;
uint8_t USB_Tx_State = 0;

#endif

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

void USB_Cable_Config(FunctionalState NewState) {
#ifdef CONFIG_HY_TEST_BOARD
  if (NewState != DISABLE) {
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
  } else {
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
  }
#else
  if (NewState != DISABLE) {
    gpio_config(PORTB, PIN9, CLK_2MHZ, OUT, AF0, PUSHPULL, NOPULL);
    gpio_enable(PORTB, PIN9);
  } else {
    gpio_config(PORTB, PIN9, CLK_2MHZ, IN, AF0, OPENDRAIN, NOPULL);
  }
#endif
}

void USB_ARC_set_kb_callback(usb_kb_report_ready_cb_f cb) {
  kb_report_ready_cb = cb;
}

void USB_ARC_set_mouse_callback(usb_mouse_report_ready_cb_f cb) {
  mouse_report_ready_cb = cb;
}

void USB_ARC_set_joystick_callback(usb_joy_report_ready_cb_f cb) {
  joy_report_ready_cb = cb;
}

bool USB_ARC_KB_can_tx(void) {
  return kb_tx_complete != 0;
}

bool USB_ARC_MOUSE_can_tx(void) {
  return mouse_tx_complete != 0;
}

bool USB_ARC_JOYSTICK_can_tx(usb_joystick j) {
  return j == JOYSTICK1 ? joy1_tx_complete : joy2_tx_complete;
}

void USB_ARC_KB_tx(usb_kb_report *report)
{
  // byte 0:   modifiers
  // byte 1:   reserved (0x00)
  // byte 2-x: keypresses
  report->reserved = 0;

  uint32_t spoon_guard = 1000000;
  while(kb_tx_complete==0 && --spoon_guard);
  ASSERT(spoon_guard > 0);

  /* Reset the control token to inform upper layer that a transfer is ongoing */
  kb_tx_complete = 0;

  /* Copy keyboard vector info in ENDP1 Tx Packet Memory Area*/
  USB_SIL_Write(EP1_IN, report->raw, sizeof(report->raw));

  /* Enable endpoint for transmission */
  SetEPTxValid(ENDP1);

}

void USB_ARC_MOUSE_tx(usb_mouse_report *report)
{
  uint32_t spoon_guard = 1000000;
  while(mouse_tx_complete==0 && --spoon_guard);
  ASSERT(spoon_guard > 0);

  /* Reset the control token to inform upper layer that a transfer is ongoing */
  mouse_tx_complete = 0;

  /* Copy mouse position info in ENDP2 Tx Packet Memory Area*/
  USB_SIL_Write(EP2_IN, report->raw, sizeof(report->raw));

  /* Enable endpoint for transmission */
  SetEPTxValid(ENDP2);

}

void USB_ARC_JOYSTICK_tx(usb_joystick j, usb_joystick_report *report)
{
  uint32_t spoon_guard = 1000000;
  if (j == JOYSTICK1) {
    while(joy1_tx_complete==0 && --spoon_guard);
  } else {
    while(joy2_tx_complete==0 && --spoon_guard);
  }
  ASSERT(spoon_guard > 0);

  /* Reset the control token to inform upper layer that a transfer is ongoing */
  if (j == JOYSTICK1) {
    joy1_tx_complete= 0;
  } else {
    joy2_tx_complete = 0;
  }

  /* Copy joystick info in ENDP3/4 Tx Packet Memory Area*/
  USB_SIL_Write(j == JOYSTICK1 ? EP3_IN : EP4_IN, report->raw, sizeof(report->raw));

  /* Enable endpoint for transmission */
  SetEPTxValid(j == JOYSTICK1 ? ENDP3 : ENDP4);
}

void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*)ID1;
  Device_Serial1 = *(uint32_t*)ID2;
  Device_Serial2 = *(uint32_t*)ID3;

  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &ARC_string_serial[2] , 8);
    IntToUnicode (Device_Serial1, &ARC_string_serial[18], 4);
  }
}

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len) {
  uint8_t idx = 0;

  for (idx = 0; idx < len; idx++) {
    if (((value >> 28)) < 0xA) {
      pbuf[2 * idx] = (value >> 28) + '0';
    } else {
      pbuf[2 * idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[2 * idx + 1] = 0;
  }
}

void USB_ARC_init(void) {
#ifdef CONFIG_ARCHID_VCD
  ringbuf_init(&tx_rb, tx_buf, sizeof(tx_buf));
  ringbuf_init(&rx_rb, rx_buf, sizeof(rx_buf));
#endif
}

void USB_ARC_start(void) {
  USB_Init();
}

#ifdef CONFIG_ARCHID_VCD
u16_t USB_SER_rx_avail(void) {
  return ringbuf_available(&rx_rb);
}

s32_t USB_SER_rx_char(u8_t *c) {
  return ringbuf_getc(&rx_rb, c);
}

s32_t USB_SER_rx_buf(u8_t *buf, u16_t len) {
  return ringbuf_get(&rx_rb, buf, len);
}

s32_t USB_SER_tx_char(u8_t c) {
  return ringbuf_putc(&tx_rb, c);
}

s32_t USB_SER_tx_buf(u8_t *buf, u16_t len) {
  return ringbuf_put(&tx_rb, buf, len);
}

void USB_SER_set_rx_callback(usb_serial_rx_cb cb, void *arg) {
  rx_cb = cb;
}

void USB_SER_get_rx_callback(usb_serial_rx_cb *cb, void **arg) {
  if (cb) *cb = rx_cb;
}

bool USB_SER_assure_tx(bool on) {
  return FALSE;
}

void USB_SER_tx_drain(void) {
  ringbuf_clear(&tx_rb);
}

void USB_SER_tx_flush(void) {
}

void Handle_USBAsynchXfer(void) {

  if (USB_Tx_State != 1) {
    u8_t *buf;
    int avail = ringbuf_available_linear(&tx_rb, &buf);
    if (avail == 0) {
      USB_Tx_State = 0;
      return;
    }

    if (avail > VIRTUAL_COM_PORT_DATA_SIZE) {
      avail = VIRTUAL_COM_PORT_DATA_SIZE;
    }

    USB_Tx_State = 1;
    UserToPMABufferCopy(buf, ENDP7_TXADDR, avail);
    ringbuf_get(&tx_rb, 0, avail);
    SetEPTxCount(ENDP7, avail);
    SetEPTxValid(ENDP7 );
  }

}
#endif

