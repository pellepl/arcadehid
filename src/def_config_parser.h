/*
 * def_config_parser.h
 *
 *  Created on: Jan 20, 2015
 *      Author: petera
 */

#ifndef SRC_DEF_CONFIG_PARSER_H_
#define SRC_DEF_CONFIG_PARSER_H_

#include "system.h"
#include "def_config.h"
#include "miniutils.h"

#define KEYPARSERR(...) print(__VA_ARGS__)

bool def_config_parse(def_config *pindef, const char *str, u16_t len);
void def_config_print(def_config *pindef);

#endif /* SRC_DEF_CONFIG_PARSER_H_ */
