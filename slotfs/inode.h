#ifndef INODE_H
#define INODE_H

#include "config.h"
#include "lock.h"
#include "rbtree.h"
#include "btree.h"
#include "entry.h"
struct pm_inode {
    size_t      i_size;
    uid_t       i_uid;
    gid_t       i_gid;
    nlink_t     i_link;
    mode_t      i_mode;

    time_t      i_atim;     /* Time of last access */
    time_t      i_mtim;     /* Time of last modification */
    time_t      i_ctim;     /* Time of last status change */

    index_t     log_head;  // log head index in slot entry table
    size_t      entry_num;
    time_t      ts;
    uint32_t csum;
};

struct dram_inode {
    ino_t       i_ino;
    size_t      i_size;
    time_t      i_ctim;
    uid_t       i_uid;
    gid_t       i_gid;
    nlink_t     i_link;
    mode_t      i_mode;
    
    time_t      i_atim;     
    time_t      i_mtim;     
    time_t      ts;

    atomic_uint ref;
    union {
        rb_root_t rb_root;
        btree_t   b_root;        
    };
    btree_ptr   b_hint;   //shortcut
    size_t      entry_num;
    size_t      invalid_num;
    index_t     f_slot_cache;
    int         f_slot_cache_off;
    index_t     log_head;  
    index_t     ghost_slot;
    list_head_t     list;
    list_head_t     invalid;
    range_lock_t    r_lock;
    spinlock_t      s_lock;

    list_head_t     release;
};

typedef struct pm_inode pm_inode_t;
typedef struct dram_inode inode_t;

typedef struct entry_unit {
    size_t     blocks;
    index_t    l_idx;
    index_t    p_idx;
} entry_unit_t;

#define EBUF_NUM    128
#define EBUF_SIZE   512
typedef struct entry_buffer {
    entry_unit_t arr[EBUF_SIZE];
    int          num;
    int          size;
    int          idx;
    spinlock_t   lock;
} ebuf_t;

typedef struct entry_buffer_manager {
    ebuf_t  entry_buffer[EBUF_NUM];
    size_t  num;
} ebuf_mgr_t;

static inline int ebuf_mgr_init(ebuf_mgr_t *mgr) {
    mgr->num = EBUF_NUM;
    for (int i = 0; i < EBUF_NUM; i++) {
        mgr->entry_buffer[i].idx = i;
        mgr->entry_buffer[i].num = 0;
        mgr->entry_buffer[i].size = EBUF_SIZE;
        spin_lock_init(&mgr->entry_buffer[i].lock);
    }
    return 0;
}

static inline int is_dir(inode_t *inode){
    return (inode->i_mode & S_IFMT) == S_IFDIR;
}
static inline int is_reg(inode_t *inode){
    return (inode->i_mode & S_IFMT) == S_IFREG;
}
static inline int is_symlink(inode_t *inode){
    return (inode->i_mode & S_IFMT) == S_IFLNK;
}
static inline int is_exec(inode_t *inode){
    return (inode->i_mode & S_IXUSR);
}

inode_t*    inode_create(inode_t *parent, const char* name, int name_len, mode_t mode);
dirent_t*   dirent_lookup(inode_t *dir, unsigned long hash);
dirent_t*   inode_lookup(inode_t *dir, const char *name, int name_len);
int         inode_rebuild(inode_t* inode);
ssize_t     inode_read(inode_t *inode, void *buf, size_t len, off_t offset);
ssize_t     inode_write(inode_t *inode, const void *buf, size_t len, off_t offset);
void        inode_flush(inode_t *inode);
int         inode_remove(inode_t *dir, dirent_t *d, inode_t *dest_inode);
dirent_t*   dir_append_entry(inode_t *dir, inode_t* inode, const char* name, int name_len);
void        dir_remove_entry(inode_t *dir, dirent_t *d);
ssize_t     inode_readlink(inode_t *inode, char *link);
void        inode_release(inode_t *inode);
void        do_inode_release(inode_t *inode);
int         inode_resize(inode_t *inode, size_t new_size);
filent_t*   filent_lookup(inode_t *file, index_t idx);
#endif // INODE_H