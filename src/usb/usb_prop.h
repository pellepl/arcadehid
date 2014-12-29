/**
  ******************************************************************************
  * @file    usb_prop.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   All processing related to Joystick Mouse demo
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
#ifndef __USB_PROP_H
#define __USB_PROP_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum _HID_REQUESTS
{
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,

  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
} HID_REQUESTS;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void KB_init(void);
void KB_Reset(void);
void KB_SetConfiguration(void);
void KB_SetDeviceAddress (void);
void KB_Status_In (void);
void KB_Status_Out (void);
RESULT KB_Data_Setup(uint8_t);
RESULT KB_NoData_Setup(uint8_t);
RESULT KB_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *KB_GetDeviceDescriptor(uint16_t );
uint8_t *KB_GetConfigDescriptor(uint16_t);
uint8_t *KB_GetStringDescriptor(uint16_t);
RESULT KB_SetProtocol(void);
uint8_t *KB_GetProtocolValue(uint16_t Length);
RESULT KB_SetProtocol(void);
uint8_t *KB_GetReportDescriptor(uint16_t Length);
uint8_t *KB_GetHIDDescriptor(uint16_t Length);

/* Exported define -----------------------------------------------------------*/
#define KB_GetConfiguration          NOP_Process
//#define KB_SetConfiguration          NOP_Process
#define KB_GetInterface              NOP_Process
#define KB_SetInterface              NOP_Process
#define KB_GetStatus                 NOP_Process
#define KB_ClearFeature              NOP_Process
#define KB_SetEndPointFeature        NOP_Process
#define KB_SetDeviceFeature          NOP_Process
//#define KB_SetDeviceAddress          NOP_Process

#define REPORT_DESCRIPTOR                  0x22

#endif /* __USB_PROP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
