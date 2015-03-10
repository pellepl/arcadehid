/*
 * cli.h
 *
 *  Created on: Jul 24, 2012
 *      Author: petera
 */

#ifndef CLI_H_
#define CLI_H_

#include "system.h"

void CLI_init();
void CLI_timer();
void CLI_parse(u32_t len, u8_t *buf);

#endif /* CLI_H_ */
