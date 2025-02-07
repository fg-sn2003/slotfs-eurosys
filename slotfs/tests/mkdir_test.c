#include "test.h"

void mkdir_test() {
    int ret;

    ret = mkdir("\\mkdir", 0777);
    EXPECT(ret == 0, "mkdir failed");
}

int main() {
    mkdir_test();
    printf("mkdir test passed\n");
}