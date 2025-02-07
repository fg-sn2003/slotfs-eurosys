#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include "slotfs.h"
#include "recover.h"
#include "btree.h"
#include "bitops.h"
rc_mgr_t *rc;

static int queue_init(queue_t *q, int size) {
    q->data = malloc(size * sizeof(int));
    if (!q->data) return -ENOMEM;
    q->front = q->rear = 0;
    q->capacity = size;
    spin_lock_init(&q->lock);
    return 0;
}

static int queue_push(queue_t *q, int val) {
    spin_lock(&q->lock);
    if ((q->rear - q->front) >= q->capacity) {
        spin_unlock(&q->lock);
        assert(0);
        return -1;
    }
    q->data[q->rear++] = val;
    spin_unlock(&q->lock);
    return 0;
}

static int queue_pop(queue_t *q, int *val) {
    spin_lock(&q->lock);
    if (q->front >= q->rear) {
        spin_unlock(&q->lock);
        return -1;
    }
    *val = q->data[q->front++];
    spin_unlock(&q->lock);
    return 0;
}

static inline rc_inode_t* rc_iget(index_t ino) {
    rc_inode_t *inode = rc->inode_table[ino];
    if (inode == NULL) {
        inode = malloc(sizeof(rc_inode_t));
        if (!inode) return NULL;
        rc->inode_table[ino] = inode;
        INIT_LIST_HEAD(&inode->list);
    }
    return inode;
}

int rc_init() {
    rc = malloc(sizeof(rc_mgr_t));
    if (!rc) return -ENOMEM;

    rc->sb = (pm_sb_t *)DAX_START;

    // Init bitmap
    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        rc->map[i] = malloc((rc->sb->slots[i].slot_num + 7) / 8);
        if (!rc->map[i]) return -ENOMEM;
        memset(rc->map[i], 0, (rc->sb->slots[i].slot_num + 7) / 8);
    }

    // Init inode table
    size_t inode_num = rc->sb->slots[SLOT_INODE].slot_num;
    rc->inode_table = malloc(inode_num * sizeof(rc_inode_t *));
    if (!rc->inode_table) return -ENOMEM;
    memset(rc->inode_table, 0, inode_num * sizeof(rc_inode_t *));
    
    // Init queue
    rc->queue = malloc(sizeof(queue_t));
    if (!rc->queue) return -ENOMEM;

    int ret = queue_init(rc->queue, rc->sb->slots[SLOT_INODE].slot_num);
    if (ret) return ret;

    rc->replay_thread = malloc(sizeof(pthread_t) * THREAD_NUM);

    // Init root inode
    rc_inode_t *root = rc_iget(0);
    root->valid = true;
    root->link = 1;
    root->ts = 0;

    return 0;
}

void rc_free() {
    int inode_num = rc->sb->slots[SLOT_INODE].slot_num;
    for (int i = 0; i < inode_num; i++) {
        if (!rc->inode_table[i]) 
            continue;

        while (!list_empty(&rc->inode_table[i]->list)) {
            rc_filent_t *filent = list_first_entry(&rc->inode_table[i]->list, rc_filent_t, list);
            list_del(&filent->list);
            free(filent);
        }
        free(rc->inode_table[i]);
    }

    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        free(rc->map[i]);
    }


    free(rc->inode_table);
    free(rc->queue->data);
    free(rc->queue);
    free(rc->replay_thread);
    free(rc);
}

void rc_flush() {
    int inode_num = rc->sb->slots[SLOT_INODE].slot_num;
    for (int i = 0; i < inode_num; i++) {
        rc_inode_t *inode = rc->inode_table[i];
        if (!inode) 
            continue;
        
        pm_inode_t *pm_inode = (pm_inode_t*)REL2ABS(_IDX2INODE(i));
        pm_inode->entry_num = inode->entry_num;
        flush_byte(&pm_inode->entry_num);
    }

    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        void *pm_map = REL2ABS(rc->sb->slots[i].bitmap);
        memcpy(pm_map, rc->map[i], (rc->sb->slots[i].slot_num + 7) / 8);
        flush_cache(pm_map, (rc->sb->slots[i].slot_num + 7) / 8, true);
    }
}


// int queue[65536];
// int scan_dirent() {
//     int front = 0, rear = 0;
//     queue[rear++] = 0; // root inode index  

//     while (front < rear) {
//         int idx = queue[front++];
//         rc_inode_t *inode = &rc->inode_table[idx];

//         // Read PM inode directly
//         pm_inode_t *pi = (pm_inode_t*)REL2ABS(_IDX2INODE(idx));
//         inode->mode = pi->i_mode;
//         inode->log_head = pi->log_head;
//         index_t ent_idx = pi->log_head;
//         printf("inode %d, log_head %d\n", idx, ent_idx);
//         if (inode->mode & S_IFDIR) {
//             while (ent_idx) {
//                 printf("dirent_idx %d\n", ent_idx);
//                 pm_dirent_t *pde = (pm_dirent_t*)REL2ABS(_IDX2DENT(ent_idx));
//                 rc->inode_table[pde->ino].link++;
//                 if (rc->inode_table[pde->ino].link == 1) {
//                     queue[rear++] = pde->ino;
//                 }
//                 inode->entry_num++;
//                 ent_idx = pde->next;
//             }
//         } else { 
//             while (ent_idx) {
//                 printf("filent_idx %d\n", ent_idx);
//                 pm_filent_t *pfe = (pm_filent_t*)REL2ABS(_IDX2FENT(ent_idx));
//                 inode->entry_num++;
//                 ent_idx = pfe->next;
//             }
//         }
//     }

