#include "../lock.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>


#define THREAD_NUM 50
#define SLEEP_TIME 10000 // Sleep time in seconds

range_lock_t lock;

// Thread function for overlapping ranges
void* thread_func_overlap(void* arg) {
    uint64_t start = 0;
    uint64_t end = 100;

    range_lock(&lock, start, end);
    printf("Thread %ld locked range [%lu, %lu]\n", (long)arg, start, end);
    usleep(SLEEP_TIME); // Simulate work by sleeping
    range_unlock(&lock, start, end);
    printf("Thread %ld unlocked range [%lu, %lu]\n", (long)arg, start, end);

    return NULL;
}

// Thread function for non-overlapping ranges
void* thread_func_nooverlap(void* arg) {
    uint64_t start = (long)arg * 100;
    uint64_t end = start + 100;

    range_lock(&lock, start, end);
    printf("Thread %ld locked range [%lu, %lu]\n", (long)arg, start, end);
    usleep(SLEEP_TIME); // Simulate work by sleeping
    range_unlock(&lock, start, end);
    printf("Thread %ld unlocked range [%lu, %lu]\n", (long)arg, start, end);

    return NULL;
}

// Function to measure running time
void measure_running_time(void* (*thread_func)(void*), const char* test_name) {
    pthread_t threads[THREAD_NUM];
    struct timespec start, end;

    // Initialize the range lock
    range_lock_init(&lock);

    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Create threads
    for (long i = 0; i < THREAD_NUM; i++) {
        pthread_create(&threads[i], NULL, thread_func, (void*)i);
    }

    // Join threads
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate running time
    double running_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%s running time: %.6f seconds\n", test_name, running_time);

}

int main() {
    // Test overlapping ranges
    measure_running_time(thread_func_overlap, "Overlapping Ranges");

    // Test non-overlapping ranges
    measure_running_time(thread_func_nooverlap, "Non-Overlapping Ranges");

    return 0;
}