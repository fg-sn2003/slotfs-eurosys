#include "slotfs.h"
#include "config.h"
#include "entry.h"  
#include "gather.h"
#include "slot_vfs.h"
static int inode_gather_batch(inode_t *inode, const int batch_size, dirent_t **start_entry) {
    int count = 0, current_group = -1;
    dirent_t *batch_entries[batch_size];
    dirent_t *cur;

    cur = (*start_entry == NULL || *start_entry == &inode->list)
            ? list_first_entry(&inode->list, dirent_t, list)
            : *start_entry;

    for (; cur != &inode->list; cur = cur->list.next) {
        int this_group = cur->idx / batch_size;
        if (count == 0) {
            current_group = this_group;
        } else if (this_group != current_group) {
            break;
        }
        batch_entries[count++] = cur;
        if (count == batch_size) {
            cur = cur->list.next; 
            break;
        }
    }
    *start_entry = (cur != &inode->list) ? cur : &inode->list;

    if (count != batch_size)
        return 0;

    index_t new_base = do_slot_alloc_range(sbi->maps[SLOT_FILE], &count);
    if (count != (size_t)batch_size)
        return -ENOSPC;

    index_t old_idx[batch_size];
    for (int i = 0; i < batch_size; i++) {
        old_idx[i] = batch_entries[i]->idx;
        index_t new_idx = new_base + i;
        pm_filent_t *old_pm = (pm_filent_t *)REL2ABS(IDX2FENT(old_idx[i]));
        pm_filent_t *new_pm = (pm_filent_t *)REL2ABS(IDX2FENT(new_idx));
        memcpy(new_pm, old_pm, sizeof(pm_filent_t));
        new_pm->next = (i < batch_size - 1) ? (new_base + i + 1) : old_pm->next;
        new_pm->csum = crc32c(0, new_pm, sizeof(pm_filent_t));
        batch_entries[i]->idx = new_idx;
    }

    flush_cache(REL2ABS(IDX2FENT(new_base)), sizeof(pm_filent_t) * batch_size, 1);
    
    for (int i = 0; i < batch_size; i++) {
        slot_firent_free(inode, old_idx[i]);
    }
    
    if (batch_entries[0]->list.prev == &inode->list) {
        inode->log_head = new_base;
        pm_inode_t *pm_inode = (pm_inode_t *)REL2ABS(IDX2INODE(inode->i_ino));
        pm_inode->log_head = new_base;
        flush_byte(&pm_inode->log_head);
    } else {
        dirent_t *prev_entry = container_of(batch_entries[0]->list.prev, dirent_t, list);
        pm_filent_t *prev_pm = (pm_filent_t *)REL2ABS(IDX2FENT(prev_entry->idx));
        prev_pm->next = new_base;
        flush_byte(&prev_pm->next);
    }
    
    return batch_size;
}

int inode_gather(inode_t *inode) {
    const int BATCH_SIZE = 4;
    int total_gathered = 0;
    time_t idle_ts = sbi->ts;
    dirent_t *start_entry = &inode->list;

    while (1) {
        if (sbi->ts != idle_ts)
            break;
        int batch_result = inode_gather_batch(inode, BATCH_SIZE, &start_entry);
        if (batch_result <= 0)
            break;
        total_gathered += batch_result;
    }
    return total_gathered;
}

void gather_thread() {
    while (sbi->status != STATUS_EXIT) {
        time_t ts_before = sbi->ts;
        sleep(1);
        if (sbi->ts != ts_before) {
            continue; // not idle yet
        }

        int inode_num = sbi->super->slots[SLOT_INODE].slot_num;
        for (int i = 0; i < inode_num; i++) {
            inode_lock(i);
            inode_t *inode = sbi->inode_table[i].inode;
            if (inode) {
                inode_gather(inode);
            }
            inode_unlock(i);
        }
    }
    sleep(1);
}
