/*
 * gpio_map.c
 *
 *  Created on: Jan 24, 2015
 *      Author: petera
 */

#include "gpio_map.h"

static const gpio_pin_map pin_map[APP_CONFIG_PINS] = {
#ifdef CONFIG_HY_TEST_BOARD
    {.port = PORTE, .pin = PIN2 },
    {.port = PORTE, .pin = PIN3 },
    {.port = PORTE, .pin = PIN4 },
    {.port = PORTE, .pin = PIN5 },
#else
    //TODO
#endif
};

static const gpio_pin_map led_map = {
#ifdef CONFIG_HY_TEST_BOARD
    .port =PORTC, .pin =PIN6
#else
    //TODO
#endif
};

const gpio_pin_map *GPIO_MAP_get_pin_map(void) {
  return &pin_map[0];
}

const gpio_pin_map *GPIO_MAP_get_led_map(void) {
  return &led_map;
}

