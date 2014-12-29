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
#define KB_SIZE_HID_DESC                   0x09
#define KB_OFFS_HID_DESC                   0x12

#define KB_SIZE_DEVICE_DESC                18
#define KB_SIZE_CONFIG_DESC                34
#define KB_SIZE_REPORT_DESC                62
#define KB_SIZE_STRING_LANGID              4
#define KB_SIZE_STRING_VENDOR              30
#define KB_SIZE_STRING_PRODUCT             32
#define KB_SIZE_STRING_SERIAL              26

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

/* Exported functions ------------------------------------------------------- */
extern const uint8_t KB_device_descriptor[KB_SIZE_DEVICE_DESC];
extern const uint8_t KB_config_descriptor[KB_SIZE_CONFIG_DESC];
extern const uint8_t KB_report_descriptor[KB_SIZE_REPORT_DESC];
extern const uint8_t KB_string_lang_ID[KB_SIZE_STRING_LANGID];
extern const uint8_t KB_string_vendor[KB_SIZE_STRING_VENDOR];
extern const uint8_t KB_string_product[KB_SIZE_STRING_PRODUCT];
extern uint8_t KB_string_serial[KB_SIZE_STRING_SERIAL];

#endif /* __USB_DESC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
