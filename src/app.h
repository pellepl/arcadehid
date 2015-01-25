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
void APP_define_pin(def_config *cfg);

#endif /* APP_H_ */
