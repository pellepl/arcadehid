#include "system_config.h"
#include "usb_kb.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_hw_config.h"
#include "usb_pwr.h"

ErrorStatus HSEStartUpStatus;
/* Extern variables ----------------------------------------------------------*/
volatile uint8_t kb_tx_complete = 1;

uint8_t USB_Tx_State = 0;
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

void Enter_LowPowerMode(void) {
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;

#ifdef CONFIG_HY_TEST_BOARD
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif
}

void Leave_LowPowerMode(void) {
  DEVICE_INFO *pInfo = &Device_Info;

  /* Set the device state to the correct state */
  if (pInfo->Current_Configuration != 0) {
    /* Device configured */
    bDeviceState = CONFIGURED;
  } else {
    bDeviceState = ATTACHED;
  }
#ifdef CONFIG_HY_TEST_BOARD
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif
}

void USB_Cable_Config(FunctionalState NewState) {
#ifdef CONFIG_HY_TEST_BOARD
  if (NewState != DISABLE) {
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
  } else {
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
  }
#endif
}

void USB_KB_tx(usb_report *report)
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

  /* Copy mouse position info in ENDP1 Tx Packet Memory Area*/
  USB_SIL_Write(EP1_IN, report->raw, sizeof(report->raw));

  /* Enable endpoint for transmission */
  SetEPTxValid(ENDP1);

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
    IntToUnicode (Device_Serial0, &KB_string_serial[2] , 8);
    IntToUnicode (Device_Serial1, &KB_string_serial[18], 4);
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

void USB_KB_init(void) {
  USB_Init();
}

