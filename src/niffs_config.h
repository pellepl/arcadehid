/*
 * niffs_config.h
 *
 *  Created on: Feb 3, 2015
 *      Author: petera
 */

#ifndef NIFFS_CONFIG_H_
#define NIFFS_CONFIG_H_

#include "system.h"
#include "miniutils.h"

#define TESTATIC static

// define for getting niffs debug output
#define NIFFS_DBG(...) DBG(D_FS, D_DEBUG, __VA_ARGS__)

// define NIFFS_DUMP to be able to visualize filesystem with NIFFS_dump
#define NIFFS_DUMP
// used to output in NIFFS_dump
#define NIFFS_DUMP_OUT(...) print(__VA_ARGS__)

#define NIFFS_MAX(x, y) (x) > (y) ? (x) : (y)
#define NIFFS_MIN(x, y) (x) < (y) ? (x) : (y)

// define for assertions within niffs
#define NIFFS_ASSERT(x) ASSERT(x)

// define maximum name length
#define NIFFS_NAME_LEN          (16)

#define NIFFS_LINEAR_AREA       (0)

// define number of bits used for object ids, used for uniquely identify a file
#define NIFFS_OBJ_ID_BITS       (8)

// define number of bits used for span indices, used for uniquely identify part of a file
#define NIFFS_SPAN_IX_BITS      (8)

// word align for target flash, e.g. stm32f1 can only write 16-bit words at a time
#define NIFFS_WORD_ALIGN        (2)

// garbage collection uses a score system to select sector to erase:
// sector_score = sector_erase_difference * F1 + free_pages * F2 + deleted_pages * F3 + busy_pages * F4
// sector with highest score is selected for garbage collection

// F1: garbage collection score factor for sector erase difference
#define NIFFS_GC_SCORE_ERASE_CNT_DIFF (100)
// F2: garbage collection score factor for percentage of free pages in sector
#define NIFFS_GC_SCORE_FREE (-4)
// F3: garbage collection score factor for percentage of deleted pages in sector
#define NIFFS_GC_SCORE_DELE (2)
// F4: garbage collection score factor for percentage of busy/written pages in sector
#define NIFFS_GC_SCORE_BUSY (-2)

// formula for selecting sector to garbage collect
// free, dele and busy is the percentage (0-99) of each type
#define NIFFS_GC_SCORE(era_cnt_diff, free, dele, busy) \
  ((era_cnt_diff) * NIFFS_GC_SCORE_ERASE_CNT_DIFF) + \
  ((free) * NIFFS_GC_SCORE_FREE) + \
  ((dele) * NIFFS_GC_SCORE_DELE) + \
  ((busy) * NIFFS_GC_SCORE_BUSY)

// enable this define to have the spare free pages worth a sector distributed,
// disable to keep the spare free pages within same sector
//#define NIFFS_EXPERIMENTAL_GC_DISTRIBUTED_SPARE_SECTOR

// type sizes, depend of the size of the filesystem and the size of the pages

// must comprise NIFFS_OBJ_ID_BITS
#define NIFFS_TYPE_OBJ_ID_SIZE u8_t

// must comprise NIFFS_SPAN_IX_BITS
#define NIFFS_TYPE_SPAN_IX_SIZE u8_t

// must comprise (NIFFS_OBJ_ID_BITS + NIFFS_SPAN_IX_BITS)
#define NIFFS_TYPE_RAW_PAGE_ID_SIZE u16_t

// must uniquely address all pages
#define NIFFS_TYPE_PAGE_IX_SIZE u16_t

// magic bits, must be sized on alignment, NIFFS_WORD_ALIGN
#define NIFFS_TYPE_MAGIC_SIZE u16_t

// sector erase counter, must be sized on alignment, NIFFS_WORD_ALIGN
#define NIFFS_TYPE_ERASE_COUNT_SIZE u16_t

// page flag, 3 values, must be sized on alignment, NIFFS_WORD_ALIGN
#define NIFFS_TYPE_PAGE_FLAG_SIZE u16_t

typedef NIFFS_TYPE_OBJ_ID_SIZE niffs_obj_id;
typedef NIFFS_TYPE_SPAN_IX_SIZE niffs_span_ix;
typedef NIFFS_TYPE_RAW_PAGE_ID_SIZE niffs_page_id_raw;
typedef NIFFS_TYPE_PAGE_IX_SIZE niffs_page_ix;
typedef NIFFS_TYPE_MAGIC_SIZE niffs_magic;
typedef NIFFS_TYPE_ERASE_COUNT_SIZE niffs_erase_cnt;
typedef NIFFS_TYPE_PAGE_FLAG_SIZE niffs_flag;

#endif /* NIFFS_CONFIG_H_ */
