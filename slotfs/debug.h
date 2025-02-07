#ifndef DEBUG_H
#define DEBUG_H

#define PM_PRINT 1

#include "inode.h"
#include "slot.h"
#include "slotfs.h"
#include "config.h"
#include "entry.h"
#include "runtime.h"

typedef struct bitmap_info {
    unsigned long remain[SLOT_TYPE_NUM];
    unsigned long used[SLOT_TYPE_NUM];
} bitmap_info_t;

inode_t *debug_get_inode(int fd);
void     debug_print_inode(int fd);
pm_inode_t *debug_get_pm_inode(int ino);
void        debug_print_pm_inode(int ino);

#endif // DEBUG_H