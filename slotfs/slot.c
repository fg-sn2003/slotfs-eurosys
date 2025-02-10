#define _GNU_SOURCE

#include <sched.h>
#include "bitops.h"
#include "slot.h"
#include "slotfs.h"

int map_num = MAP_NUM;
index_t do_slot_alloc(dram_bitmap_t *bitmaps) {
    size_t start_bitmap = sched_getcpu() % map_num;
    size_t current_bitmap = start_bitmap;
    index_t index = -1;

    do {
        dram_bitmap_t *bitmap = &bitmaps[current_bitmap];
        if (bitmap->remain > 0) {
            spin_lock(&bitmap->s_lock);

            size_t bit = next_zero_bit(bitmap->dram, bitmap->bit_num, bitmap->hint);
            if (bit >= bitmap->bit_num) {
                bit = next_zero_bit(bitmap->dram, bitmap->hint, 0);
            }

            if (bit < bitmap->bit_num) {
                set_bit(bit, bitmap->dram);
                bitmap->hint = bit + 1;
                bitmap->remain--;
                index = bit + bitmap->bit_offset;
                spin_unlock(&bitmap->s_lock);
                break;
            }

            spin_unlock(&bitmap->s_lock);
        }

        current_bitmap = (current_bitmap + 1) % map_num;
    } while (current_bitmap != start_bitmap);

    debug_assert(index != -1);
    return index;
}

//best effort slot allocation
#define SEARCH_ROUND 10
index_t slot_be_alloc(dram_bitmap_t *bitmaps) {
    index_t index = -1;
    size_t start_bitmap = sched_getcpu() % map_num;
    size_t current_bitmap = start_bitmap;
    int round = 0;
    do {
        dram_bitmap_t *bitmap = &bitmaps[current_bitmap];
        if (bitmap->remain > 0) {
            spin_lock(&bitmap->s_lock);

            // try 64-bit check
            for (size_t i = 0; i < bitmap->bit_num / 64; i++) {
                if (round > SEARCH_ROUND) {
                    break;
                }
                uint64_t *chunk = (uint64_t *)(bitmap->dram + i * 8);
                if (*chunk == 0) {
                    set_bit(i * 64, bitmap->dram);
                    bitmap->hint = i * 64 + 1;
                    bitmap->remain--;
                    index = i * 64 + bitmap->bit_offset;
                    spin_unlock(&bitmap->s_lock);
                    break;
                }
                round++;
            }

            if (index != -1) break;
            round = 0;
            // otherwise try 16-bit check
            for (size_t i = 0; i < bitmap->bit_num / 16; i++) {
                uint16_t *chunk = (uint16_t *)(bitmap->dram + i * 2);
                if (round > SEARCH_ROUND) {
                    break;
                }
                if (*chunk == 0) {
                    set_bit(i * 16, bitmap->dram);
                    bitmap->hint = i * 16 + 1;
                    bitmap->remain--;
                    index = i * 16 + bitmap->bit_offset;
                    spin_unlock(&bitmap->s_lock);
                    break;
                }
                round++;
            }

            if (index != -1) break;
            round = 0;
            // otherwise single-bit approach (existing code)
            size_t bit = next_zero_bit(bitmap->dram, bitmap->bit_num, bitmap->hint);
            if (bit >= bitmap->bit_num) {
                bit = next_zero_bit(bitmap->dram, bitmap->hint, 0);
            }

            if (bit < bitmap->bit_num) {
                set_bit(bit, bitmap->dram);
                bitmap->hint = bit + 1;
                bitmap->remain--;
                index = bit + bitmap->bit_offset;
                spin_unlock(&bitmap->s_lock);
                break;
                round++;
            }

            spin_unlock(&bitmap->s_lock);
            if (index != -1)
                break;
        }
        current_bitmap = (current_bitmap + 1) % map_num;
    } while (current_bitmap != start_bitmap);

    return index;
}

static inline index_t slot_try_cache_alloc(dram_bitmap_t *bitmaps, index_t *cache, int *offset) {
    if (*cache == 0 && *offset == 0)
        return -1;

    size_t chunk_size = 1;
    if ((*cache % 64) == 0) {
        chunk_size = 64;
    } else if ((*cache % 16) == 0) {
        chunk_size = 16;
    }

    if (*offset >= chunk_size)
        return -1;

    index_t bit_idx = *cache + *offset;
    size_t bitmap_size = bitmaps[0].bit_num;
    size_t bitmap_index = bit_idx / bitmap_size;
    size_t bit = bit_idx % bitmap_size;

    debug_assert(bitmap_index < map_num);

    dram_bitmap_t *bitmap = &bitmaps[bitmap_index];
    spin_lock(&bitmap->s_lock);
    if (!check_bit(bit, bitmap->dram)) {
        set_bit(bit, bitmap->dram);
        bitmap->remain--;
    }
    spin_unlock(&bitmap->s_lock);

    (*offset)++;
    return bit_idx;
}

