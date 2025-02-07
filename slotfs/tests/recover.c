#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include "test.h"
#define ITERATIONS 1000000000

char buf[4096];
// basic test
int recover_1() {
        int fd, ret;
    fd = open("\\/test0", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test1", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test2", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test3", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test4", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test5", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test6", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test7", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test8", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test9", O_RDWR | O_CREAT, 0644);
    fd = open("\\/test10", O_RDWR | O_CREAT, 0644);
    fd = mkdir("\\/test11", 0644);
    fd = mkdir("\\/test12", 0644);
    fd = mkdir("\\/test13", 0644);
    fd = mkdir("\\/test14", 0644);
    fd = mkdir("\\/test15", 0644);
    fd = mkdir("\\/test16", 0644);
    fd = mkdir("\\/test17", 0644);
    fd = mkdir("\\/test18", 0644);
    fd = mkdir("\\/test19", 0644);
    fd = mkdir("\\/test20", 0644);
}

// test if ghost slot can be identified
int recover_2() {
    int fd, ret;
    fd = open("\\/test0", O_RDWR | O_CREAT, 0644);
    unlink("\\/test0");
    fd = open("\\/test1", O_RDWR | O_CREAT, 0644);
    unlink("\\/test1");
    fd = open("\\/test2", O_RDWR | O_CREAT, 0644);
    unlink("\\/test2");
}

int recover_3() {
    int fd, ret;
    fd = open("\\/test0", O_RDWR | O_CREAT, 0644);
    for (int i = 0; i < 10; i++) {
        ret = write(fd, buf, 4096);
        assert(ret == 4096);
    }
    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < 10; i++) {
        write(fd, buf, 4096);
        assert(ret == 4096);
    }
    close(fd);
}

#define DIR_DEPTH 4
#define DIR_WIDTH 4
char buf[4096];
int create_directories(const char* base_path, int level) {
    char path[100];
    if (level == DIR_DEPTH) {
        for (int j = 1; j <= DIR_WIDTH; j++) {
            snprintf(path, sizeof(path), "%s/file%d", base_path, j);
            int fd = open(path, O_RDWR | O_CREAT, 0644);
            EXPECT(fd >= 0, "open failed");
            write(fd, buf, 4096);
            close(fd);
        }
        return 0;
    }
    for (int j = 1; j <= DIR_WIDTH; j++) {
        snprintf(path, sizeof(path), "%s/level%d_subdir%d", base_path, level, j);
        int result = mkdir(path, 0755);
        EXPECT(result == 0, "mkdir failed");

        create_directories(path, level + 1);
    }
    return 0;
}

int recover_4() {
    int ret;
    mkdir("\\testdir", 0755);
    ret = create_directories("\\/testdir", 1);
    assert(ret == 0);
}

int main() {
    recover_3();
}