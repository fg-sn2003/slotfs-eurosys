#include "test.h"
#include <pthread.h>
#include "../lock.h"
#include <string.h>

char buf[4096];
spinlock_t lock;
pthread_rwlock_t rwlock;

#define ROUND 1000000
#define THREAD_NUM 10
char buf[4096];
spinlock_t lock;
pthread_rwlock_t rwlock;

void* spinlock_thread(void* arg) {
    for (int i = 0; i < ROUND; i++) {
        spin_lock(&lock);
        // memset(buf, 0, 4096); 
        spin_unlock(&lock);
    }
    return NULL;
}

void* rwlock_thread(void* arg) {
    for (int i = 0; i < ROUND; i++) {
        pthread_rwlock_wrlock(&rwlock);
        // memset(buf, 0, 4096); 
        pthread_rwlock_unlock(&rwlock);
    }
    return NULL;
}

int main() {
    pthread_t threads[THREAD_NUM];
    struct timespec start, end;

    spin_lock_init(&lock);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&threads[i], NULL, spinlock_thread, NULL);
    }
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Spinlock time: %ld ns\n", (end.tv_sec - start.tv_sec) * 1000000000L + end.tv_nsec - start.tv_nsec);


    pthread_rwlock_init(&rwlock, NULL);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&threads[i], NULL, rwlock_thread, NULL);
    }
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("RWLock time: %ld ns\n", (end.tv_sec - start.tv_sec) * 1000000000L + end.tv_nsec - start.tv_nsec);

    return 0;
}