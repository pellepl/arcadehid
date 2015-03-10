/*
 * app_annoyatron.c
 *
 *  Created on: Mar 10, 2015
 *      Author: petera
 */

#include "system.h"
#include "niffs_impl.h"
#include "taskq.h"
#include "miniutils.h"
#include "cli.h"

typedef struct {
  u32_t fd_offs;
  u32_t loops;
} loop_stack_entry;

#define LOOP_STACK 4

static int annoy_fd = -1;
static int wait_cycle = 1;
static loop_stack_entry loop_stack[LOOP_STACK];
static int loop_stack_ix = -1;
static niffs *fs;
static task_timer timer;
static task *annoy_task;

void annoy_loop_enter(int loops) {
  if (annoy_fd >= 0) {
    loop_stack_ix++;
    ASSERT(loop_stack_ix < LOOP_STACK);
    ASSERT(loop_stack_ix >= 0);
    loop_stack[loop_stack_ix].loops = loops;
    loop_stack[loop_stack_ix].fd_offs = NIFFS_ftell(fs, annoy_fd);
  }
}

void annoy_loop_exit(void) {
  ASSERT(loop_stack_ix >= 0);
  if (loop_stack[loop_stack_ix].loops == 0) {
    loop_stack_ix--;
    ASSERT(loop_stack_ix >= -1);
  } else {
    NIFFS_lseek(fs, annoy_fd, loop_stack[loop_stack_ix].fd_offs, NIFFS_SEEK_SET);
    loop_stack[loop_stack_ix].loops--;
  }
}

void annoy_wait(int time) {
  wait_cycle = time;
}

void annoy_stop(void) {
  TASK_stop_timer(&timer);
  if (annoy_fd >= 0) NIFFS_close(fs, annoy_fd);
}

static void annoy_find_new_file(void) {
  niffs_DIR d;
  struct niffs_dirent e;
  struct niffs_dirent *pe = &e;
  u32_t files = 0;

  NIFFS_opendir(fs, "/", &d);
  while ((pe = NIFFS_readdir(&d, pe))) {
    if (strncmp("annoy_", (char *)pe->name, 6) == 0) {
      files++;
    }
  }
  NIFFS_closedir(&d);

  if (files > 0) {
    u32_t file_ix = (rand_next() / 137) % files;

    files = 0;
    pe = &e;
    NIFFS_opendir(fs, "/", &d);
    while ((pe = NIFFS_readdir(&d, pe))) {
      if (strncmp("annoy_", (char *)pe->name, 6) == 0) {
        if (files >= file_ix) {
          annoy_fd = NIFFS_open(fs, (char *)pe->name, NIFFS_O_RDONLY, 0);
          break;
        }
        files++;
      }
    }
    NIFFS_closedir(&d);

    wait_cycle = 4000 + (rand_next() % (1000*60*60));

    niffs_stat s;
    NIFFS_fstat(fs, annoy_fd, &s);
    print("running %s in %i seconds...\n", s.name, wait_cycle/1000);
  } else {
    wait_cycle = 5000;
    print("no scripts, trying again in %i seconds...\n", wait_cycle/1000);
  }
}

void annoy_readln(void) {
  u8_t buf[256];
  int res;
  int ix = 0;
  wait_cycle = 1;
  do {
    if (annoy_fd < 0) {
      res = ERR_NIFFS_END_OF_FILE;
      break;
    }
    u8_t *ptr;
    u32_t avail;
    res = NIFFS_read_ptr(fs, annoy_fd, &ptr, &avail);
    if (avail < 1) res = ERR_NIFFS_END_OF_FILE;
    if (res < NIFFS_OK) break;
    res = NIFFS_lseek(fs, annoy_fd, 1, NIFFS_SEEK_CUR);
    if (*ptr == '\n') {
      buf[ix] = 0;
      if (ix > 0) {
        break;
      }
    } else {
      buf[ix++] = *ptr;
    }
    if (res < NIFFS_OK) break;
  } while (res >= NIFFS_OK);

  bool new_file = FALSE;
  if (res >= NIFFS_OK) {
    // got us a line, execute
    print("] %s\n", buf);
    CLI_parse(ix, buf);
  } else {
    NIFFS_close(fs, annoy_fd);
    new_file = TRUE;
  }

  if (new_file) {
    loop_stack_ix = -1;
    annoy_find_new_file();
  }

  // start tick for next line/file
  TASK_start_timer(annoy_task, &timer, 0, 0, wait_cycle, 0, "annoy_timer");
}

static void annoy_task_f(u32_t ignore, void *ignore_p) {
  annoy_readln();
}

void annoy_init(void) {
  fs = FS_get_fs();
  annoy_fd = NIFFS_open(fs, "init_annoy", NIFFS_O_RDONLY, 0);
  wait_cycle = 1;
  loop_stack_ix = -1;
  annoy_task = TASK_create(annoy_task_f, TASK_STATIC);
  ASSERT(annoy_task);
  TASK_start_timer(annoy_task, &timer, 0, 0, 1000, 0, "annoy_timer");
  rand_seed(0xf7eefa11);
}
