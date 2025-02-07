#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "test.h"

char buf[4096];

int init_test_file() {
    int fd = open("\\test_file", O_CREAT | O_RDWR, 0666);
    EXPECT(fd != -1, "open test file failed");
    return fd;
}

void basic_rw() {
    int ret;
    int fd = init_test_file();

    for (int i = 0; i < 4096; i++) {
        buf[i] = 3;
    }
    
    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write failed");

    ret = lseek(fd, 0, SEEK_SET);
    EXPECT(ret == 0, "seek failed");
    
    ret = read(fd, buf, 4096);
    EXPECT(ret == 4096, "read failed");
    for (int i = 0; i < 4096; i++) {
        EXPECT(buf[i] == 3, "read content error");
    }
    ret = read(fd, buf, 4096);
    EXPECT(ret == 0, "read failed");
    close(fd);
}

void small_rw()
{
    #define PER_SIZE 512
    int ret;
    int fd = open("\\small_rw", O_CREAT | O_RDWR, 0666);
    EXPECT(fd != -1, "open test file failed");

    for (int i = 0; i < 4096; i++) {
        buf[i] = 3;
    }
    
    for (int i = 0; i < 4096; i += PER_SIZE) {
        ret = write(fd, buf + i, PER_SIZE);
        EXPECT(ret == PER_SIZE, "write failed");
    }

    ret = lseek(fd, 0, SEEK_SET);
    EXPECT(ret == 0, "seek failed");

    for (int i = 0; i < 4096; i += PER_SIZE) {
        ret = read(fd, buf + i, PER_SIZE);
        EXPECT(ret == PER_SIZE, "read failed");
        for (int j = 0; j < PER_SIZE; j++) {
            EXPECT(buf[i + j] == 3, "read content error");
        }
    }
}

void partial_rw()
{
    int ret;
    int fd = open("\\partial_rw", O_CREAT | O_RDWR, 0666);
    EXPECT(fd != -1, "open test file failed");

    /* partial write whtin a block */
    memset(buf, 3, 4096);
    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write failed");

    ret = lseek(fd, 1024, SEEK_SET);
    EXPECT(ret == 0, "seek failed");

    memset(buf, 5, 4096);
    ret = write(fd, buf, 2048);
    EXPECT(ret == 2048, "write failed");

    ret = lseek(fd, 0, SEEK_SET);
    EXPECT(ret == 0, "seek failed");

    ret = read(fd, buf, 4096);
    EXPECT(ret == 4096, "read failed");
    for (int i = 0; i < 1024; i++) {
        EXPECT(buf[i] == 3, "read content error");
    }
    for (int i = 1024; i < 1024 * 3; i++) {
        EXPECT(buf[i] == 5, "read content error");
    }
    for (int i = 1024 * 3; i < 4096; i++) {
        EXPECT(buf[i] == 3, "read content error");
    }


    /* partial write across blocks */
    memset(buf, 3, 4096);
    ret = lseek(fd, 0, SEEK_SET);
    EXPECT(ret == 0, "seek failed");
    for (int i = 0; i < 3; i++) {
        ret = write(fd, buf, 4096);
        EXPECT(ret == 4096, "write failed");
    }

    ret = lseek(fd, 1024, SEEK_SET);
    EXPECT(ret == 0, "seek failed");

    memset(buf, 5, 4096);
    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write failed");
    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write failed");

    ret = lseek(fd, 0, SEEK_SET);

    ret = read(fd, buf, 4096);
    EXPECT(ret == 4096, "read failed");
    for (int i = 0; i < 1024; i++) 
        EXPECT(buf[i] == 3, "read content error");
    for (int i = 1024; i < 4096; i++) 
        EXPECT(buf[i] == 5, "read content error");

    ret = read(fd, buf, 4096);
    EXPECT(ret == 4096, "read failed");
    for (int i = 0; i < 4096; i++) 
        EXPECT(buf[i] == 5, "read content error");

    ret = read(fd, buf, 4096);
    EXPECT(ret == 4096, "read failed");
    for (int i = 0; i < 1024; i++) 
        EXPECT(buf[i] == 5, "read content error");
    for (int i = 1024; i < 4096; i++) {
        EXPECT(buf[i] == 3, "read content error");
    }
}

int journal_write() {
    int fd, ret;

    fd = open("\\journal", O_CREAT | O_RDWR, 0666);
    EXPECT(fd != -1, "open journal file failed");

    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write journal failed");

    pwrite(fd, buf, 10, 0);
    pwrite(fd, buf, 10, 10);
    pwrite(fd, buf, 10, 20);
    pwrite(fd, buf, 10, 30);
    pwrite(fd, buf, 10, 40);
    pwrite(fd, buf, 10, 50);
    pwrite(fd, buf, 10, 60);
}

int main() {
    basic_rw();
    small_rw();
    partial_rw();
    journal_write();
    printf("rw test passed\n");
}