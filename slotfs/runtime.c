#include <stdbool.h>
#include <assert.h>
#include "runtime.h"
#include "slot_vfs.h"
#include "config.h"
#include "slotfs_func.h"

runtime_t rt;

f_header_t* fd_alloc() {
    f_header_t* fh = rt.f_table;
    int again = 0;
    spin_lock(&rt.lock);
again:
    for (int i = 0; i < FD_MAX; i++) {
        int index = (rt.fd_hint + i) % FD_MAX;
        bool expected = false;
        fh = &rt.f_table[index];
        if (atomic_compare_exchange_strong(&fh->valid, &expected, true)) {
            rt.fd_hint = (index + 1) % FD_MAX;
            fh->fd = index;
            fh->offset = 0;
            fh->flags = 0;
            fh->file = NULL;
            fh->temp_dirent = NULL;
            spin_unlock(&rt.lock);
            return fh;
        }
    }

    if (!again) {
        again = 1;
        goto again;
    }
    spin_unlock(&rt.lock);
    return NULL;
}

void fd_free(int fd) {
    spin_lock(&rt.lock);
    atomic_store(&rt.f_table[fd].valid, false);
    rt.f_table[fd].file = NULL;
    spin_unlock(&rt.lock);
    // rt.fd_hint = fd;
}

void runtime_init() {
    rt.f_table = calloc(FD_MAX, sizeof(f_header_t));

    assert(rt.f_table);
    rt._errno = 0;
    rt.fd_hint = 0; 
    rt.uid = getuid();
    rt.gid = getgid();
    rt.cwd = iget(0);
    inode_unlock(0);

    spin_lock_init(&rt.lock);
    spin_lock_init(&rt.openlock);
    
    for (int i = 0; i < FD_MAX; i++) {
        rt.f_table[i].fd = i;
        rt.f_table[i].file = NULL;
        rt.f_table[i].offset = 0;
        rt.f_table[i].flags = 0;
        atomic_init(&rt.f_table[i].valid, false);
    }
}

void runtime_exit() {
    for (int i = 0; i < FD_MAX; i++) {
        if (!rt.f_table[i].file || !rt.f_table[i].valid) {
            continue;
        }
        slotfs_close(i);
    }
}