#define _GNU_SOURCE

#include "slotfs.h"
#include "shm.h" 
#include "runtime.h"
#include "btree.h"
#include "mkfs.h"
#include "recover.h"
#include "slotfs_func.h"
#include "gather.h" 
#include <syslog.h>

dram_sb_t* sbi;

int dax_map(char *dax) {
    struct stat st;
    int fd;
    void *dax_base;

    if ((fd = open(dax, O_RDWR, 0)) < 0) {
        perror("open dax");
        return -1;
    }

#ifdef SLOTFS_LOOPFILE
    if ((dax_base = mmap((void *)DAX_START, DAX_SIZE, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0)) == MAP_FAILED) {
        perror("mmap dax");
        assert(0);
        return -1;
    }
#else
    unsigned long size = dax_size_safe(dax);
    if ((dax_base = mmap((void *)DAX_START, size, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_SYNC | MAP_POPULATE, fd, 0)) != (void *)DAX_START) {
        perror("mmap dax");
        assert(0);
        return -1;
    }
#endif
    logger_info("Dax map %s to %p\n", dax, dax_base);

    close(fd);
    return 0;
}

void slotfs_exit() {
    sbi->user--;
    runtime_exit();
    if (sbi->user > 0)
        return;
        
    char *crash_env = getenv("SLOTFS_CRASH");
    int crash = 0;
    if (crash_env != NULL) 
        crash = atoi(crash_env);
    
    if (!crash) {
        pm_sb_t *pm = (pm_sb_t *)DAX_START;
        pm->umount = 1;
        flush_byte(&pm->umount);
    }

    if (sbi->user == 0) {
        sbi->status = STATUS_EXIT;
        pthread_join(sbi->release_thread, NULL);
    }
}

int need_mkfs() {
#if 1
    return 1;
#endif
    if (sbi->status != STATUS_UNINIT) {
        return 0;
    }

    char *mkfs_env = getenv("SLOTFS_MKFS");
    int mkfs = 0;

    if (mkfs_env != NULL) {
        mkfs = atoi(mkfs_env);
        setenv("SLOTFS_MKFS", "0", 1);
    }

    return mkfs;
}

int need_recovery() {
    if (sbi->status != STATUS_UNINIT) {
        return 0;
    }
    pm_sb_t *pm = (pm_sb_t *)DAX_START;
    return !pm->umount;
}

void release_thread() {
    while (1) {
        logger_trace("release thread running\n");
        if (sbi->status == STATUS_EXIT) {
            break;
        }
        spin_lock(&sbi->release_lock);
        if (list_empty(&sbi->release_list)) {
            spin_unlock(&sbi->release_lock);
            sleep(1);
            continue;
        }
        
        inode_t *inode = list_entry(sbi->release_list.next, inode_t, release);
        list_del(&inode->release);
        spin_unlock(&sbi->release_lock);
        do_inode_release(inode);
    }   
}

int init_dramon() {
    pthread_create(&sbi->release_thread, NULL, (void *)release_thread, NULL);
    // pthread_create(&sbi->gather_thread, NULL, (void *)gather_thread, NULL);
    return 0;
}

int slotfs_init() {
    int ret;
    int recovered = 0;

    ret = dax_map(DEVICE);
    assert(ret == 0);

    ret = shm_map(SLOTFS_SHM_NAME);
    assert(ret == 0);
    
    if (need_mkfs()) {
        ret = mkfs(DEVICE);
        assert(ret == 0);
    }

    if (need_recovery()) {
        recovered = 1;
        ret = recover();
        assert(ret == 0);
    }

    while (sbi->status == STATUS_INITING) {
        sleep(1);
    }

    if (sbi->status == STATUS_UNINIT) {
        sbi->status = STATUS_INITING;
        ret = shm_init(SLOTFS_SHM_NAME);
        assert(ret == 0);
        ret = init_dramon();
        assert(ret == 0);

        if (recovered) {
            ret = journal_replay();
            assert(ret == 0);
        }
        runtime_init();
        btree_module_init(btree_node_alloc, btree_node_free);
        sbi->status = STATUS_READY;   
    } else {
        runtime_init();
        btree_module_init(btree_node_alloc, btree_node_free);
    }

    sbi->user++;
    pm_sb_t *pm = (pm_sb_t *)DAX_START;
    pm->umount = 0;
    flush_byte(&pm->umount);
    return 0;
error:
    logger_fail("slotfs init error: %d\n", ret);
    return ret;
}