#include <stdio.h>
#include "test.h"
#include "../inode.h"
#include "../debug.h"

void link_test() {
    int ret;
    ret = mkdir("\\link", 0777);
    EXPECT(ret == 0, "mkdir failed");

    int fd = open("\\link/file", O_CREAT | O_RDWR, 0);
    EXPECT(fd >= 0, "open failed");

    ret = link("\\link/file", "\\link/link");
    EXPECT(ret == 0, "link failed");

    int fd2 = open("\\link/link", O_RDWR, 0);
    EXPECT(fd2 >= 0, "open failed");
    ret = write(fd, "hello", 5);
    EXPECT(ret == 5, "write failed");
    ret = write(fd2, "world", 5);
    EXPECT(ret == 5, "write failed");

    char buf[10];
    lseek(fd, 0, SEEK_SET);
    ret = read(fd, buf, 10);
    EXPECT(ret == 5, "read failed");
    buf[5] = 0;
    EXPECT(strcmp(buf, "world") == 0, "read failed");
}

void unlink_test() {
    int ret;
    ret = mkdir("\\unlink", 0777);
    EXPECT(ret == 0, "mkdir failed");

    int fd = open("\\unlink/file", O_CREAT | O_RDWR, 0);
    EXPECT(fd >= 0, "open failed");
    
    ret = link("\\unlink/file", "\\unlink/link");
    EXPECT(ret == 0, "link failed");
    
    ret = unlink("\\unlink/file");
    EXPECT(ret == 0, "unlink failed");
    
    fd = open("\\unlink/file", O_RDWR, 0);
    EXPECT(fd < 0, "open failed");

    fd = open("\\unlink/link", O_RDWR, 0);
    EXPECT(fd >= 0, "open failed");
    
    ret = unlink("\\unlink/link");
    EXPECT(ret == 0, "unlink failed");

    fd = open("\\unlink/link", O_RDWR, 0);
    EXPECT(fd < 0, "open failed");
}

int main() {
    link_test();
    unlink_test();
    printf("link test passed\n");
    return 0;
}