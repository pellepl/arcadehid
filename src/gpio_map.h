/*
 * gpio_map.h
 *
 *  Created on: Jan 24, 2015
 *      Author: petera
 */

#ifndef SRC_GPIO_MAP_H_
#define SRC_GPIO_MAP_H_

#include "system.h"
#include "gpio.h"

typedef struct {
  gpio_port port;
  gpio_pin pin;
} gpio_pin_map;

const gpio_pin_map *GPIO_MAP_get_pin_map(void);
const gpio_pin_map *GPIO_MAP_get_led_map(void);

#endif /* SRC_GPIO_MAP_H_ */
