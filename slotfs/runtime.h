#ifndef RUNTIME_H
#define RUNTIME_H

#include <sys/types.h>
#include "lock.h"
#include "inode.h"
#include <stdatomic.h>
#include <dirent.h>
typedef struct file_header {
    int         fd;
    inode_t*    file;
    off_t       offset;
    int         flags;
    atomic_bool valid;
    struct dirent* temp_dirent;
} f_header_t;

typedef struct runtime {
    f_header_t* f_table;
    int         fd_hint; 
    int         _errno;
    uid_t       uid;
    gid_t       gid;   

    inode_t*    cwd;   
    spinlock_t  lock;
    spinlock_t  openlock;
} runtime_t;

extern runtime_t rt;


static inline void set_errno(int err) {
	rt._errno = err;
}

static inline int* slotfs_errno() {
    return &rt._errno;
}

void runtime_init();
void runtime_exit();
f_header_t* fd_alloc();
void fd_free(int fd);

#endif // RUNTIME_H