//     return 0;
// }


static int rc_handle_dirent(rc_inode_t *inode, index_t idx, index_t *next) {
    pm_dirent_t *de = (pm_dirent_t*)REL2ABS(_IDX2DENT(idx));
    rc_inode_t *child = rc_iget(de->ino);
    if (!child) return -ENOMEM;

    set_bit(idx, rc->map[SLOT_DIR]);
    child->link++;
    if (child->link == 1) {
        queue_push(rc->queue, de->ino);
    }
    inode->entry_num++;
    *next = de->next;
    return 0;
}

static int rc_handle_filent(rc_inode_t *inode, index_t idx, index_t *next) {
    pm_filent_t *fe = (pm_filent_t*)REL2ABS(_IDX2FENT(idx));
    set_bit(idx, rc->map[SLOT_FILE]);
    
    rc_filent_t *filent = (rc_filent_t*)malloc(sizeof(rc_filent_t));
    if (!filent) return -ENOMEM;

    filent->ts = fe->ts;
    filent->p_idx = fe->p_idx;
    filent->blocks = fe->blocks;
    filent->l_idx = fe->l_idx;
    *next = fe->next;
    if (*next == 0)  // ghost slot
        return 0;
    
    // __builtin_prefetch(next, 0, 3);
    volatile char temp = *(char *)next;
    void **ptr;
    rc_filent_t *old_filent;

    for (int i = 0; i < filent->blocks; i++) {
        index_t idx = filent->l_idx + i;
        ptr = btree_get_ptr_hint(&inode->btree, idx, &inode->hint);
        debug_assert(ptr);

        old_filent = (rc_filent_t *)*ptr;
        if (old_filent) {
            if (old_filent->ts < filent->ts) {
                *ptr = filent;
                old_filent->ref--;
                clear_bit(old_filent->p_idx + (old_filent->l_idx - idx), rc->map[SLOT_DATA]);
                if (old_filent->ref == 0) {
                    list_del(&old_filent->list);
                    free(old_filent);
                }
            } else {
                clear_bit(filent->p_idx + (filent->l_idx - idx), rc->map[SLOT_DATA]);
            }
        } else {
            *ptr = filent;
            filent->ref++;
        }
    }

    if (filent->ref == 0) {
        // list_del(&filent->list);
        free(filent);
    } else {
        list_add_tail(&filent->list, &inode->list);
    }

    inode->entry_num++;
    return 0;
}

int rc_handle_inode(index_t ino) {
    int ret;
    rc_inode_t *inode = rc_iget(ino);
    if (!inode) return -ENOMEM;

    pm_inode_t *pi = (pm_inode_t*)REL2ABS(_IDX2INODE(ino));
    inode->mode = pi->i_mode;
    inode->log_head = pi->log_head;
    set_bit(ino, rc->map[SLOT_INODE]);
    if (!(inode->mode & S_IFDIR)) {
        btree_create(&inode->btree);
        inode->hint.node = NULL;
        inode->hint.index = 0;
    }

    index_t idx = pi->log_head;
    index_t next = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (idx) {
        if (inode->mode & S_IFDIR) {
            ret = rc_handle_dirent(inode, idx, &next);
        } else {
            struct timespec start, end;
            ret = rc_handle_filent(inode, idx, &next);
        }
        if (ret) return ret;
        idx = next;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("inode %d, entry_num %d, time %ld\n", ino, inode->entry_num, (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec));
    return 0;
}

static void* rc_replay(void *data) {
    while (1) {
        int idx, ret;
        ret = queue_pop(rc->queue, &idx);
        if (ret) {
            atomic_fetch_sub(&rc->active_thread, 1);
            while (1) {
                if (rc->active_thread == 0) return 0;
                if ((rc->queue->rear - rc->queue->front) > 0) {
                    atomic_fetch_add(&rc->active_thread, 1);
                    break;
                }
                for (int i = 0; i < 10000; i++) {}      // a little delay
            }
            continue;
        }
        ret = rc_handle_inode(idx);
        if (ret) return NULL;
    }
}

int recover() {
    int ret;
    btree_module_init_default();
    ret = rc_init();
    if (ret) return -ENOMEM;

    queue_push(rc->queue, 0);

    atomic_init(&rc->active_thread, THREAD_NUM);
    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&rc->replay_thread[i], NULL, rc_replay, NULL);
    }

    for (int i = 0; i < THREAD_NUM; i++) {
        pthread_join(rc->replay_thread[i], NULL);
    }
    rc_flush();

    rc_free();

    return 0;
}