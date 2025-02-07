#include "test.h"

char buf[4096];
// Link to a file in another directory
void symlink_basic_1() {
    int ret;
    int fd;
    mkdir("\\/dir1", 0777);
    mkdir("\\/dir2", 0777);
    mkdir("\\/dir1/dira", 0777);
    mkdir("\\/dir2/dirb", 0777);
    fd = open("\\/dir1/dira/file1", O_CREAT | O_RDWR, 0777);
    EXPECT(fd > 0, "open fail");
    ret = pwrite(fd, "hello", 5, 0);
    EXPECT(ret == 5, "pwrite fail");

    ret = symlink("\\/dir1/dira/file1", "\\/dir2/dirb/link");
    EXPECT(ret == 0, "symlink fail");

    fd = open("\\/dir2/dirb/link", O_RDWR);
    EXPECT(fd > 0, "open fail");
    ret = pread(fd, buf, 5, 0);
    EXPECT(ret == 5, "pread fail");
    EXPECT((strncmp(buf, "hello", 5) == 0), "buf content wrong");

    fd = open("\\/dir2/dirb/link", O_RDWR | O_NOFOLLOW);
    EXPECT(fd < 0, "open fail");
}

// Link to a file in the same directory
void symlink_basic_2() {
    int ret;
    int fd;
    mkdir("\\/basic2", 0777);
    ret = symlink("\\/basic2/file2", "\\/basic2/link");
    EXPECT(ret == 0, "symlink fail");

    fd = open("\\/basic2/link", O_RDWR);
    EXPECT(fd < 0, "open fail");

    fd = open("\\/basic2/file2", O_CREAT | O_RDWR, 0777);
    EXPECT(fd > 0, "open fail");

    fd = open("\\/basic2/link", O_RDWR);
    EXPECT(fd > 0, "open fail");
}

void symlink_multiple() {
    int ret;
    int fd;

    mkdir("\\/s_multiple", 0777);

    fd = open("\\/s_multiple/file1", O_CREAT | O_RDWR, 0777);
    EXPECT(fd > 0, "open fail");
    ret = pwrite(fd, "hello", 5, 0);
    EXPECT(ret == 5, "pwrite fail");

    ret = symlink("\\/s_multiple/file1", "\\/s_multiple/link1");
    EXPECT(ret == 0, "symlink fail");

    ret = symlink("\\/s_multiple/link1", "\\/s_multiple/link2");
    EXPECT(ret == 0, "symlink fail");

    ret = symlink("\\/s_multiple/link2", "\\/s_multiple/link3");
    EXPECT(ret == 0, "symlink fail");

    fd = open("\\/s_multiple/link3", O_RDWR);
    EXPECT(fd > 0, "open fail");
    ret = pread(fd, buf, 5, 0);
    EXPECT(ret == 5, "pread fail");
    EXPECT((strncmp(buf, "hello", 5) == 0), "buf content wrong");
}

// Create a symlink loop
void symlink_loop() {
    int ret;
    mkdir("\\/dir_loop", 0777);
    ret = symlink("\\/dir_loop/link2", "\\/dir_loop/link1");
    EXPECT(ret == 0, "symlink fail");

    ret = symlink("\\/dir_loop/link1", "\\/dir_loop/link2");
    EXPECT(ret == 0, "symlink fail");

    ret = open("\\/dir_loop/link1", O_RDWR);
    EXPECT(ret < 0, "open fail");
}

void symlink_relative() {
    int ret;
    int fd;

    ret = mkdir("\\/sym_relative", 0777);
    EXPECT(ret == 0, "mkdir fail");

    ret = symlink("file1", "\\/sym_relative/link1");
    EXPECT(ret == 0, "symlink fail");

    fd = open("\\/sym_relative/link1", O_RDWR);
    EXPECT(fd < 0, "open fail");

    fd = open("\\/sym_relative/file1", O_CREAT | O_RDWR, 0777);
    EXPECT(fd > 0, "open fail");

    fd = open("\\/sym_relative/link1", O_RDWR);
    EXPECT(fd > 0, "open fail");
}

int main() {
    symlink_basic_1();
    symlink_basic_2();
    symlink_multiple();
    symlink_loop();
    symlink_relative();
    printf("symlink test passed\n");
    return 0;
}