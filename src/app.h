/*
 * app.h
 *
 *  Created on: Jan 2, 2014
 *      Author: petera
 */

#ifndef APP_H_
#define APP_H_

#include "system.h"
#include "def_config.h"

void APP_init(void);
void APP_timer(void);
void APP_cfg_set_pin(def_config *cfg);
def_config *APP_cfg_get_pin(u8_t pin);
void APP_cfg_set_debounce_cycles(u8_t cycles);
u8_t APP_cfg_get_debounce_cycles(void);
void APP_cfg_set_mouse_delta_ms(time ms);
time APP_cfg_get_mouse_delta_ms(void);
void APP_cfg_set_acc_pos_speed(u16_t speed);
u16_t APP_cfg_get_acc_pos_speed(void);
void APP_cfg_set_acc_wheel_speed(u16_t speed);
u16_t APP_cfg_get_acc_wheel_speed(void);

#endif /* APP_H_ */
