#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include "inode.h"
#include "slot.h"
#include "slotfs.h"
#include "btree.h"
#include "config.h"
#include "list.h"
#include "slot_vfs.h"
#include <immintrin.h>

ebuf_t* ebuf_alloc() {
    int cpuid = sched_getcpu();
    ebuf_mgr_t *mgr = sbi->ebuf_mgr;
    ebuf_t *ebuf = &mgr->entry_buffer[cpuid % EBUF_NUM];
    
    spin_lock(&ebuf->lock);
    ebuf->num = 0;
    return ebuf;
}

void ebuf_put(ebuf_t *ebuf) {
    spin_unlock(&ebuf->lock);
}

void ebuf_add(ebuf_t *ebuf, index_t l_idx, index_t p_idx, size_t blocks) {
    ebuf->arr[ebuf->num].l_idx = l_idx;
    ebuf->arr[ebuf->num].p_idx = p_idx;
    ebuf->arr[ebuf->num].blocks = blocks;
    ebuf->num++;
}

entry_unit_t* ebuf_get(ebuf_t *ebuf, int idx) {
    return ebuf->arr + idx;
}

typedef struct {
    inode_t *inode;
} filent_ctx_t;

int dir_index_insert(inode_t *dir, dirent_t *d);

int filent_insert_callback(void *ptr, int idx, void* ctx) {
    filent_t *fe = (filent_t *)ptr;
    fe->ref--;
    filent_ctx_t *fctx = (filent_ctx_t *)ctx;
    inode_t *inode = fctx->inode;
    slot_data_free(inode, fe->p_idx + (idx - fe->l_idx), 1);
    debug_assert(fe->ref >= 0);
    
    if (fe->ref == 0) {
        list_add_tail(&fe->invalid_list, &inode->invalid);
        inode->invalid_num++;
    }

    return 0;
}

void inode_init_header(inode_t *inode, mode_t mode) {
    memset(inode, 0, sizeof(inode_t));
    
    if (mode & S_IFDIR) {
        inode->rb_root = RB_ROOT;
    } else {
        btree_create(&inode->b_root);
    }

    inode->b_hint.node = NULL;
    inode->b_hint.index = 0;
    inode->b_hint.key = 0;
    INIT_LIST_HEAD(&inode->list);
    INIT_LIST_HEAD(&inode->invalid);
    range_lock_init(&inode->r_lock);    
    spin_lock_init(&inode->s_lock);
    inode->i_mode = mode;
    inode->ref = 0;
}

dirent_t* dirent_lookup(inode_t *dir, unsigned long hash) {
    rb_root_t *tree = &dir->rb_root;
    dirent_t *curr = NULL;
    rb_node_t *temp = tree->rb_node;
    
    while (temp) {
        curr = container_of(temp, dirent_t, rb_node);
        if (hash < curr->hash) {
            temp = temp->rb_left;
        } else if (hash > curr->hash) {
            temp = temp->rb_right;
        } else {
            return curr;
        }
    }
    return NULL;
}

dirent_t* inode_lookup(inode_t *dir, const char *name, int name_len) {
    unsigned long hash;
    if (unlikely(!is_dir(dir))) {
        return ERR_PTR(ENOTDIR);
    }

    hash = BKDRHash(name, name_len);

    return dirent_lookup(dir, hash);
}

filent_t* filent_lookup(inode_t *file, index_t idx) {
    btree_t *tree = &file->b_root;

#ifdef SLOTFS_DEBUG
    filent_t *a = btree_get(tree, idx);
    filent_t *b = btree_get_hint(tree, idx, &file->b_hint);

    assert(a == b);
    return a;
#endif
    // filent_t *fe = btree_get_hint(tree, idx, &file->b_hint);
    filent_t *fe = btree_get(tree, idx);

    return fe;
}

