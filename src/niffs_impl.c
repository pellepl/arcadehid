/*
 * niffs_impl.c
 *
 *  Created on: Feb 26, 2015
 *      Author: petera
 */

#include "system.h"
#include "niffs_impl.h"
#include "niffs.h"
#include "gpio_map.h"
#include "def_config.h"
#include "def_config_parser.h"
#include "app.h"

/* Flash Control Register bits */
#define CR_PG_Set                ((uint32_t)0x00000001)
#define CR_PG_Reset              ((uint32_t)0x00001FFE)
#define CR_PER_Set               ((uint32_t)0x00000002)
#define CR_PER_Reset             ((uint32_t)0x00001FFD)
#define CR_MER_Set               ((uint32_t)0x00000004)
#define CR_MER_Reset             ((uint32_t)0x00001FFB)
#define CR_OPTPG_Set             ((uint32_t)0x00000010)
#define CR_OPTPG_Reset           ((uint32_t)0x00001FEF)
#define CR_OPTER_Set             ((uint32_t)0x00000020)
#define CR_OPTER_Reset           ((uint32_t)0x00001FDF)
#define CR_STRT_Set              ((uint32_t)0x00000040)
#define CR_LOCK_Set              ((uint32_t)0x00000080)

/* FLASH Mask */
#define RDPRT_Mask               ((uint32_t)0x00000002)
#define WRP0_Mask                ((uint32_t)0x000000FF)
#define WRP1_Mask                ((uint32_t)0x0000FF00)
#define WRP2_Mask                ((uint32_t)0x00FF0000)
#define WRP3_Mask                ((uint32_t)0xFF000000)
#define OB_USER_BFB2             ((uint16_t)0x0008)

/* FLASH Keys */
#define RDP_Key                  ((uint16_t)0x00A5)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

/* Delay definition */
#define FLASH_TIMEOUT            ((uint32_t)0x000B0000)

#define SECTOR_SIZE       (1024)
#define NIFFS_FLASH_LEN   (32*1024)
#define NIFFS_FLASH_BASE  (FLASH_BASE + 128*1024-NIFFS_FLASH_LEN)

typedef enum {
  FLASH_OK = 0,
  FLASH_ERR_BUSY,
  FLASH_ERR_WRITE_PROTECTED,
  FLASH_ERR_TIMEOUT,
  FLASH_ERR_OTHER
} FLASH_res;

static void _flash_open();
static void _flash_close();
static FLASH_res _flash_erase(u32_t addr);
static FLASH_res _flash_write_hword(u32_t addr, u16_t data);

static niffs fs;
static u8_t niffs_buf[128];
static niffs_file_desc niffs_fd[4];

static int niffs_hal_erase(u8_t *addr, u32_t len) {
  _flash_open();
  FLASH_res r = _flash_erase((u32_t)addr);
  _flash_close();
  return r == FLASH_OK ? NIFFS_OK : ERR_NIFFS_HAL;
}
static int niffs_hal_write(u8_t *addr, const u8_t *src, u32_t len) {
  _flash_open();
  FLASH_res r = FLASH_OK;
  while (r == FLASH_OK && len > 0) {
    if (len == 1) {
      u16_t d = 0xff00 | (*src);
      r = _flash_write_hword((u32_t)addr, d);
      addr += 2;
      len--;
      src++;
    } else {
      u16_t d = (src[1] << 8) | (src[0]);
      r = _flash_write_hword((u32_t)addr, d);
      addr += 2;
      len -= 2;
      src += 2;
    }
  }
  _flash_close();
  return r == FLASH_OK ? NIFFS_OK : ERR_NIFFS_HAL;
}

int FS_mount(void) {
  int res;
  res = NIFFS_init(&fs,
    (u8_t *)NIFFS_FLASH_BASE,
    NIFFS_FLASH_LEN / SECTOR_SIZE,
    SECTOR_SIZE,
    128,
    niffs_buf, sizeof(niffs_buf),
    niffs_fd, sizeof(niffs_fd)/sizeof(niffs_file_desc),
    niffs_hal_erase, niffs_hal_write,
    0);
  if (res != NIFFS_OK) return res;
  res = NIFFS_mount(&fs);
  if (res == ERR_NIFFS_NOT_A_FILESYSTEM) {
    DBG(D_FS, D_INFO, "not a fs, formatting..\n");
    res = NIFFS_format(&fs);
    if (res != NIFFS_OK) return res;
    res = NIFFS_mount(&fs);
  }
  return res;
}

void FS_dump(void) {
  NIFFS_dump(&fs);
}

