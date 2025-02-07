#include "inode.h"
#include "slotfs.h"
#include "slot.h"
#include "config.h"
#include "slot_vfs.h"
#include "runtime.h"
#include "debug.h"

bitmap_info_t get_bitmap_info() {
    bitmap_info_t info;
    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        dram_bitmap_t *bitmap = sbi->maps[i];
        info.remain[i] = 0;
        info.used[i] = 0;
        for (int j = 0; j < 16; j++) {
            info.remain[i] += bitmap[j].remain;
            info.used[i] += bitmap[j].bit_num - bitmap[j].remain;
        }
    }
    return info;
}

void print_bitmap_info()
{
    bitmap_info_t info = get_bitmap_info();
    printf("   type:   used     remain\n");
    printf("  inode : %ld      %ld\n", info.used[SLOT_INODE], info.remain[SLOT_INODE]);
    printf("  dirent: %ld      %ld\n", info.used[SLOT_DIR], info.remain[SLOT_DIR]);
    printf("  firent: %ld      %ld\n", info.used[SLOT_FILE], info.remain[SLOT_FILE]);
    printf("  data  : %ld      %ld\n", info.used[SLOT_DATA], info.remain[SLOT_DATA]);
    return;
}

static void print_inode(inode_t *inode) {
    printf("DRAM Inode Information:\n");
    printf("  Inode: %ld\n", inode->i_ino);
    printf("  Size: %lu\n", inode->i_size);
    printf("  Link: %ld\n", inode->i_link);
    printf("  Mode: %d\n", inode->i_mode);
    printf("  Entry: %ld\n", inode->entry_num);
    printf("  Ref: %d\n", inode->ref);
    printf("  Head: %lu\n", inode->log_head);
    printf("  Ghost: %lu\n", inode->ghost_slot);
}

void debug_pm_inode_ino(pm_inode_t *inode) {
    printf("PM Inode Information:\n");
    printf("  Size: %lu\n", inode->i_size);
    printf("  Link: %ld\n", inode->i_link);
    printf("  Mode: %d\n", inode->i_mode);
    printf("  Entry: %ld\n", inode->entry_num);
    printf("  Head: %lu\n", inode->log_head);
}

inode_t *debug_get_inode(int fd) {
    fd = fd - FD_OFFSET;
    printf("fd = %d\n", fd);
    printf("f_table = %p\n", rt.f_table);
    printf("rt = %p\n", &rt);
    if (unlikely(fd < 0 || fd >= FD_MAX)) 
        return NULL;
    if (unlikely(!rt.f_table[fd].valid)) 
        return NULL;
    printf("inode = %p\n", rt.f_table[fd].file);
    return rt.f_table[fd].file;
}

void debug_print_inode(int fd) {
    inode_t *inode;
    inode = debug_get_inode(fd);
    if (inode == NULL) 
        return;
    print_inode(inode);
}

pm_inode_t *debug_get_pm_inode(int fd) {
    inode_t *inode = debug_get_inode(fd);
    if (inode == NULL) 
        return NULL;

    pm_inode_t *pm_inode = (pm_inode_t *)REL2ABS(IDX2INODE(inode->i_ino));
    return pm_inode;
}

void debug_print_pm_inode(int fd) {
    pm_inode_t *inode;
    inode = debug_get_pm_inode(fd);
    if (inode == NULL) 
        return;
    debug_pm_inode_ino(inode);
}

void debug_dram_list(int fd) {
    inode_t *inode;
    inode = debug_get_inode(fd);
    if (inode == NULL) 
        return;

    printf("\n=== DRAM List for Inode %ld ===\n", inode->i_ino);
    list_head_t *pos;
    if (is_dir(inode)) {
        list_for_each(pos, &inode->list) {
            dirent_t *entry = list_entry(pos, dirent_t, list);
            if (entry->ref == 0) {
                printf("    idx = %ld, invalid, ts = %ld\n", entry->idx, entry->ts);
            } else {
                printf("    [Dirent] idx=%ld, ino=%ld, hash=%lu, ts=%ld, ref=%d\n",
                    entry->idx, entry->ino, entry->hash, entry->ts, entry->ref);
            }
        }
    } else {
        list_for_each(pos, &inode->list) {
            filent_t *entry = list_entry(pos, filent_t, list);
            if (entry->ref == 0) {
                printf("    idx = %ld, invalid, ts = %ld\n", entry->idx, entry->ts);
            } else {
                printf("    [Filent] idx=%ld, l_idx=%ld, p_idx=%ld, blocks=%zu, ts=%ld, ref=%d\n", 
                    entry->idx, entry->l_idx, entry->p_idx, 
                    entry->blocks, entry->ts, entry->ref);

            }
        }
    }
}

void debug_pm_list(int fd) {
    inode_t *inode;
    inode = debug_get_inode(fd);
    if (inode == NULL) 
        return;

    printf("\n=== PM List for Inode %ld ===\n", inode->i_ino);
    inode_unlock(inode->i_ino);

    index_t idx = inode->log_head;
    size_t count = 0;
    while (count < inode->entry_num) {
        if (is_dir(inode)) {
            pm_dirent_t *entry = (pm_dirent_t *)REL2ABS(IDX2DENT(idx));
            printf("    [Dirent] name_len=%d, name=%s, ino=%ld, ts=%ld, next=%ld\n",
                   entry->name_len, entry->name, entry->ino, entry->ts, entry->next);
            idx = entry->next;
        } else {
            pm_filent_t *entry = (pm_filent_t *)REL2ABS(IDX2FENT(idx));
            printf("    [Filent] l_idx=%d, p_idx=%u, blocks=%d, ts=%ld, next=%ld\n",
                   entry->l_idx, entry->p_idx, entry->blocks, entry->ts, entry->next);
            idx = entry->next;
        }
        count++;
    }
}