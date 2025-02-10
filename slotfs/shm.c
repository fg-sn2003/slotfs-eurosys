#include "shm.h"
#include "config.h"
#include "slotfs.h"
#include "slot_vfs.h"

static void* shm_init_bitmap(void *base) {
    pm_slot_t *slots = sbi->super->slots;
    void *dram_start = base;
    int num = MAP_NUM;

    for (int i = 0; i < SLOT_TYPE_NUM; i++) {
        dram_bitmap_t *maps = sbi->maps[i];
        relptr_t pm_start   = slots[i].bitmap;
        size_t total_bits   = slots[i].slot_num;
        size_t bits_per_map = ROUND_UP(total_bits / num, 64);   // 64 bits per uin64_t
        size_t bit_offset   = 0;

        for (int j = 0; j < num; j++) {
            size_t bits = (j == num - 1) ? total_bits : bits_per_map;
            maps[j].pm = pm_start;
            maps[j].bit_offset = bit_offset;
            maps[j].bit_num = bits;
            maps[j].dram = dram_start;
            spin_lock_init(&maps[j].s_lock);

            size_t bytes = (bits + 7) / 8;
            memcpy(dram_start, REL2ABS(pm_start), bytes);

            int bit_count = count_bits(dram_start, bytes);
            maps[j].remain = bits - bit_count;
            maps[j].hint = next_zero_bit(dram_start, bits, 0);
            total_bits -= bits;
            bit_offset += bits;
            pm_start   += bytes;
            dram_start += bytes;
        }
        dram_start = (void *)ALIGN((uint64_t)dram_start, PAGE_SIZE);
    }
    logger_trace("DRAM bitmap init success\n");
    return dram_start;
}

int shm_init() {
    void *base = (void *)SHM_BASE;

    logger_info("Initing shm area\n");

    base += sizeof(dram_sb_t);
    sbi->super = (pm_sb_t *)base;
    memcpy(sbi->super, (void *)DAX_START, sizeof(pm_sb_t));

    sbi->device_size = sbi->super->size;
    sbi->ts = sbi->super->ts;
    sbi->user = 0;
    sbi->cpus = sysconf(_SC_NPROCESSORS_CONF);
    base = (void *)ALIGN((uint64_t)base, PAGE_SIZE);
    for(int i = 0; i < SLOT_TYPE_NUM; i++) {
        sbi->maps[i] = (dram_bitmap_t *)base;
        base += PAGE_SIZE;
    }

    INIT_LIST_HEAD(&sbi->release_list);
    spin_lock_init(&sbi->release_lock);

    base = shm_init_bitmap(base);

    base = (void *)ALIGN((uint64_t)base, PAGE_SIZE);

    sbi->inode_table = (inode_table_t)base;
    size_t inode_num = sbi->super->slots[SLOT_INODE].slot_num;
    for (int i = 0; i < inode_num; i++) {
        sbi->inode_table[i].inode = NULL;
        spin_lock_init(&sbi->inode_table[i].lock);
    }
    base += inode_num * sizeof(inode_entry_t);

    base = (void *)ALIGN((uint64_t)base, PAGE_SIZE);
    sbi->ebuf_mgr = (ebuf_mgr_t *)base;
    ebuf_mgr_init(sbi->ebuf_mgr);
    base += sizeof(ebuf_mgr_t);

    base = (void *)ALIGN((uint64_t)base, PAGE_SIZE);
    sbi->jnl_mgr = (jnl_mgr_t *)base;
    journal_mgr_init(sbi->jnl_mgr);
    base += sizeof(jnl_mgr_t);

    base = (void *)ALIGN((uint64_t)base, PAGE_SIZE);

    arena_init(&sbi->btree_arena, base, BNODE_ARENA, sizeof(btree_node_t));
    base += BNODE_ARENA;

    arena_init(&sbi->inode_arena, base, INODE_ARENA, sizeof(inode_t));
    base += INODE_ARENA;

    size_t entry_size = sizeof(dirent_t) > sizeof(filent_t) ? sizeof(dirent_t) : sizeof(filent_t);
    arena_init(&sbi->entry_arena, base, ENTRY_ARENA, entry_size);
    base += ENTRY_ARENA;

    if (base > (void *)HEAP_START) {
        logger_fail("shm init error: out of memory\n");
        return -1;
    }
    heap_init(&sbi->heap_allocator, (void *)HEAP_START, HEAP_SIZE);
    spin_lock_init(&sbi->heap_lock);
    logger_debug("new base %p, heap_base %p\n", base, (void *)HEAP_START);

    btree_module_init(btree_node_alloc, btree_node_free);
    sbi->magic = SUPER_BLOCK_MAGIC;
    sbi->root = iget(0);
    inode_unlock(0);

    return 0;
}

int shm_create(char *path) {
    int fd;
    int ret;
    
    fd = shm_open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return fd;
    }
    ret = ftruncate(fd, SHM_SIZE);
    if (ret < 0) {
        perror("ftruncate");
        close(fd);
        return ret;
    }

    close(fd);

    return 0;
}

int shm_map(char *name) {
    struct stat st;
    void *shm_base;
    char path[MAX_STR_LEN];
    int fd;
    int first = 0;
    int ret;
    
    snprintf(path, MAX_STR_LEN, "/dev/shm/%s", name);

    if (!file_exist_safe(path)) {
        shm_create(name);
        first = 1;
    }

    fd = shm_open(name, O_RDWR, 0);
    if (fd < 0) {
        assert(0);
        perror("shm_open");
        return fd;
    }
    ret = ftruncate(fd, SHM_SIZE);
    if (ret < 0) 
        return ret;

    shm_base = mmap((void *)SHM_BASE, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    if (shm_base != (void *)SHM_BASE) {
        logger_fail("shm base address error: %p\n", shm_base);
        return -1;
    }

    sbi = (dram_sb_t *)SHM_BASE;

    if (first) {
        memset(sbi, 0, sizeof(dram_sb_t));
        sbi->status = STATUS_UNINIT;
    }
    return 0;    
}