dirent_t* dir_append_entry(inode_t *dir, inode_t* inode, const char* name, int name_len) {
    dirent_t *d;
    int ret;

    d = dirent_alloc();
    if (unlikely(!d)) return ERR_PTR(-ENOMEM);
    index_t dno = dir->ghost_slot;
    
    dir->ghost_slot = slot_dirent_alloc(dir);

    if (unlikely(dir->ghost_slot < -1)) {
        dir->ghost_slot = dno;
        dirent_free(d);
        return ERR_PTR(-ENOSPC);
    }
    
    d->ino  = inode->i_ino;
    d->ts   = inode->ts;
    d->hash = BKDRHash(name, name_len);
    d->idx  = dno;
    ret = dir_index_insert(dir, d);
    debug_assert(!ret);
    
    pm_dirent_t *pd = (pm_dirent_t *)REL2ABS(IDX2DENT(dno));
    pd->ino      = inode->i_ino;
    pd->ts       = inode->ts;
    pd->next     = dir->ghost_slot;
    pd->name_len = name_len;
    memcpy(pd->name, name, name_len);
    flush_cache(pd, sizeof(pm_dirent_t), 1);

    inode->entry_num++;
    list_add_tail(&d->list, &dir->list);
    return d;
}

void dir_remove_entry(inode_t *dir, dirent_t *d) {
    dirent_t *prev_d, *next_d;
    pm_dirent_t *pd;
    index_t next_ptr, *prev_ptr;
    pm_inode_t *p_dir = (pm_inode_t *)REL2ABS(IDX2INODE(dir->i_ino));
    prev_d = container_of(d->list.prev, dirent_t, list);
    next_d = container_of(d->list.next, dirent_t, list);
    
    rb_erase(&d->rb_node, &dir->rb_root);
    // if the dirent is the head of the list
    // then the previous pointer should be the log_head
    if (dir->log_head == d->idx) {
        prev_ptr = &p_dir->log_head;
        if (d->list.next == &dir->list) {
            dir->log_head = dir->ghost_slot;
        } else {
            dir->log_head = next_d->idx;
        }
    } else {
        pd = (pm_dirent_t *)REL2ABS(IDX2DENT(prev_d->idx));
        prev_ptr = &pd->next;
    }

    // if the dirent is the tail of the list
    // then the next pointer should be the ghost slot
    if (d->list.next == &dir->list) {
        next_ptr = dir->ghost_slot;
    } else {
        next_ptr = next_d->idx;
    } 
    
    // update the next pointer of the previous dirent
    *prev_ptr = next_ptr;
    flush_byte(prev_ptr);

    // Set the next pointer to 0 AFTER the entry is removed from the link
    pd = (pm_dirent_t *)REL2ABS(IDX2DENT(d->idx));
    pd->next = 0;
    flush_byte(&pd->next);

    slot_dirent_free(dir, d->idx);
    list_del(&d->list);
    dirent_free(d);
    dir->entry_num--;
}

void file_remove_entry(inode_t *file, filent_t *fe) {
    filent_t *prev_fe, *next_fe;
    pm_filent_t *pf;
    index_t next_ptr, *prev_ptr;
    pm_inode_t *p_file = (pm_inode_t *)REL2ABS(IDX2INODE(file->i_ino));

    prev_fe = container_of(fe->list.prev, filent_t, list);
    next_fe = container_of(fe->list.next, filent_t, list);

    // if the dirent is the head of the list
    // then the previous pointer should be the log_head
    if (file->log_head == fe->idx)
        prev_ptr = &p_file->log_head;
    else {
        pf = (pm_filent_t *)REL2ABS(IDX2FENT(prev_fe->idx));
        prev_ptr = &pf->next;
    }

    // if the dirent is the tail of the list
    // then the next pointer should be the ghost slot
    if (fe->list.next == &file->list) 
        next_ptr = file->ghost_slot;
    else 
        next_ptr = next_fe->idx;

    // update the next pointer of the previous dirent
    *prev_ptr = next_ptr;
    flush_byte(prev_ptr);

    // Set the next pointer to 0 AFTER the entry is removed from the link
    pf = (pm_filent_t *)REL2ABS(IDX2FENT(fe->idx));
    pf->next = 0;
    flush_byte(&pf->next);

    btree_set_range(&file->b_root, fe->l_idx, fe->l_idx + fe->blocks, NULL);

    slot_firent_free(file, fe->idx);
    filent_free(fe);
    list_del(&fe->list);
    file->entry_num--;
}

int init_ghost_slot(inode_t *inode) {
    inode->ghost_slot = is_dir(inode) ? slot_dirent_alloc(inode) 
        : slot_firent_alloc(inode);
    if (inode->ghost_slot < 0) {
        return -ENOSPC;
    }
    inode->log_head = inode->ghost_slot;
    return 0;
}

int inode_remove(inode_t *dir, dirent_t *d, inode_t *dest_inode) {
    dir_remove_entry(dir, d);
    dest_inode->i_link--;
    return 0;
}