void FS_ls(void) {
  niffs_DIR d;
  struct niffs_dirent e;
  struct niffs_dirent *pe = &e;
  u32_t files = 0;

  NIFFS_opendir(&fs, "/", &d);
  while ((pe = NIFFS_readdir(&d, pe))) {
    print("  %s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
    files++;
  }
  NIFFS_closedir(&d);
  niffs_info info;
  NIFFS_info(&fs, &info);
  print("  %i bytes used of %i total in %i file%c\n", info.used_bytes, info.total_bytes, files, files == 1 ? ' ' : 's');
  if (info.overflow) {
    print("WARNING: filesystem crammed. Remove files and run check.\n");
  }
}

int FS_save_config(char *name) {
  int res;
  int fd;
  fd = NIFFS_open(&fs, name, NIFFS_O_WRONLY | NIFFS_O_CREAT | NIFFS_O_TRUNC | NIFFS_O_APPEND, 0);
  if (fd < NIFFS_OK) {
    DBG(D_FS, D_INFO, "save err: open %i\n", fd);
    return fd;
  }
  file_config_hdr hdr =
    { .file_version = FS_FILE_VERSION,
      .nbr_of_pins = APP_CONFIG_PINS,
      .defs_per_pin = APP_CONFIG_DEFS_PER_PIN
    };
  hdr.debounce_cycles = APP_cfg_get_debounce_cycles();
  hdr.mouse_delta_ms = APP_cfg_get_mouse_delta_ms();
  hdr.acc_pos_speed = APP_cfg_get_acc_pos_speed();
  hdr.acc_wheel_speed = APP_cfg_get_acc_wheel_speed();
  hdr.joystick_delta_ms = APP_cfg_get_joystick_delta_ms();
  hdr.joystick_acc_speed = APP_cfg_get_joystick_acc_speed();

  res = NIFFS_write(&fs, fd, (u8_t *)&hdr, sizeof(hdr));
  if (res < NIFFS_OK) {
    DBG(D_FS, D_INFO, "save err: write hdr %i\n", res);
    NIFFS_close(&fs, fd);
    return res;
  }
  def_config *cfg = APP_cfg_get_pin(0);
    res = NIFFS_write(&fs, fd, (u8_t *)cfg, sizeof(def_config)*hdr.nbr_of_pins);
  if (res < NIFFS_OK) {
    DBG(D_FS, D_INFO, "save err: write cfg %i\n", res);
    NIFFS_close(&fs, fd);
    return res;
  }

  NIFFS_close(&fs, fd);
  return res < NIFFS_OK ? res : NIFFS_OK;
}

int FS_load_config(char *name) {
  int res = NIFFS_OK;
#ifndef CONFIG_ANNOYATRON
  int fd;
  fd = NIFFS_open(&fs, name, NIFFS_O_RDONLY , 0);
  if (fd < NIFFS_OK) {
    DBG(D_FS, D_INFO, "load err: open %i\n", fd);
    return fd;
  }
  file_config_hdr hdr;
  res = NIFFS_read(&fs, fd, (u8_t *)&hdr, sizeof(hdr));
  if (res < NIFFS_OK) {
    DBG(D_FS, D_INFO, "load err: read hdr %i\n", res);
    NIFFS_close(&fs, fd);
    return res;
  }
  if (hdr.file_version != FS_FILE_VERSION) {
    print("wrong file version\n");
    NIFFS_close(&fs, fd);
    return 0;
  }
  if (hdr.defs_per_pin != APP_CONFIG_DEFS_PER_PIN) {
    print("defs per pin mismatch\n");
    NIFFS_close(&fs, fd);
    return 0;
  }
  if (hdr.nbr_of_pins != APP_CONFIG_PINS) {
    print("nbr of pin mismatch\n");
    NIFFS_close(&fs, fd);
    return 0;
  }

  APP_cfg_set_debounce_cycles(hdr.debounce_cycles);
  APP_cfg_set_mouse_delta_ms(hdr.mouse_delta_ms);
  APP_cfg_set_acc_pos_speed(hdr.acc_pos_speed);
  APP_cfg_set_acc_wheel_speed(hdr.acc_wheel_speed);
  APP_cfg_set_joystick_delta_ms(hdr.joystick_delta_ms);
  APP_cfg_set_joystick_acc_speed(hdr.joystick_acc_speed);

  u8_t pin;
  for (pin = 0; pin < APP_CONFIG_PINS; pin++) {
    def_config cfg;
    res = NIFFS_read(&fs, fd, (u8_t *)&cfg, sizeof(def_config));
    if (res < NIFFS_OK) {
      DBG(D_FS, D_INFO, "read err: read cfg %i\n", res);
      NIFFS_close(&fs, fd);
      return res;
    }
    APP_cfg_set_pin(&cfg);
    if (cfg.pin) def_config_print(&cfg);
  }

  NIFFS_close(&fs, fd);
#endif
  return res < NIFFS_OK ? res : NIFFS_OK;
}

int FS_rm_config(char *name) {
  int res = NIFFS_remove(&fs, name);
  return res;
}

int FS_chk(void) {
  int res = NIFFS_unmount(&fs);
  if (res != NIFFS_OK) return res;
  res = NIFFS_chk(&fs);
  if (res != NIFFS_OK) return res;
  res = NIFFS_mount(&fs);
  return res;
}

int FS_format(void) {
  int res = NIFFS_unmount(&fs);
  if (res != NIFFS_OK) return res;
  res = NIFFS_format(&fs);
  if (res != NIFFS_OK) return res;
  res = NIFFS_mount(&fs);
  return res;
}

int FS_create(char *name) {
  return NIFFS_creat(&fs, name, 0);
}

int FS_append(char *name, char *line) {
  int fd = NIFFS_open(&fs, name, NIFFS_O_APPEND | NIFFS_O_CREAT | NIFFS_O_RDWR, 0);
  if (fd < 0) return fd;
  int res = NIFFS_write(&fs, fd, (u8_t *)line, strlen(line));
  if (res < NIFFS_OK) return res;
  u8_t nl = '\n';
  res = NIFFS_write(&fs, fd, &nl, 1);
  if (res < NIFFS_OK) return res;
  return NIFFS_close(&fs, fd);
}

int FS_less(char *name) {
  int fd = NIFFS_open(&fs, name, NIFFS_O_RDONLY, 0);
  if (fd < 0) return fd;
  char buf[32];
  int res;
  do {
    res = NIFFS_read(&fs, fd, (u8_t *)buf, sizeof(buf)-1);
    if (res > 0) {
      buf[res] = 0;
      print("%s", buf);
    }
  } while (res >= NIFFS_OK);
  if (res == ERR_NIFFS_END_OF_FILE) res = NIFFS_OK;
  if (res > 0) res = 0;
  print("\n");
  NIFFS_close(&fs, fd);
  return res;
}

int FS_rename(char *oldname, char *newname) {
  return NIFFS_rename(&fs, oldname, newname);
}


niffs *FS_get_fs(void) {
  return &fs;
}


// flash hal

static FLASH_res _flash_status(void) {
  FLASH_res res = FLASH_OK;

  if ((FLASH->SR & FLASH_FLAG_BANK1_BSY) == FLASH_FLAG_BSY) {
    res = FLASH_ERR_BUSY;
  } else if ((FLASH->SR & FLASH_FLAG_BANK1_PGERR) != 0) {
    DBG(D_FS, D_WARN, "FLASH_ERR_OTHER\n");
    res = FLASH_ERR_OTHER;
  } else if ((FLASH->SR & FLASH_FLAG_BANK1_WRPRTERR) != 0) {
    DBG(D_FS, D_WARN, "FLASH_ERR_WRP\n");
    res = FLASH_ERROR_WRP;
  } else {
    res = FLASH_OK;
  }

  return res;
}

static FLASH_res _flash_wait(u32_t timeout) {
  FLASH_res res;

  while (((res = _flash_status()) == FLASH_ERR_BUSY) && timeout) {
    timeout--;
  }

  if (timeout == 0) {
    DBG(D_FS, D_WARN, "FLASH_ERR_TIMEOUT\n");
    res = FLASH_ERR_TIMEOUT;
  }

  return res;
}

void _flash_get_sector(u32_t phys_addr, u32_t *sect_addr, u32_t *sect_len) {
  *sect_addr = phys_addr & (~(FLASH_PAGE_SIZE-1));
  *sect_len = FLASH_PAGE_SIZE;
}

static void _flash_open() {
  // unlock flash block

  // FLASH_UnlockBank1()
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  // clear flags
  FLASH->SR = (FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
}

static void _flash_close() {
  //FLASH_LockBank1();
  FLASH->CR |= CR_LOCK_Set;
}

static FLASH_res _flash_erase(u32_t addr) {
  // FLASH_ErasePage(BANK2_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
  const gpio_pin_map *led = GPIO_MAP_get_led_map();
  gpio_enable(led->port, led->pin);
  FLASH_res res = _flash_wait(FLASH_TIMEOUT );

  if (res == FLASH_OK) {
    /* if the previous operation is completed, proceed to erase the page */
    FLASH->CR |= CR_PER_Set;
    FLASH->AR = addr;
    FLASH->CR |= CR_STRT_Set;

    res = _flash_wait(FLASH_TIMEOUT );

    /* Disable the PER Bit */
    FLASH->CR &= CR_PER_Reset;
  }
  gpio_disable(led->port, led->pin);
  return res;
}

static FLASH_res _flash_write_hword(u32_t addr, u16_t data) {
  //FLASH_ProgramHalfWord(Address, Data);
  const gpio_pin_map *led = GPIO_MAP_get_led_map();
  gpio_enable(led->port, led->pin);
  FLASH_res res = _flash_wait(FLASH_TIMEOUT);

  if (res == FLASH_OK) {
    // if the previous operation is completed, proceed to program the new first
    // half word
    FLASH->CR |= CR_PG_Set;

    *(__IO uint16_t*) addr = (uint16_t) data;
    res = _flash_wait(FLASH_TIMEOUT);
  }
  // Disable the PG Bit
  FLASH->CR &= CR_PG_Reset;
  gpio_disable(led->port, led->pin);

  return res;
}

