#include "slotfs.h"

static void mkfs_journal(pm_sb_t *sb) {
    void *jnl = REL2ABS(JOURNAL_START);
    memset(jnl, 0, JOURNAL_SIZE);
    flush_cache(jnl, JOURNAL_SIZE, 0);
}

static void mkfs_slot(pm_sb_t *sb) {
    pm_slot_t *slots  = sb->slots;
    uint64_t remain_pages = (sb->size - BITMAP_START) >> PAGE_SHIFT;

    slots[SLOT_INODE].slot_size     = INODE_SLOT_SIZE;
    slots[SLOT_DIR].slot_size       = DIR_SLOT_SIZE;
    slots[SLOT_FILE].slot_size      = FILE_SLOT_SIZE;
    slots[SLOT_DATA].slot_size      = 4096;

    slots[SLOT_INODE].bitmap_pages  = INODE_BITMAP_PAGES;
    slots[SLOT_DIR].bitmap_pages    = DIR_BITMAP_PAGES;

    slots[SLOT_INODE].slot_num      = slots[SLOT_INODE].bitmap_pages << (PAGE_SHIFT + 3);
    slots[SLOT_DIR].slot_num        = slots[SLOT_DIR].bitmap_pages << (PAGE_SHIFT + 3);
    slots[SLOT_INODE].slot_pages    = (slots[SLOT_INODE].slot_size * slots[SLOT_INODE].slot_num + PAGE_SIZE - 1) >> PAGE_SHIFT;
    slots[SLOT_DIR].slot_pages      = (slots[SLOT_DIR].slot_size * slots[SLOT_DIR].slot_num + PAGE_SIZE - 1) >> PAGE_SHIFT;

    remain_pages -= slots[SLOT_INODE].bitmap_pages + slots[SLOT_DIR].bitmap_pages + slots[SLOT_INODE].slot_pages + slots[SLOT_DIR].slot_pages;

    // file entry slot num is 1.2 times of inode slot num
    slots[SLOT_FILE].slot_num       = ((remain_pages *PAGE_SIZE) / (PAGE_SIZE + slots[SLOT_FILE].slot_size)) * 6 / 5; 
    slots[SLOT_FILE].slot_pages     = (slots[SLOT_FILE].slot_size * slots[SLOT_FILE].slot_num + PAGE_SIZE - 1) >> PAGE_SHIFT;
    slots[SLOT_FILE].bitmap_pages   = (slots[SLOT_FILE].slot_num >> (PAGE_SHIFT + 3)) + 1;
        
    remain_pages -= slots[SLOT_FILE].bitmap_pages + slots[SLOT_FILE].slot_pages;

    slots[SLOT_DATA].bitmap_pages  = (remain_pages >> (PAGE_SHIFT + 3)) + 1;

    remain_pages -= slots[SLOT_DATA].bitmap_pages;

    slots[SLOT_DATA].slot_num = remain_pages;

    slots[SLOT_INODE].bitmap = BITMAP_START;
    slots[SLOT_DIR].bitmap   = slots[SLOT_INODE].bitmap + (slots[SLOT_INODE].bitmap_pages << PAGE_SHIFT);
    slots[SLOT_FILE].bitmap  = slots[SLOT_DIR].bitmap + (slots[SLOT_DIR].bitmap_pages << PAGE_SHIFT);
    slots[SLOT_DATA].bitmap  = slots[SLOT_FILE].bitmap + (slots[SLOT_FILE].bitmap_pages << PAGE_SHIFT);

    slots[SLOT_INODE].slot_table = slots[SLOT_DATA].bitmap + (slots[SLOT_DATA].bitmap_pages << PAGE_SHIFT);
    slots[SLOT_DIR].slot_table   = slots[SLOT_INODE].slot_table + (slots[SLOT_INODE].slot_pages << PAGE_SHIFT);
    slots[SLOT_FILE].slot_table  = slots[SLOT_DIR].slot_table + (slots[SLOT_DIR].slot_pages << PAGE_SHIFT);
    slots[SLOT_DATA].slot_table  = slots[SLOT_FILE].slot_table + (slots[SLOT_FILE].slot_pages << PAGE_SHIFT);

    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        memset_nt(REL2ABS(slots[i].bitmap), 0, slots[i].bitmap_pages << PAGE_SHIFT);
#ifndef ONE_ROUND
        if (i != SLOT_DATA) {
            memset(REL2ABS(slots[i].slot_table), 0, slots[i].slot_pages << PAGE_SHIFT);
            flush_cache(REL2ABS(slots[i].slot_table), slots[i].slot_pages << PAGE_SHIFT, 0);
        }
#endif 
    }

    // reserve the "0th" bit
    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        set_bit(0, REL2ABS(slots[i].bitmap));
        flush_cache(REL2ABS(slots[i].bitmap), 1, 0);
    }

    // reserve ghost slot of the root inode
    set_bit(1, REL2ABS(slots[SLOT_DIR].bitmap));
    flush_cache(REL2ABS(slots[SLOT_DIR].bitmap), 1, 1);    
    
    logger_info("File system size %lu MB\n", sb->size >> 20);
    logger_info("Inode: num: %lu, table size: %lu MB\n", slots[SLOT_INODE].slot_num, slots[SLOT_INODE].slot_size * slots[SLOT_INODE].slot_num >> 20);
    logger_info("Dir  : num: %lu, table size: %lu MB\n", slots[SLOT_DIR].slot_num, slots[SLOT_DIR].slot_size * slots[SLOT_DIR].slot_num >> 20);
    logger_info("File : num: %lu, table size: %lu MB\n", slots[SLOT_FILE].slot_num, slots[SLOT_FILE].slot_size * slots[SLOT_FILE].slot_num >> 20);
    logger_info("Data : num: %lu, table size: %lu MB\n", slots[SLOT_DATA].slot_num, slots[SLOT_DATA].slot_size * slots[SLOT_DATA].slot_num >> 20);
    logger_info("Metadata area: %lu MB\n", slots[SLOT_DATA].slot_table >> 20);
}