inode_t* inode_create(inode_t *parent, const char* name, int name_len, mode_t mode) {
    int ret;

    debug_assert(is_dir(parent));
    ino_t ino = slot_inode_alloc(parent);
    if (unlikely(ino < 0)) {
        return ERR_PTR(-ENOSPC);
    }

    inode_t *inode = inode_alloc();
    if (unlikely(!inode)) {
        ret = -ENOMEM;
        goto err_free_ino;
    }
    inode_init_header(inode, mode);
    inode->i_ino     = ino;
    inode->i_link    = 1;
    inode->i_mode    = mode;
    inode->i_atim    = inode->i_ctim = inode->i_mtim = time(NULL);
    ret = init_ghost_slot(inode);
    if (unlikely(ret)) {
        goto err_free_inode;
    }
    // write the inode to the pm and atomic append the entry that points to inode
    inode_flush(inode);
    sfence();
    
    dirent_t* d = dir_append_entry(parent, inode, name, name_len);
    
    if (unlikely(IS_ERR(d))) {
        ret = PTR_ERR(d);
        goto err_free_inode;
    }

    return inode;    

err_free_inode:
    inode_free(inode);
err_free_ino:
    slot_inode_free(parent, ino);

    return ERR_PTR(ret);
}

int dir_index_insert(inode_t *dir, dirent_t *d) {
    dirent_t *curr;
    rb_root_t *tree = &dir->rb_root;
    rb_node_t **temp, *parent;
    int ret = 0;

    temp = &(tree->rb_node);
    parent = NULL;

    while (*temp) {
        curr = container_of(*temp, dirent_t, rb_node);
        parent = *temp;

        if (d->hash < curr->hash) {
            temp = &((*temp)->rb_left);
        } else if (d->hash > curr->hash) {
            temp = &((*temp)->rb_right);
        } else {
            ret = -EEXIST;
            return ret;
        }
    }

    rb_link_node(&d->rb_node, parent, temp);
    rb_insert_color(&d->rb_node, tree);    

    return 0; 
}

int file_index_insert(inode_t *file, filent_t *fe) {
    static unsigned long total_time = 0;
    btree_t *tree = &file->b_root;
    filent_ctx_t fctx = { .inode = file};
    int ret = btree_set_range_callback(tree, fe->l_idx, 
        fe->l_idx + fe->blocks, fe, filent_insert_callback, &fctx);
    // int ret =  btree_set_range_hint_callbak(tree, fe->l_idx, 
    //     fe->l_idx + fe->blocks, fe, &file->b_hint, filent_insert_callback, &fctx);

    return ret;
}

static int dir_rebuild_insert(inode_t *dir, pm_dirent_t *pd, index_t idx) {
    dirent_t * d;
    int ret;

    d = dirent_alloc();
    if (!d) return -ENOMEM;

    d->hash = BKDRHash(pd->name, pd->name_len);
    d->ino  = pd->ino;
    d->ts   = pd->ts;
    d->idx  = idx;
    ret = dir_index_insert(dir, d);
    if (ret) {
        dirent_free(d);
        return ret;
    }

    list_add_tail(&d->list, &dir->list);
    return 0;
}

static int dir_rebuild(inode_t *inode) {
    pm_dirent_t pd;
    index_t curr = inode->log_head;
    int ret = 0;

    inode->ghost_slot = inode->log_head;
    for (int i = 0; i < inode->entry_num; i++) {
        memcpy(&pd, REL2ABS(IDX2DENT(curr)), sizeof(dirent_t));
        ret = dir_rebuild_insert(inode, &pd, curr);
        if (ret) 
            return ret;

        curr = pd.next;
        inode->ghost_slot = pd.next;
    }
    return 0;
}

static int file_rebuild(inode_t *inode) {
    return 0;
}

int inode_rebuild(inode_t* inode) {
    pm_inode_t pm_inode;
    index_t ino = inode->i_ino;
    pm_inode_t *pi = (pm_inode_t *)REL2ABS(IDX2INODE(ino));
    memcpy(&pm_inode, pi, sizeof(pm_inode_t));

    inode_init_header(inode, pm_inode.i_mode);
    inode->i_size   = pm_inode.i_size;
    inode->i_uid    = pm_inode.i_uid;
    inode->i_gid    = pm_inode.i_gid;
    inode->i_mode   = pm_inode.i_mode;
    inode->i_atim   = time(NULL);
    inode->i_mtim   = pm_inode.i_mtim;
    inode->i_ctim   = pm_inode.i_ctim;
    inode->log_head = pm_inode.log_head;
    inode->i_link   = pm_inode.i_link;
    inode->i_ino    = ino;
    inode->entry_num = pm_inode.entry_num;
    return is_dir(inode) ? dir_rebuild(inode) 
        : file_rebuild(inode);
}

