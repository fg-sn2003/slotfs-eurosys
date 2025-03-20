#include "lock.h"
#include "config.h"
#include <assert.h>
#include <sched.h>
#include <time.h>

void range_lock_init(range_lock_t *lock) {
    for (int i = 0; i < MAX_LOCKS; i++) {
        lock->rlock[i].type = RLOCK_FREE;
    }
    spin_lock_init(&lock->spin);
    lock->lock_count = 0;
}

int range_lock_write(range_lock_t *lock, uint64_t start, uint64_t end) {
retry_write:
    spin_lock(&lock->spin);
    
    if (lock->lock_count == 0) {
        lock->rlock[0].start = start;
        lock->rlock[0].end = end;
        lock->rlock[0].type = RLOCK_WRITE;
        lock->lock_count++;
        spin_unlock(&lock->spin);
        return 0;
    }
    
    for (int i = 0; i < MAX_LOCKS; i++) {
        rlock_entry_t *entry = &lock->rlock[i];
        if (entry->type != RLOCK_FREE && 
            (entry->start <= end) && (start <= entry->end)) {
            spin_unlock(&lock->spin);

            // TODO: make this more efficient
            struct timespec ts = {0, 1000};
            nanosleep(&ts, NULL);
            goto retry_write;
        }
    }

    int ticket = -1;
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (lock->rlock[i].type == RLOCK_FREE) {
            lock->rlock[i].start = start;
            lock->rlock[i].end = end;
            lock->rlock[i].type = RLOCK_WRITE;
            ticket = i;
            lock->lock_count++;
            break;
        }
    }

    spin_unlock(&lock->spin);
    return ticket;
}

int range_lock_read(range_lock_t *lock, uint64_t start, uint64_t end) {
retry_read:
    spin_lock(&lock->spin);
    
    if (lock->lock_count == 0) {
        lock->rlock[0].start = start;
        lock->rlock[0].end = end;
        lock->rlock[0].type = RLOCK_READ;
        lock->lock_count++;
        spin_unlock(&lock->spin);
        return 0;
    }

    for (int i = 0; i < MAX_LOCKS; i++) {
        rlock_entry_t *entry = &lock->rlock[i];
        if (entry->type == RLOCK_WRITE && 
            (entry->start <= end) && (start <= entry->end)) {
            spin_unlock(&lock->spin);
            
            struct timespec ts = {0, 1000};
            nanosleep(&ts, NULL);
            goto retry_read;
        }
    }

    int ticket = -1;
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (lock->rlock[i].type == RLOCK_FREE) {
            lock->rlock[i].start = start;
            lock->rlock[i].end = end;
            lock->rlock[i].type = RLOCK_READ;
            ticket = i;
            lock->lock_count++;
            break;
        }
    }

    spin_unlock(&lock->spin);
    return ticket;
}

void range_unlock(range_lock_t *lock, int ticket) {
    spin_lock(&lock->spin);

    assert(ticket >= 0 && ticket < MAX_LOCKS);
    
    rlock_entry_t *entry = &lock->rlock[ticket];
    entry->type = RLOCK_FREE;
    lock->lock_count--;

    spin_unlock(&lock->spin);
}