int mkfs_rootinode(pm_sb_t *sb) {
    pm_slot_t *slots = sb->slots;
    pm_inode_t *root = REL2ABS(sb->slots[SLOT_INODE].slot_table);
    
    // init root inode in slot table
    root->i_mode = S_IFDIR | 0755;
    root->i_uid = 0;
    root->i_gid = 0;
    root->i_size = 0;
    root->i_link = 1;
    root->i_atim = root->i_mtim = root->i_ctim = time(NULL);
    root->log_head = 1;
    root->entry_num = 0;
    root->ts = 0;
    root->csum = crc32c(0, (void *)root, sizeof(pm_inode_t));
    flush_cache(root, sizeof(pm_inode_t), 1);

    // alloc root inode in slot bitmap
    char* inode_bitmap = REL2ABS(slots[SLOT_INODE].bitmap);
    set_bit(0, inode_bitmap);
    flush_cache(inode_bitmap, 1, 1);

    return 0;
}

int mkfs(char* path) {
    pm_sb_t sb;
    struct stat st;

    logger_info("Make file system on %s\n", path);
    
    sb.magic    = SUPER_BLOCK_MAGIC;
    sb.size     = dax_size_safe(path);
    sb.ctime    = time(NULL);
    sb.mtime    = sb.ctime;
    sb.ts       = 0;
    sb.umount   = 1;

    mkfs_journal(&sb);
    mkfs_slot(&sb);
    mkfs_rootinode(&sb);
    sb.csm = crc32c(0, (void *)&sb, sizeof(pm_sb_t));

    memcpy(REL2ABS(0), &sb, sizeof(pm_sb_t));
    flush_cache(REL2ABS(0), sizeof(pm_sb_t), 1);
    return 0;
}