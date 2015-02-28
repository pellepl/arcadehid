/*
 * niffs_impl.h
 *
 *  Created on: Feb 26, 2015
 *      Author: petera
 */

#ifndef SRC_NIFFS_IMPL_H_
#define SRC_NIFFS_IMPL_H_

#include "niffs.h"

#define FS_FILE_VERSION   2

#define ERR_NIFFS_HAL     -11050

typedef struct {
  u16_t file_version;
  u8_t nbr_of_pins;
  u8_t defs_per_pin;

  u8_t debounce_cycles;
  time mouse_delta_ms;
  u16_t acc_pos_speed;
  u16_t acc_wheel_speed;
  time joystick_delta_ms;
  u16_t joystick_acc_speed;
} file_config_hdr;

int FS_mount(void);
void FS_dump(void);
void FS_ls(void);
int FS_save_config(char *name);
int FS_load_config(char *name);
int FS_rm_config(char *name);
int FS_chk(void);
int FS_format(void);


#endif /* SRC_NIFFS_IMPL_H_ */
