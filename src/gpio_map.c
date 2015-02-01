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
    {.port = PORTB, .pin = PIN12 }, //1
    {.port = PORTB, .pin = PIN13 }, //2
    {.port = PORTB, .pin = PIN14 }, //3
    {.port = PORTB, .pin = PIN15 }, //4
    {.port = PORTA, .pin = PIN8 }, //5
    {.port = PORTA, .pin = PIN9 }, //6
    {.port = PORTA, .pin = PIN10 }, //7
    {.port = PORTA, .pin = PIN15 }, //8
    {.port = PORTB, .pin = PIN3 }, //9
    {.port = PORTB, .pin = PIN4 }, //10
    {.port = PORTB, .pin = PIN5 }, //11
    {.port = PORTB, .pin = PIN6 }, //12
    {.port = PORTB, .pin = PIN7 }, //13
    {.port = PORTB, .pin = PIN11 }, //14
    {.port = PORTB, .pin = PIN10 }, //15
    {.port = PORTB, .pin = PIN2 }, //16
    {.port = PORTB, .pin = PIN1 }, //17
    {.port = PORTB, .pin = PIN0 }, //18
    {.port = PORTA, .pin = PIN7 }, //19
    {.port = PORTA, .pin = PIN6 }, //20
    {.port = PORTA, .pin = PIN5 }, //21
    {.port = PORTA, .pin = PIN4 }, //22
    {.port = PORTA, .pin = PIN1 }, //23
    {.port = PORTA, .pin = PIN0 }, //24
    {.port = PORTC, .pin = PIN14 }, //25
    {.port = PORTC, .pin = PIN13 }, //26
#endif
};

static const gpio_pin_map led_map = {
#ifdef CONFIG_HY_TEST_BOARD
    .port =PORTC, .pin =PIN6
#else
    .port =PORTC, .pin =PIN15
#endif
};

const gpio_pin_map *GPIO_MAP_get_pin_map(void) {
  return &pin_map[0];
}

const gpio_pin_map *GPIO_MAP_get_led_map(void) {
  return &led_map;
}

