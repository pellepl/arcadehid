/**
  ******************************************************************************
  * @file    usb_desc.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Descriptor Header for Joystick Mouse Demo
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_DESC_H
#define __USB_DESC_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define HID_DESCRIPTOR_TYPE                     0x21
#define ARC_SIZE_HID_DESC                   0x09
#define ARC_OFFS_HID_DESC                   0x12

#define ARC_SIZE_DEVICE_DESC                18
#ifndef CONFIG_ANNOYATRON
#ifdef CONFIG_ARCHID_VCD
#define ARC_SIZE_CONFIG_DESC                59+50+58
#else
#define ARC_SIZE_CONFIG_DESC                59+50
#endif
#else // CONFIG_ANNOYATRON
#define ARC_SIZE_CONFIG_DESC                59
#endif // CONFIG_ANNOYATRON
#define ARC_KB_SIZE_REPORT_DESC             62
#define ARC_MOUSE_SIZE_REPORT_DESC          74
#define ARC_JOYSTICK_SIZE_REPORT_DESC       48
#define ARC_SIZE_STRING_LANGID              4
#define ARC_SIZE_STRING_VENDOR              30
#define ARC_SIZE_STRING_PRODUCT             31
#define ARC_SIZE_STRING_SERIAL              26

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

#ifdef CONFIG_ARCHID_VCD

#define VIRTUAL_COM_PORT_DATA_SIZE              64
#define VIRTUAL_COM_PORT_INT_SIZE               8

#endif

/* Exported functions ------------------------------------------------------- */
extern const uint8_t ARC_device_descriptor[ARC_SIZE_DEVICE_DESC];
extern const uint8_t ARC_config_descriptor[ARC_SIZE_CONFIG_DESC];
extern const uint8_t ARC_KB_report_descriptor[ARC_KB_SIZE_REPORT_DESC];
extern const uint8_t ARC_MOUSE_report_descriptor[ARC_MOUSE_SIZE_REPORT_DESC];
extern const uint8_t ARC_JOYSTICK_report_descriptor[ARC_JOYSTICK_SIZE_REPORT_DESC];
extern const uint8_t ARC_string_lang_ID[ARC_SIZE_STRING_LANGID];
extern const uint8_t ARC_string_vendor[ARC_SIZE_STRING_VENDOR];
extern const uint8_t ARC_string_product[ARC_SIZE_STRING_PRODUCT];
extern uint8_t ARC_string_serial[ARC_SIZE_STRING_SERIAL];

#endif /* __USB_DESC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
