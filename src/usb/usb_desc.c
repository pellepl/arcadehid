/**
  ******************************************************************************
  * @file    usb_desc.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Descriptors for Joystick Mouse Demo
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_desc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* USB Standard Device Descriptor */
const uint8_t KB_device_descriptor[KB_SIZE_DEVICE_DESC] =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    0x40,                       /*bMaxPacketSize 64*/
    0xde,                       /*idVendor (0xfede)*/
    0xfe,
    0xda,                       /*idProduct = 0xbeda*/
    0xbe,
    0x30,                       /*bcdDevice rel. 2.00*/
    0x02,
    1,                          /*Index of string descriptor describing
                                                  manufacturer */
    2,                          /*Index of string descriptor describing
                                                 product*/
    3,                          /*Index of string descriptor describing the
                                                 device serial number */
    0x01                        /*bNumConfigurations*/
  }
  ; /* Device Descriptor */


/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t KB_config_descriptor[KB_SIZE_CONFIG_DESC] =
  {
    0x09, /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    KB_SIZE_CONFIG_DESC,               /* wTotalLength: Bytes returned */
    0x00,
    0x01,         /*bNumInterfaces: 1 interface*/
    0x01,         /*bConfigurationValue: Configuration value*/
    0x00,         /*iConfiguration: Index of string descriptor describing
                                     the configuration*/
    0xE0,         /*bmAttributes: Self powered */
    0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/

    /************** Descriptor of interface ****************/
    /* 09 */
    0x09,         /*bLength: Interface Descriptor size*/
    USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
    0x00,         /*bInterfaceNumber: Number of Interface*/
    0x00,         /*bAlternateSetting: Alternate setting*/
    0x01,         /*bNumEndpoints*/
    0x03,         /*bInterfaceClass: HID*/
    0x01,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
    0x01,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
    0,            /*iInterface: Index of string descriptor*/
    /******************** Descriptor of HID ********************/
    /* 18 */
    0x09,         /*bLength: HID Descriptor size*/
    HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
    0x11,         /*bcdHID: HID Class Spec release number*/
    0x01,
    0x00,         /*bCountryCode: Hardware target country*/
    0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
    0x22,         /*bDescriptorType*/
    KB_SIZE_REPORT_DESC,/*wItemLength: Total length of Report descriptor*/
    0x00,
    /******************** Descriptor of endpoint ********************/
    /* 27 */
    0x07,          /*bLength: Endpoint Descriptor size*/
    USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

    0x81,          /*bEndpointAddress: Endpoint Address (IN)*/
    0x03,          /*bmAttributes: Interrupt endpoint*/
    0x40,          /*wMaxPacketSize: 64 bytes max ///8 Byte max */
    0x00,
    24,          /*bInterval: Polling Interval (24 ms)*/
    /* 34 */
  }; /* config descriptor */

const uint8_t KB_report_descriptor[KB_SIZE_REPORT_DESC] =
  {
      0x05, 0x01,                         // Usage Page (Generic Desktop)
      0x09, 0x06,                         // Usage (Keyboard)
      0xA1, 0x01,                         // Collection (Application)
      0x05, 0x07,                         //     Usage Page (Key Codes)
      0x19, 0xe0,                         //     Usage Minimum (224)
      0x29, 0xe7,                         //     Usage Maximum (231)
      0x15, 0x00,                         //     Logical Minimum (0)
      0x25, 0x01,                         //     Logical Maximum (1)

      0x75, 0x01,                         //     Report Size (1)
      0x95, 0x08,                         //     Report Count (8)
      0x81, 0x02,                         //     Input (Data, Variable, Absolute)

      0x95, 0x01,                         //     Report Count (1)
      0x75, 0x08,                         //     Report Size (8)
      0x81, 0x01,                         //     Input (Constant) reserved byte(1)

      0x95, USB_REPORT_KEYMAP_SIZE,            //     Report Count (normally 6)
      0x75, 0x08,                         //     Report Size (8)
      0x26, 0xff, 0x00,
      0x05, 0x07,                         //     Usage Page (Key codes)
      0x19, 0x00,                         //     Usage Minimum (0)
      0x29, 0x91,
      0x81, 0x00,                         //     Input (Data, Array) Key array(6 bytes)

      0x95, 0x03,                         //     Report Count (3)
      0x75, 0x01,                         //     Report Size (1)
      0x05, 0x08,                         //     Usage Page (Page# for LEDs)
      0x19, 0x01,                         //     Usage Minimum (1)
      0x29, 0x03,                         //     Usage Maximum (3)
      0x91, 0x02,                         //     Output (Data, Variable, Absolute), Led report
      0x95, 0x05,                         //     Report Count (5)
      0x75, 0x01,                         //     Report Size (1)
      0x91, 0x01,                         //     Output (Data, Variable, Absolute), Led report padding

      0xC0                                // End Collection (Application)

  };

/* USB String Descriptors (optional) */
const uint8_t KB_string_lang_ID[KB_SIZE_STRING_LANGID] =
  {
    KB_SIZE_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04
  }
  ; /* LangID = 0x0409: U.S. English */

const uint8_t KB_string_vendor[KB_SIZE_STRING_VENDOR] =
  {
    KB_SIZE_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,  /* bDescriptorType*/

    'p', 0, 'e', 0, 'l', 0, 'l', 0, 'e', 0, 'p', 0, 'l', 0,
    'u', 0, 't', 0, 't', 0, '.', 0, 'c', 0, 'o', 0, 'm', 0
  };

const uint8_t KB_string_product[KB_SIZE_STRING_PRODUCT] =
  {
    KB_SIZE_STRING_PRODUCT,          /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    'A', 0, 'r', 0, 'c', 0, 'a', 0, 'd', 0, 'e', 0, '-', 0,
    'k', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0
  };
uint8_t KB_string_serial[KB_SIZE_STRING_SERIAL] =
  {
    KB_SIZE_STRING_SERIAL,           /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    'p', 0, 'e', 0, 'l', 0, 'l', 0, 'e', 0
  };