filent_t* filent_try_reuse(inode_t *inode) {
    filent_t *fe;
    if (list_empty(&inode->invalid)) {
        return NULL;
    }

    fe = list_first_entry(&inode->invalid, filent_t, invalid_list);
    list_del(&fe->invalid_list);
    inode->invalid_num--;
    return fe;
}

filent_t *filent_dispatch(inode_t *inode, size_t size) {
    float space_ratio = 1 - (float)inode->invalid_num / (float)inode->entry_num;

    float ratio = size / (PM_CACHELINE_SIZE * space_ratio * 100);
    
    if (ratio < APPEND_THRESHOLD) {
        return NULL;
    } else {
        inode->invalid_num--;
        return filent_try_reuse(inode);
    }
}

filent_t* file_append_entry(inode_t *inode, pm_filent_t* entry) {
    filent_t *fe;
    int ret;
    int reuse = 0;
    index_t reuse_next;
    index_t fno;

    fe = filent_dispatch(inode, entry->blocks << PAGE_SHIFT);
    if (fe) {
        reuse = 1;
        filent_t *next;
        if (list_is_last(&fe->list, &inode->list)) {
            reuse_next = inode->ghost_slot;
        } else {
            next = list_entry(fe->list.next, filent_t, list);
            reuse_next = next->idx;
        }
        fno = fe->idx;
    } else {
        fe = filent_alloc();
        if (unlikely(!fe)) return ERR_PTR(-ENOMEM);
        
        fno = inode->ghost_slot;
        if (unlikely(is_symlink(inode))) {
            inode->ghost_slot = 0;
        } else {
            inode->ghost_slot = slot_firent_alloc(inode);
        }
        
        if (unlikely(inode->ghost_slot < -1)) {
            inode->ghost_slot = fno;
            filent_free(fe);
            return ERR_PTR(-ENOSPC);
        }
    }

    fe->idx   = fno;
    fe->ts    = entry->ts;
    fe->l_idx = entry->l_idx;
    fe->p_idx = entry->p_idx;
    fe->blocks = entry->blocks;
    fe->ref   = fe->blocks;
    if (!reuse)
        list_add_tail(&fe->list, &inode->list);

    ret = file_index_insert(inode, fe);
    debug_assert(!ret);
    
    pm_filent_t *pf = (pm_filent_t *)REL2ABS(IDX2FENT(fno));
    
    sfence();
    entry->next = reuse ? reuse_next : inode->ghost_slot;
    entry->csum = crc32c(0, (uint8_t *)entry, sizeof(pm_filent_t));
    avx_cpy(pf, entry, sizeof(pm_filent_t));
    sfence();
    
    if (!reuse) {
        inode->entry_num++;
    }

    return fe;
}

ssize_t inode_readlink(inode_t *inode, char *link) {
    size_t len = inode->i_size;
    filent_t *fe;
    fe = filent_lookup(inode, 0);
    
    debug_assert(len > 0 && len < MAX_STR_LEN);
    debug_assert(is_symlink(inode));
    debug_assert(fe);

    memcpy(link, REL2ABS(IDX2DATA(fe->p_idx)), len);
    return len;
}

