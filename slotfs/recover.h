#include <stdatomic.h>
#include "btree.h"
#define _IDX2INODE(ino) (rc->sb->slots[SLOT_INODE].slot_table + (ino) * rc->sb->slots[SLOT_INODE].slot_size)
#define _IDX2DATA(idx)  (rc->sb->slots[SLOT_DATA].slot_table + ((idx) << PAGE_SHIFT))
#define _IDX2DENT(idx) (rc->sb->slots[SLOT_DIR].slot_table + ((idx) * rc->sb->slots[SLOT_DIR].slot_size))
#define _IDX2FENT(idx) (rc->sb->slots[SLOT_FILE].slot_table + ((idx) * rc->sb->slots[SLOT_FILE].slot_size))

#define THREAD_NUM 8
typedef struct queue {
    int     front;
    int     rear;
    int*    data;
    int     capacity;
    spinlock_t lock;
} queue_t;

typedef struct rc_inode {
    index_t ino;
    index_t log_head;
    size_t  entry_num;
    bool    valid;
    time_t  ts;
    int     link;
    int     mode;
    btree_t btree;
    btree_ptr hint;
    list_head_t list;
} rc_inode_t;

typedef struct rc_filent {
    time_t  ts;
    int     ref;
    index_t p_idx;
    index_t l_idx;
    int     blocks;
    list_head_t list;
} rc_filent_t;
 
typedef struct recover_manager {
    void*        map[SLOT_TYPE_NUM];
    pm_sb_t*     sb;
    rc_inode_t** inode_table;
    queue_t*     queue;
    pthread_t*   replay_thread;
    atomic_uint_fast16_t  active_thread;
} rc_mgr_t;

int recover();