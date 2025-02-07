#include "../slot.h"
#include "../slotfs.h"
#include "../lock.h"
#include <assert.h>

#define NUM_MAP 16
#define NUM_BIT 64
dram_bitmap_t mock_bitmaps[NUM_MAP];

void init_mock_bitmaps() {
    for (int i = 0; i < NUM_MAP; i++) {
        mock_bitmaps[i].bit_num = NUM_BIT;
        mock_bitmaps[i].remain = NUM_BIT;
        mock_bitmaps[i].hint = 0;
        mock_bitmaps[i].bit_offset = i * NUM_BIT;
        mock_bitmaps[i].dram = malloc(NUM_BIT / 8);
        spin_lock_init(&mock_bitmaps[i].s_lock);
        memset(mock_bitmaps[i].dram, 0, NUM_BIT / 8);
    }
}

int allocated[NUM_BIT * NUM_MAP] = {0};
void slot_alloc_free() {
    init_mock_bitmaps();

    for (int i = 0; i < NUM_BIT * NUM_MAP; i++) {
        index_t index = do_slot_alloc(mock_bitmaps);
        assert(index != -1);
        allocated[index] = 1;
    }

    for (int i = 0; i < NUM_BIT * NUM_MAP; i++) {
        assert(allocated[i] == 1);
    }

    for (int i = 0; i < NUM_BIT * NUM_MAP; i += 2) {
        do_slot_free(mock_bitmaps, i);
    }

    size_t remain = 0;
    for (int i = 0; i < NUM_MAP; i++) {
        for (int j = 0; j < NUM_BIT; j++) {
            // printf("i = %d, j = %d\n", i, j);
            if (j % 2 == 0) {
                // printf("check_bit: %d\n", check_bit(j, mock_bitmaps[i].dram));
                assert(check_bit(j, mock_bitmaps[i].dram) == 0);
            } else {
                assert(check_bit(j, mock_bitmaps[i].dram) != 0);
            }
        }
        remain += mock_bitmaps[i].remain;
    }
    assert(remain == NUM_BIT * NUM_MAP / 2);
    for (int i = 1; i < NUM_BIT * NUM_MAP; i += 2) {
        do_slot_free(mock_bitmaps, i);
    }

    remain = 0;
}   

void slot_alloc_free_range_basic() {
    init_mock_bitmaps();

    size_t count = 16;
    assert(NUM_BIT % count == 0);
    for (int i = 0; i < NUM_BIT * NUM_MAP; i+=count) {
        size_t _count = count;
        index_t index = do_slot_alloc_range(mock_bitmaps, &_count);
        assert(_count == count);
        assert(index != -1);
        for (int k = 0; k < count; k++) {
            allocated[index + k] = 1;
        }
    }

    for (int i = 0; i < NUM_BIT * NUM_MAP; i++) {
        assert(allocated[i] == 1);
    }

    count = 8;
    for (int i = 0; i < NUM_BIT * NUM_MAP; i += count) {
        do_slot_free_range(mock_bitmaps, i, count);
    }

    count = 4;
    for (int i = 0; i < NUM_BIT * NUM_MAP; i += count) {
        size_t _count = count;
        index_t index = do_slot_alloc_range(mock_bitmaps, &_count);
        assert(index != -1);
        assert(_count == count);
    }

    count = 2;
    for (int i = 0; i < NUM_BIT * NUM_MAP; i += count) {
        do_slot_free_range(mock_bitmaps, i, count);
    }
}

int main() {
    slot_alloc_free_range_basic();
    return 0;
}