ssize_t inode_read(inode_t *inode, void *buf, size_t len, off_t offset) {
    unsigned long idx, start_idx, end_idx;
    if (len > inode->i_size - offset) {
        len = inode->i_size - offset;
    }
    if (len <= 0)
        return 0;
    size_t left = len;
    idx = offset >> PAGE_SHIFT;
    start_idx = idx;
    end_idx = (offset + len - 1) >> PAGE_SHIFT;
    // range_lock(&inode->r_lock, start_idx, end_idx);
    inode_lock(inode->i_ino);
    do {
        /* nr is the maximum number of bytes to copy in this iterations */
        unsigned long page_offset = offset & PAGE_MASK;
        unsigned long idx = offset >> PAGE_SHIFT;
        unsigned long nr;
        void *pmem = NULL;
        filent_t *fe;
        int zero = 0;

        fe = filent_lookup(inode, idx);
        if (unlikely(!fe)) {
            zero = 1;
            nr = left > (PAGE_SIZE - page_offset) 
                ? (PAGE_SIZE - page_offset) : left;
        } else {
            debug_assert(idx >= fe->l_idx && idx <= fe->l_idx + fe->blocks);
            nr = left > ((fe->blocks << PAGE_SHIFT) - page_offset) 
                ? ((fe->blocks << PAGE_SHIFT) - page_offset) : left; 
        }
        
        if (zero) {
            memset(buf, 0, nr);
        } else {
            pmem = (void *)REL2ABS(IDX2DATA(fe->p_idx + (idx - fe->l_idx)));
            memcpy(buf, pmem + page_offset, nr);
        }
        
        logger_trace("%s: read %lu bytes at offset %lu, zero: %d, pmem: %p\n", 
            __func__, nr, offset, zero, pmem + page_offset);
        
        left -= nr;
        buf += nr;
        offset += nr;
    } while (left > 0);

    inode_unlock(inode->i_ino);
    // range_unlock(&inode->r_lock, start_idx, end_idx);
    return len;
}

