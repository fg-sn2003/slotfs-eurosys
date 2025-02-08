#include "lock.h"
#include "config.h"
#include <assert.h>

static bool is_overlap(uint64_t start1, uint64_t end1, uint64_t start2, uint64_t end2) {
    return (start1 < end2) && (start2 < end1);
}

void range_lock_init(range_lock_t *lock) {
    for (int i = 0; i < RLOCK_NUM; i++) {
        lock->rlock[i].valid = false;
    }
    spin_lock_init(&lock->global_lock);
    lock->lock_count = 0;
}

void range_lock(range_lock_t *lock, uint64_t start, uint64_t end) {
    do {
        spin_lock(&lock->global_lock);
        if (likely(lock->lock_count == 0)) {
            lock->rlock[0].valid = true;
            lock->rlock[0].start = start;
            lock->rlock[0].end = end;
            lock->lock_count++;
            spin_unlock(&lock->global_lock);
            return;
        }

        bool overlap = false;
        for (int i = 0; i < RLOCK_NUM; i++) {
            if (lock->rlock[i].valid && is_overlap(start, end, lock->rlock[i].start, lock->rlock[i].end)) {
                overlap = true;
                // for (int i = 0; i < 1000000; i++) {
                    
                // }
                break;
            }
        }

        if (!overlap) {
            for (int i = 0; i < RLOCK_NUM; i++) {
                if (!lock->rlock[i].valid) {
                    lock->rlock[i].valid = true;
                    lock->rlock[i].start = start;
                    lock->rlock[i].end = end;
                    lock->lock_count++;
                    spin_unlock(&lock->global_lock);
                    return;
                }
            }
        }
        spin_unlock(&lock->global_lock);

        do {} while (lock->lock_count == RLOCK_NUM);
    } while (1);
}

void range_unlock(range_lock_t *lock, uint64_t start, uint64_t end) {
    spin_lock(&lock->global_lock);
    for (int i = 0; i < RLOCK_NUM; i++) {
        if (lock->rlock[i].valid && lock->rlock[i].start == start 
            && lock->rlock[i].end == end) {
            lock->rlock[i].valid = false;
            lock->lock_count--;
            spin_unlock(&lock->global_lock);
            return;
        }
    }
    spin_unlock(&lock->global_lock);
    assert(0);
}

