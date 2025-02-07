#include "test.h"

void rename_basic_1() {
    int ret, fd;
    mkdir("\\/rndir", 0777);
    fd = open("\\/rndir/rnfile", O_CREAT | O_RDWR, 0777);
    close(fd);

    ret = rename("\\/rndir/rnfile", "\\/rndir/rnfile2");
    assert(ret == 0);

    fd = open("\\/rndir/rnfile2", O_RDWR);
    assert(fd > 0);
    close(fd);

    ret = rename("\\/rndir/rnfile2", "\\/rnfile3");
    assert(ret == 0);
    
    fd = open("\\/rnfile3", O_RDWR);
    assert(fd > 0);
    close(fd);
}

void rename_basic_2() {
    int ret, fd;
    fd = mkdir("\\/rndir2", 0777);
    assert(fd == 0);

    fd = open("\\/rndir2/rnfile", O_CREAT | O_RDWR, 0777);
    close(fd);

    ret = rename("\\/rndir2/rnfile", "\\/rndir2/rnfile2");
    ret = rename("\\/rndir2/rnfile2", "\\/rndir2/rnfile3");
    ret = rename("\\/rndir2/rnfile3", "\\/rndir2/rnfile4");
    ret = rename("\\/rndir2/rnfile4", "\\/rndir2/rnfile5");
    ret = rename("\\/rndir2/rnfile5", "\\/rndir2/rnfile6");
    ret = rename("\\/rndir2/rnfile6", "\\/rndir2/rnfile7");
    ret = rename("\\/rndir2/rnfile7", "\\/rndir2/rnfile8");
    ret = rename("\\/rndir2/rnfile8", "\\/rndir2/rnfile9");
    ret = rename("\\/rndir2/rnfile9", "\\/rndir2/rnfile10");
    ret = rename("\\/rndir2/rnfile10", "\\/rndir2/rnfile11");
    ret = rename("\\/rndir2/rnfile11", "\\/rndir2/rnfile12");
    assert(ret == 0);

    fd = open("\\/rndir2/rnfile12", O_RDWR);
    assert(fd > 0);
    close(fd);
}

void rename_replace() {
    int ret, fd;
    fd = mkdir("\\/rndir3", 0777);
    assert(fd == 0);

    fd = open("\\/rndir3/rnfile", O_CREAT | O_RDWR, 0777);
    close(fd);

    fd = open("\\/rndir3/rnfile2", O_CREAT | O_RDWR, 0777);
    close(fd);

    ret = rename("\\/rndir3/rnfile", "\\/rndir3/rnfile2");
    assert(ret < 0);        //not supported yet
}

int main() {
    rename_basic_1();
    rename_basic_2();
    rename_replace();
    printf("rename test passed\n");
    return 0;
}