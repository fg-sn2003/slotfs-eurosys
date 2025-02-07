#ifndef LOCK_H
#define LOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef pthread_spinlock_t spinlock_t;
static inline int spin_lock_init(spinlock_t *lock) {
	return pthread_spin_init(lock, 1);
}
static inline int spin_lock(spinlock_t *lock) {
	return pthread_spin_lock(lock);
}
static inline int spin_unlock(spinlock_t *lock) {
	return pthread_spin_unlock(lock);
}
static inline int spin_trylock(spinlock_t *lock) {
	return pthread_spin_trylock(lock);
}

#define RLOCK_NUM 10
typedef struct {
	uint64_t start;
	uint64_t end;
	bool valid;
} rlock_node_t;

typedef struct range_lock{
	rlock_node_t rlock[RLOCK_NUM];
	spinlock_t global_lock;
	int lock_count;
} range_lock_t;

void range_lock_init(range_lock_t *lock);
void range_lock(range_lock_t *lock, uint64_t start, uint64_t end);
void range_unlock(range_lock_t *lock, uint64_t start, uint64_t end);

#endif // LOCK_H