#include "test.h"
#define BUFFER_SIZE 4096 * 16
char buf[BUFFER_SIZE] = {1};

void test() {
    int ret;
    int fd = open("\\test_file", O_CREAT | O_RDWR, 0666);
    EXPECT(fd != -1, "open test file failed");

    ret = write(fd, buf, BUFFER_SIZE);
    EXPECT(ret == BUFFER_SIZE, "write test file failed");
    
    lseek(fd, 0, SEEK_SET);
    // 3
    ret = write(fd, buf, 1024 * 6);
    EXPECT(ret == 1024 * 6, "write test file failed");
    ret = write(fd, buf, 1024 * 6);
    EXPECT(ret == 1024 * 6, "write test file failed");
    // 3
    ret = write(fd, buf, 1024 * 6);
    EXPECT(ret == 1024 * 6, "write test file failed");
    ret = write(fd, buf, 1024 * 6);
    EXPECT(ret == 1024 * 6, "write test file failed");

    ret = write(fd, buf, 4096);
    EXPECT(ret == 4096, "write test file failed");

    ret = write(fd, buf, 4096 * 2);
    EXPECT(ret == 4096 * 2, "write test file failed");

    ret = write(fd, buf, 4096 * 4);
    EXPECT(ret == 4096 * 4, "write test file failed");

    ret = write(fd, buf, 4096 * 8);
    EXPECT(ret == 4096 * 8, "write test file failed");
}

int main() {
    test();
    return 0;
}