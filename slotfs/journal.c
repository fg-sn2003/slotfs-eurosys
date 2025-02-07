#include "journal.h"
#include "slotfs.h"
#include "inode.h"
#include "slotfs_func.h"
#include "slot_vfs.h"

void journal_rename(relptr_t jnl, index_t old_parent, index_t new_parent, 
        const char *old_name, int old_len, const char *new_name, int new_len)
{
    rename_jnl_t *rjnl = (rename_jnl_t*)REL2ABS(jnl);
    rjnl->valid = 1;
    rjnl->type = JNL_RENAME;
    rjnl->old_parent = old_parent;
    rjnl->new_parent = new_parent;
    memcpy(rjnl->old_name, old_name, old_len);
    rjnl->old_len = old_len;
    memcpy(rjnl->new_name, new_name, new_len);
    rjnl->new_len = new_len;
    flush_cache(rjnl, sizeof(rename_jnl_t), true);
}

void journal_write(relptr_t jnl, index_t ino, relptr_t begin, size_t len) {
    write_jnl_t *wjnl = (write_jnl_t*)REL2ABS(jnl);
    memcpy(wjnl + sizeof(write_jnl_t), REL2ABS(begin), len);
    wjnl->valid = 1;
    wjnl->type = JNL_WRITE;
    wjnl->ino = ino;
    wjnl->begin = begin;
    wjnl->len = len;
    flush_cache(wjnl, sizeof(write_jnl_t), true);
}

void journal_truncate(relptr_t jnl, index_t ino, size_t size) {
    truncate_jnl_t *tjnl = (truncate_jnl_t*)REL2ABS(jnl);
    tjnl->valid = 1;
    tjnl->type = JNL_TRUNCATE;
    tjnl->ino = ino;
    tjnl->size = size;
    flush_cache(tjnl, sizeof(truncate_jnl_t), true);
}

relptr_t journal_alloc() {
    jnl_mgr_t *mgr = sbi->jnl_mgr;
    spin_lock(&mgr->lock);
    if (likely(mgr->count == 0)) {
        mgr->use[0] = true;
        spin_unlock(&mgr->lock);
        return JOURNAL_START;
    }

    for (int i = 0; i < JOURNAL_NUM; i++) {
        if (!mgr->use[i]) {
            mgr->use[i] = true;
            mgr->count++;
            spin_unlock(&mgr->lock);
            return JOURNAL_START + i * PAGE_SIZE;
        }
    }

    spin_unlock(&mgr->lock);
    return 0;
}

void journal_free(relptr_t jnl) {
    spin_lock(&sbi->jnl_mgr->lock);
    index_t idx = (jnl - JOURNAL_START) / PAGE_SIZE;
    debug_assert(idx < JOURNAL_NUM);
    debug_assert(sbi->jnl_mgr->use[idx]);

    sbi->jnl_mgr->use[idx] = false;
    sbi->jnl_mgr->count--;
    spin_unlock(&sbi->jnl_mgr->lock);
}

void journal_commit(relptr_t jnl) {
    bool *valid = (bool*)REL2ABS(jnl);
    *valid = 0;
    flush_byte(valid);
    journal_free(jnl);
}

int journal_replay_write(write_jnl_t *jnl) {
    void *dest = REL2ABS(jnl->begin);
    void *src = jnl + sizeof(write_jnl_t);

    size_t len = jnl->len;
    memcpy(dest, src, len);
    flush_cache(dest, len, true);
    
    return 0;
}

void journal_mgr_init(jnl_mgr_t *mgr) {
    spin_lock_init(&mgr->lock);
    mgr->count = 0;
    for (int i = 0; i < JOURNAL_NUM; i++) {
        mgr->use[i] = false;
    }
}

int journal_replay_rename(rename_jnl_t *jnl) {
    inode_t *old_parent = iget(jnl->old_parent);
    inode_t *new_parent = iget(jnl->new_parent);


    unsigned long old_hash = BKDRHash(jnl->old_name, jnl->old_len);
    unsigned long new_hash = BKDRHash(jnl->new_name, jnl->new_len);

    dirent_t *old_dirent = dirent_lookup(old_parent, old_hash);
    dirent_t *new_dirent = dirent_lookup(new_parent, new_hash);

    if (!new_dirent) {  // crash before append
        if (!old_dirent) {
            assert(0);      //this should not happen
        }
        inode_t fake_inode;
        fake_inode.i_ino = old_dirent->ino;
        new_dirent = dir_append_entry(new_parent, &fake_inode, jnl->new_name, jnl->new_len);
        if (IS_ERR(new_dirent)) {
            return PTR_ERR(new_dirent);
        }
    }

    if (old_dirent) {   // crash before remove
        dir_remove_entry(old_parent, old_dirent);
    }

    return 0;
}

int journal_replay_truncate(truncate_jnl_t *jnl) {
    inode_t *inode = iget(jnl->ino);
    inode->i_size = jnl->size;
    return  inode_resize(inode, jnl->size);
}

int journal_replay() {
    int ret = 0;
    for (int i = 0; i < JOURNAL_NUM; i++) {
        relptr_t jnl = JOURNAL_START + i * PAGE_SIZE;
        journal_t *j = (journal_t*)REL2ABS(jnl);
        if (!j->valid) continue;

        switch (j->type) {
            case JNL_WRITE:
                ret = journal_replay_write((write_jnl_t *)jnl);
                break;
            case JNL_RENAME:
                ret = journal_replay_rename((rename_jnl_t *) jnl);
                break;
            case JNL_TRUNCATE:
                ret = journal_replay_truncate((truncate_jnl_t *) jnl);
                break;
            default:
                assert(0);
        }

        if (ret) {
            return ret;
        }
        j->valid = 0;
        flush_byte((void *)jnl);
    }
    return 0;
}