ssize_t inode_write_inplace(inode_t *inode, const void *buf, size_t len, off_t offset) {
    size_t page_offset, c;
    unsigned long num_blk, start_blk, _start_blk, end_blk, _end_blk;
    size_t bytes, count, entry_num;
    void *pmem;
    int ret; 

    page_offset = offset & PAGE_MASK;
    num_blk     = (len + page_offset + PAGE_SIZE - 1) >> PAGE_SHIFT;
    start_blk   = offset >> PAGE_SHIFT;
    end_blk     = start_blk + num_blk - 1;
    _start_blk  = start_blk;
    _end_blk    = end_blk;
    c = offset;
    count = len;
    entry_num = 0;

    ebuf_t *ebuf = ebuf_alloc();

    // range_lock(&inode->r_lock, _start_blk, _end_blk);
    inode_lock(inode->i_ino); 
    while (num_blk > 0) {
        page_offset = c & PAGE_MASK;
        start_blk = c >> PAGE_SHIFT;
        
        filent_t *fe;
        if (c == inode->i_size && page_offset == 0) {
            fe = NULL;
        } else {
            fe = filent_lookup(inode, start_blk);
        }
        
        // inplace write
        if (fe) {
            bytes = (fe->blocks + fe->l_idx - start_blk) << PAGE_SHIFT - page_offset;
            if (bytes > count) 
                bytes = count;
            
            pmem = (void *)REL2ABS(IDX2DATA(fe->p_idx + (start_blk - fe->l_idx)));
            avx_cpy(pmem + page_offset, buf, bytes);
            num_blk -= (bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;
        } else {
            unsigned long nr = 0;
            unsigned long allocated = num_blk;
            nr = slot_data_alloc(inode, &allocated);
            if (unlikely(nr < 0)) {
                ret = -ENOSPC;
                debug_assert(0);      
                goto error;
            }
            pmem = (void *)REL2ABS(IDX2DATA(nr));
            bytes = PAGE_SIZE * allocated - page_offset;
            if (bytes > count)
                bytes = count;

            avx_cpy(pmem + page_offset, buf, bytes);
            ebuf_add(ebuf, start_blk, nr, allocated);
            entry_num++;
            num_blk -= allocated;
        }

        c += bytes;
        buf += bytes;
        count -= bytes;
    }

    if (entry_num > 0) {
        time_t ts = slotfs_timestamp();
        int idx = 0;
        while(idx < entry_num) {
            entry_unit_t *unit = ebuf_get(ebuf, idx);
            pm_filent_t pf;
            pf.l_idx = unit->l_idx;
            pf.p_idx = unit->p_idx;
            pf.blocks = unit->blocks;
            pf.ts = ts;
    
            filent_t *fe = file_append_entry(inode, &pf);
            if (IS_ERR(fe)) {
                ret = PTR_ERR(fe);
                inode_unlock(inode->i_ino);
                goto error;
            }
            idx++;    
        }
    }
    inode_unlock(inode->i_ino);
    ebuf_put(ebuf);

    inode->i_size = max(inode->i_size, offset + len);
    inode->i_ctim = inode->i_mtim = time(NULL);

    sfence();
    pm_inode_t *pi = (pm_inode_t *)REL2ABS(IDX2INODE(inode->i_ino));
    pi->i_size = inode->i_size;
    pi->i_ctim = inode->i_ctim;
    flush_cache(pi, sizeof(pm_inode_t), 0);

    return len;
error:
    debug_assert(0);
    inode_unlock(inode->i_ino);
    // range_unlock(&inode->r_lock, _start_blk, end_blk);
    ebuf_put(ebuf);
    return ret;
}

// just for testing purpose: test the upper boundaries bandwidth
ssize_t inode_write_fake(inode_t *inode, const void *buf, size_t len, off_t offset) {
    static int i = 0;
    void *pmem = REL2ABS(IDX2DATA(i));

    i = i + (len + PAGE_SIZE - 1) / PAGE_SIZE;

    // memmove_movnt_avx512f_clwb(pmem, buf, len);
    // flush_cache(pmem, len, 1);
    // pmem_memcpy(pmem, buf, len, 0);
    // memcpy(pmem, buf, len);
    // printf("pmem = %p, len = %lu\n", pmem, len);
    memset_nt(pmem, 0, len);
    inode->i_size += len;
    return len;
}

static int inode_write_journal(inode_t *inode, const void *buf, size_t len, off_t offset) {
    index_t blk_idx = offset >> PAGE_SHIFT;
    filent_t *fe = filent_lookup(inode, blk_idx);
    if (unlikely(!fe)) {
        return -1;      
    }

    relptr_t jnl_rel = journal_alloc();
    relptr_t begin_rel = IDX2DATA(fe->p_idx + (blk_idx - fe->l_idx)) + (offset & PAGE_MASK);
    journal_write(jnl_rel, inode->i_ino, begin_rel, len);
    
    // inplace write protected by journal
    memcpy(REL2ABS(begin_rel), buf, len);
    flush_cache(REL2ABS(begin_rel), len, 0);
    
    journal_commit(jnl_rel);
    return len;
}

ssize_t inode_write_atomic(inode_t *inode, const void *buf, size_t len, off_t offset) {
    size_t page_offset, c;
    unsigned long num_blk, start_blk, _start_blk, end_blk, _end_blk;
    size_t bytes, count, entry_num;
    void *pmem;
    int ret; 

    page_offset = offset & PAGE_MASK;
    num_blk     = (len + page_offset + PAGE_SIZE - 1) >> PAGE_SHIFT;
    start_blk   = offset >> PAGE_SHIFT;
    end_blk     = start_blk + num_blk - 1;
    _start_blk  = start_blk;
    _end_blk    = end_blk;
    c = offset;
    count = len;
    entry_num = 0;

    if (unlikely(num_blk == 1 && len < 1024 && c != inode->i_size)) {
        ret = inode_write_journal(inode, buf, len, offset);
        if (ret > 0)
            return ret;
    }
    ebuf_t *ebuf = ebuf_alloc();
    // range_lock(&inode->r_lock, _start_blk, _end_blk);

    inode_lock(inode->i_ino);
    while (num_blk > 0) {
        unsigned long page_offset = c & PAGE_MASK;
        unsigned long nr = 0;
        unsigned long allocated = 0;
        start_blk = c >> PAGE_SHIFT;
        // if the write is small and can write in place at the end of the file
        if (c == inode->i_size && page_offset && num_blk == 1) {
            filent_t* fe = filent_lookup(inode, start_blk);
            debug_assert(fe);
            debug_assert(start_blk >= fe->l_idx && start_blk <= fe->l_idx + fe->blocks);
            bytes = PAGE_SIZE - page_offset;
            if (bytes > count)
                bytes = count;

            pmem = (void *)REL2ABS(IDX2DATA(fe->p_idx + (start_blk - fe->l_idx)));
            logger_trace("%s: append small write %lu bytes at offset %lu, pmem: %p\n",
                 __func__, bytes, c, pmem + page_offset);
            avx_cpy(pmem + page_offset, buf, bytes);
            num_blk -= 1;
            sfence();
            pm_inode_t *pi = (pm_inode_t *)REL2ABS(IDX2INODE(inode->i_ino));
            pi->i_size = inode->i_size = max(inode->i_size, offset + len);
            flush_byte(&pi->i_size);
            
            goto write_next;
        }

        allocated = num_blk;
        nr = slot_data_alloc(inode, &allocated);
        if (unlikely(nr < 0)) {
            ret = -ENOSPC;
            debug_assert(0);      
            goto error;
        }
        pmem = (void *)REL2ABS(IDX2DATA(nr));
        bytes = PAGE_SIZE * allocated - page_offset;
        if (bytes > count)
            bytes = count;

        // copy the old data in the first block to the new allocated page
        if (page_offset) {
            filent_t* old = filent_lookup(inode, start_blk);
            if (likely(old)) {
                void *old_pmem = (void *)REL2ABS(IDX2DATA(
                        old->p_idx + (start_blk - old->l_idx)));
                avx_cpy(pmem, old_pmem, page_offset);
            }
        }
        // copy the old data in the last block to the new allocated page
        if ((page_offset + bytes) & PAGE_MASK) {
            filent_t* old = filent_lookup(inode, start_blk + allocated - 1);
            unsigned long end_offset = (offset + len) & PAGE_MASK;
            if (likely(old)) {
                void *old_pmem = (void *)REL2ABS(IDX2DATA(
                        old->p_idx + (start_blk + allocated - 1 - old->l_idx)));
                void *end_pmem = (void *)REL2ABS(IDX2DATA(nr + allocated - 1));
                avx_cpy(end_pmem + end_offset, old_pmem + end_offset, PAGE_SIZE - end_offset);
            }
        }

        // write the new data to the new allocated page
        avx_cpy(pmem + page_offset, buf, bytes);
        // syscall_trace("%s: new allocate write %lu bytes at offset %lu, pmem: %p\n", 
        //     __func__, bytes, c, pmem + page_offset);

        ebuf_add(ebuf, start_blk, nr, allocated);
        entry_num++;

        num_blk -= allocated;
write_next:
        c += bytes;
        buf += bytes;
        count -= bytes;
    }
    
    // inode_lock(inode->i_ino);
    time_t ts = slotfs_timestamp();
    int idx = 0;
    while(idx < entry_num) {
        entry_unit_t *unit = ebuf_get(ebuf, idx);
        pm_filent_t pf;
        pf.l_idx = unit->l_idx;
        pf.p_idx = unit->p_idx;
        pf.blocks = unit->blocks;
        pf.ts = ts;

        filent_t *fe = file_append_entry(inode, &pf);
        if (IS_ERR(fe)) {
            ret = PTR_ERR(fe);
            inode_unlock(inode->i_ino);
            goto error;
        }
        idx++;    
    }
    inode_unlock(inode->i_ino);
    ebuf_put(ebuf);

    inode->i_size = max(inode->i_size, offset + len);
    inode->i_ctim = inode->i_mtim = time(NULL);

    return len;
    
error:
    debug_assert(0);
    inode_unlock(inode->i_ino);
    // range_unlock(&inode->r_lock, _start_blk, end_blk);
    ebuf_put(ebuf);
    return ret;
}

ssize_t inode_write(inode_t *inode, const void *buf, size_t len, off_t offset) {
#ifdef INPLACE_WRITE
    return inode_write_inplace(inode, buf, len, offset);
#else
    return inode_write_atomic(inode, buf, len, offset);
#endif
    // return inode_write_fake(inode, buf, len, offset);
}

int inode_extend(inode_t *inode, size_t new_size) {
    filent_t *fe;
    unsigned long start_blk, num_blk, _start_blk, _end_blk;
    size_t page_offset;
    int ret;
    void *pmem;

    // printf("inode_extend: inode %lu, new_size %lu\n", inode->i_ino, new_size);
    page_offset = inode->i_size & PAGE_MASK;
    start_blk = (inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    _start_blk = start_blk;
    _end_blk = start_blk + num_blk - 1;
    num_blk = (new_size - inode->i_size + PAGE_SIZE - 1) / PAGE_SIZE;

    inode_lock(inode->i_ino);
    // range_lock(&inode->r_lock, _start_blk, _end_blk);
    ebuf_t *ebuf = ebuf_alloc();
    if (page_offset) {
        fe = filent_lookup(inode, start_blk);
        debug_assert(fe);

        pmem = (void *)REL2ABS(IDX2DATA(fe->p_idx + (start_blk - fe->l_idx)));
#if 0
        memset_nt(pmem + page_offset, 0, PAGE_SIZE - page_offset);
#endif
        start_blk++;
        num_blk--;
        page_offset = 0;
    }

    while (num_blk > 0) {
        unsigned long allocated = num_blk;
        unsigned long nr = slot_data_alloc(inode, &allocated);
        // printf("loop2\n");
        if (unlikely(nr < 0)) {
            return -ENOSPC;
        }
        
        pmem = (void *)REL2ABS(IDX2DATA(nr));
#if 0
        memset_nt(pmem, 0, PAGE_SIZE);
#endif
        ebuf_add(ebuf, start_blk, nr, allocated);
        num_blk -= allocated;
    } 

    int idx = 0;
    while (idx < ebuf->num) {
        entry_unit_t *unit = ebuf_get(ebuf, idx);
        pm_filent_t pf;
        pf.l_idx = unit->l_idx;
        pf.p_idx = unit->p_idx;
        pf.blocks = unit->blocks;
        pf.ts = inode->ts;   //todo: update the timestamp
        filent_t *fe = file_append_entry(inode, &pf);
        if (IS_ERR(fe)) {
            ret = PTR_ERR(fe);
            goto error;
        }
        idx++;
    }
    ebuf_put(ebuf);
    inode_unlock(inode->i_ino);
    // printf("return from inode_extend\n");
    // range_unlock(&inode->r_lock, _start_blk, _end_blk);
    return 0;
error:
    //todo: rollback
    ebuf_put(ebuf);
    inode_unlock(inode->i_ino);
    // range_unlock(&inode->r_lock, _start_blk, _end_blk);
    return ret;
}

int inode_shrink(inode_t *inode, size_t new_size) {
    filent_t *fe;
    unsigned long start_blk, end_blk, num_blk;

    start_blk = (new_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    end_blk = (inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

    relptr_t jnl = journal_alloc();
    journal_truncate(jnl, inode->i_ino, new_size);

    while (start_blk < end_blk) {
        fe = filent_lookup(inode, start_blk);
        if (!fe)
            continue;
        num_blk = fe->blocks;
        if (start_blk + num_blk > end_blk)
            break;

        file_remove_entry(inode, fe);
        start_blk += num_blk;
    }
    journal_commit(jnl);

    return 0;
}

int inode_resize(inode_t *inode, size_t new_size) {
    debug_assert(!is_dir(inode));

    if (unlikely(new_size == inode->i_size)) {
        return 0;
    }
    if (new_size < inode->i_size) {
        return inode_shrink(inode, new_size);
    } else {
        return inode_extend(inode, new_size);
    }
} 

void do_inode_release(inode_t *inode) {
    if (is_dir(inode)) {
        for (dirent_t *d; !list_empty(&inode->list); ) {
            d = list_entry(inode->list.next, dirent_t, list);
            list_del(&d->list);
            dirent_free(d);
            dir_remove_entry(inode, d);
        }
    } else {
        for (filent_t *f; !list_empty(&inode->list); ) {
            f = list_entry(inode->list.next, filent_t, list);
            slot_data_free(inode, f->p_idx, f->blocks);
            // pm_filent_t *pf = (pm_filent_t *)REL2ABS(IDX2FENT(f->idx));
            // pf->next = 0;
            // flush_byte(&pf->next);
            
            list_del(&f->list);
            filent_free(f);
            slot_firent_free(inode, f->idx);
        }
        btree_destroy(&inode->b_root);
    }

    slot_inode_free(inode, inode->i_ino);
    inode_free(inode);
}

void inode_release(inode_t *inode) {
    debug_assert(inode->ref == 0 && inode->i_link == 0);
    logger_trace("%s: inode %lu is released\n", __func__, inode->i_ino);
    spin_lock(&sbi->release_lock);
    list_add(&inode->release, &sbi->release_list);
    spin_unlock(&sbi->release_lock);
    // do_inode_release(inode);
}

void inode_flush(inode_t *inode) {
    pm_inode_t* pm_inode;
    pm_inode = (pm_inode_t *)REL2ABS(IDX2INODE(inode->i_ino));
    pm_inode->i_size = inode->i_size;
    pm_inode->i_uid = inode->i_uid;
    pm_inode->i_gid = inode->i_gid;
    pm_inode->i_mode = inode->i_mode;
    pm_inode->i_atim = inode->i_atim;
    pm_inode->i_mtim = inode->i_mtim;
    pm_inode->i_ctim = inode->i_ctim;
    pm_inode->log_head = inode->log_head;
    pm_inode->entry_num = inode->entry_num;
    pm_inode->csum = crc32c((uint32_t)0, (const void *)pm_inode, sizeof(pm_inode_t));
    flush_cache(pm_inode, sizeof(pm_inode_t), 1);
}