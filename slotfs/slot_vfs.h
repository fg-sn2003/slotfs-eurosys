#ifndef _SLOT_VFS_H
#define _SLOT_VFS_H

#include "inode.h"
#include "lock.h"
#include "slotfs.h"
#include "entry.h"
#include <sys/types.h> 

#define ND_PARENT   0b00001
#define ND_NOFOLLOW 0b00010

typedef struct namidata {
    const char*     path;
    inode_t*        start;
    inode_t*        current;
    inode_t*        parent;
    dirent_t*       dentry;
    uint8_t         depth;
    uint16_t        flags;
    const char*     cursor;
    const char*     last;
    int             last_len;
} nameidata_t;

static inline void inode_lock(ino_t ino) {
    spin_lock(&sbi->inode_table[ino].lock);
}

static inline void inode_unlock(ino_t ino) {
    spin_unlock(&sbi->inode_table[ino].lock);
}

inode_t *iget(unsigned long ino);
void icache_install(inode_t *inode);
int path_lookup(nameidata_t *nd);

#endif // _SLOT_VFS_H