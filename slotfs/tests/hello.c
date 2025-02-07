#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include "test.h"

char buf[4096];
int main() {
    // int fd, ret;
    // fd = open("\\/test", O_RDWR | O_CREAT, 0644);
    // assert(fd > 0);

    // for (int i = 0; i < 10; i++) {
    //     ret = write(fd, buf, 4096);
    //     assert(ret == 4096);
    // }
    // ret = close(fd);
    // assert(ret == 0);

    printf("Hello, World!\n");
    return 0;
}