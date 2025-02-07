#ifndef TEST_H
#define TEST_H
#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define EXPECT(cond, msg) \
    if (!(cond)) { \
        perror(msg); \
        assert(0); \
    }

typedef void (*test_func_t)(void *arg1, void *arg2, void *arg3);

void measure_execution_time(test_func_t func, void *arg1, void *arg2, void *arg3) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    func(arg1, arg2, arg3);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    printf("Function execution time: %lld ns\n", elapsed_ns);
}
#endif // TEST_H