void do_slot_free(dram_bitmap_t *bitmaps, index_t index) {
    size_t bitmap_size = bitmaps[0].bit_num;  // each bitmap has the same size except for the last one.
    size_t bitmap_index = index / bitmap_size;
    size_t bit = index % bitmap_size;

    if (bitmap_index < map_num) {
        dram_bitmap_t *bitmap = &bitmaps[bitmap_index];
        spin_lock(&bitmap->s_lock);

        debug_assert(check_bit(bit, bitmap->dram));

        clear_bit(bit, bitmap->dram);
        bitmap->remain++;

        spin_unlock(&bitmap->s_lock);
    }
}

index_t do_slot_alloc_range(dram_bitmap_t *bitmaps, size_t *count) {
    size_t start_bitmap = sched_getcpu() % map_num;
    size_t current_bitmap = start_bitmap;
    index_t result_index = -1;

    do {
        dram_bitmap_t *bitmap = &bitmaps[current_bitmap];

        if (bitmap->remain > 0) {
            spin_lock(&bitmap->s_lock);

            size_t bit_num  = bitmap->bit_num;
            size_t hint     = bitmap->hint;
            size_t range_start = next_zero_bit(bitmap->dram, bit_num, hint);
            if (range_start >= bit_num) 
                range_start = next_zero_bit(bitmap->dram, hint, 0);
            debug_assert(range_start < bit_num);

            size_t range_end = next_set_bit(bitmap->dram, 
                range_start + (*count > bit_num - range_start ? bit_num - range_start : *count), range_start);
            
            size_t found_count = range_end - range_start;
            if (found_count == 0) { //todo: fix this
                spin_unlock(&bitmap->s_lock);
                current_bitmap = (current_bitmap + 1) % map_num;
                continue;
            }
            debug_assert(found_count <= *count);
            set_bit_range(range_start, bitmap->dram, found_count);
            bitmap->hint = range_end;
            bitmap->remain -= found_count;
            result_index = range_start + bitmap->bit_offset;
            spin_unlock(&bitmap->s_lock);
            *count = found_count;
            return result_index;
        }

        current_bitmap = (current_bitmap + 1) % map_num;
    } while (current_bitmap != start_bitmap);

    *count = 0;
    debug_assert(0);
    return -1;
}

void do_slot_free_range(dram_bitmap_t *bitmaps, index_t index, size_t count) {
    size_t bitmap_size = bitmaps[0].bit_num;  // each bitmap has the same size except for the last one.
    size_t bitmap_index = index / bitmap_size;
    size_t bit = index % bitmap_size;

    dram_bitmap_t *bitmap = &bitmaps[bitmap_index];
    spin_lock(&bitmap->s_lock);

    clear_bit_range(bit, bitmap->dram, count);
    bitmap->remain += count;

    spin_unlock(&bitmap->s_lock);
}

index_t slot_inode_alloc(inode_t* inode) {
    return do_slot_alloc(sbi->maps[SLOT_INODE]);
}

index_t slot_data_alloc(inode_t* inode, size_t *count) {
    return do_slot_alloc_range(sbi->maps[SLOT_DATA], count);
}

void slot_data_free(inode_t* inode, index_t index, size_t count) {
    do_slot_free_range(sbi->maps[SLOT_DATA], index, count);
}

void slot_inode_free(inode_t* inode, index_t index) {
    do_slot_free(sbi->maps[SLOT_INODE], index);
}

index_t slot_dirent_alloc(inode_t* inode) {
    return do_slot_alloc(sbi->maps[SLOT_DIR]);
}

void slot_dirent_free(inode_t* inode, index_t index) {
    do_slot_free(sbi->maps[SLOT_DIR], index);
}

index_t slot_firent_alloc(inode_t* inode) {
    // index_t index = slot_try_cache_alloc(sbi->maps[SLOT_FILE], 
    //     &inode->f_slot_cache, &inode->f_slot_cache_off);
    // if (index != -1) {
    //     // printf("cache alloc index = %ld\n", index);
    //     return index;
    // }
    
    // index = slot_be_alloc(sbi->maps[SLOT_FILE]);
    // if (unlikely(index != -1)) {
    //     inode->f_slot_cache = index;
    //     inode->f_slot_cache_off = 1;
    //     // printf("be alloc index = %ld\n", index);
    // }
    // return index;
    return do_slot_alloc(sbi->maps[SLOT_FILE]);
}

void slot_firent_free(inode_t* inode, index_t index) {
    do_slot_free(sbi->maps[SLOT_FILE], index);
}




