#include "test.h"

char buf[4096];

void release_test_1() {
    int ret, fd;

    fd = open("\\/test.txt", O_CREAT | O_RDWR, 0644);
    memset(buf, 'a', 4096);

    for (int i = 0; i < 1000; i++) {
        ret = write(fd, buf, 4096);
        EXPECT(ret == 4096, "write");
    }

    ret = unlink("\\/test.txt");
    EXPECT(ret == 0, "unlink");

    close(fd);
}

void release_test_2() {
    int ret, fd;

    fd = open("\\/test.txt", O_CREAT | O_RDWR, 0644);
    memset(buf, 'a', 4096);

    for (int i = 0; i < 1000; i++) {
        ret = write(fd, buf, 4096);
        EXPECT(ret == 4096, "write");
    }
    close(fd);

    ret = unlink("\\/test.txt");
    EXPECT(ret == 0, "unlink");

}

int main() {
    release_test_1();
    release_test_2();
    printf("release test passed\n");
    return 0;
}