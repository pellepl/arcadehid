/*
 * app_annoyatron.h
 *
 *  Created on: Mar 10, 2015
 *      Author: petera
 */

#ifndef APP_ANNOYATRON_H_
#define APP_ANNOYATRON_H_

void annoy_loop_enter(int loops);
void annoy_loop_exit(void);
void annoy_wait(int time);
void annoy_stop(void);
void annoy_set_cycle(u32_t ms);
void annoy_init(void);

#endif /* APP_ANNOYATRON_H_ */
