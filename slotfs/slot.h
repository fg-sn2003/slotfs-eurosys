#ifndef _SLOT_H
#define _SLOT_H

#include "inode.h"
#include "lock.h"
#include "list.h"

typedef struct dram_bitmap {
    spinlock_t s_lock;    
    void* dram;         // dram address
    relptr_t pm;        // pm address

    size_t bit_offset;  // bit offset in the big bitmap
    size_t bit_num;     // number of bits
    size_t remain;
    size_t hint;
} dram_bitmap_t;


index_t do_slot_alloc(dram_bitmap_t *bitmaps);
void do_slot_free(dram_bitmap_t *bitmaps, index_t index);
index_t do_slot_alloc_range(dram_bitmap_t *bitmaps, size_t *count);
void do_slot_free_range(dram_bitmap_t *bitmaps, index_t index, size_t count);

index_t slot_inode_alloc(inode_t* inode);
index_t slot_data_alloc(inode_t* inode, size_t *count);
index_t slot_dirent_alloc(inode_t* inode);
index_t slot_firent_alloc(inode_t* inode);

void slot_data_free(inode_t* inode, index_t index, size_t count);
void slot_inode_free(inode_t* inode, index_t index);
void slot_dirent_free(inode_t* inode, index_t index);
void slot_firent_free(inode_t* inode, index_t index);

#endif // _SLOT_H