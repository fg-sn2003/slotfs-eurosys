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

void dax_map(char *dax) {
    int fd;
    void *dax_base;

    if ((fd = open(dax, O_RDWR, 0)) < 0) {
        perror("open dax");
        assert(0);
    }

#ifdef SLOTFS_LOCAL
    if ((dax_base = mmap((void *)DAX_START, DAX_SIZE, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0)) == MAP_FAILED) {
        perror("mmap dax");
        assert(0);
    }
#else
    unsigned long size = dax_size_safe(dax);
    if ((dax_base = mmap((void *)DAX_START, size, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_SYNC | MAP_POPULATE, fd, 0)) != (void *)DAX_START) {
        perror("mmap dax");
        return -1;
    }
#endif
    close(fd);
    
    logger_info("Map device %s to %p\n", dax, dax_base);

}

void shm_map(char *shm, int *first_instance) {
    void *base;
    int fd;

    fd = shm_open(SLOTFS_SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        fd = shm_open(SLOTFS_SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd != -1) {
            *first_instance = 1;
        } else if (errno == EEXIST) {   
            //race condition: another process create the shm
            fd = shm_open(SLOTFS_SHM_NAME, O_RDWR, 0666);
            if (fd == -1) {
                perror("shm_open");
                assert(0);
            }
        } else {
            perror("shm_open");
            assert(0);
        }
    }

    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        assert(0);
    }

    base = mmap((void *)SHM_BASE, SHM_SIZE, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_POPULATE, fd, 0);
    if (base != (void *)SHM_BASE) {
        logger_fail("shm base address error: %p\n", base);
        assert(0);
    }
}

void slotfs_exit() {
    runtime_exit();
    
    if (atomic_fetch_sub(&sbi->instance, 1) > 1)
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

    if (atomic_load(&sbi->instance) == 0) {
        sbi->status = STATUS_EXIT;
        pthread_join(sbi->release_thread, NULL);
    }
}

static int needs_mkfs() {
    char *env = getenv("SLOTFS_MKFS");

    if (env == NULL || atoi(env) != 1) {
        return false;
    }

#if 0
    printf("SLOTFS_MKFS is set. Do you want to proceed with mkfs? [Y/N]: ");
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);  
    ch = getchar();

    return (ch == 'Y' || ch == 'y');
#else
    return true;
#endif
}

int needs_recover() {
    pm_sb_t *pm = (pm_sb_t *)DAX_START;
    
    if (pm->umount == 1) {
        return false;
    }

#if 0
    printf("Detected an unclean shutdown. Do you want to proceed with recovery? [Y/N]: ");
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);  
    ch = getchar();

    if (ch == 'Y' || ch == 'y') {
        return true;
    } else {
        perror("corrupted file system");
        assert(0);
    }
#else
    return true;
#endif
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

int deamon_init() {
    pthread_create(&sbi->release_thread, NULL, (void *)release_thread, NULL);
    // pthread_create(&sbi->gather_thread, NULL, (void *)gather_thread, NULL);
    return 0;
}

int slotfs_init() {
    int recovered = 0;
    int first_instance = 0;
    int ret;

    dax_map(DEVICE);
    
    shm_map(SLOTFS_SHM_NAME, &first_instance);

    sbi = (dram_sb_t *)SHM_BASE;
    if (first_instance) {
        logger_info("%d: first instance\n", getpid());

        memset(sbi, 0, sizeof(dram_sb_t));
        
        atomic_store(&sbi->magic, SUPER_BLOCK_MAGIC);
        __sync_synchronize();
        
        atomic_store(&sbi->status, STATUS_INITING);
        __sync_synchronize();
        
        if (needs_mkfs()) {
            ret = slotfs_mkfs(DEVICE);
            assert(ret == 0);
        }
        
        if (needs_recover()) {
            ret = slotfs_recover();
            recovered = 1;
            assert(ret == 0);
        }

        ret = shm_init();
        assert(ret == 0);

        ret = deamon_init();   
        assert(ret == 0);

        atomic_store(&sbi->instance, 0);
        sbi->cpus = sysconf(_SC_NPROCESSORS_ONLN);

        pm_sb_t *pm = (pm_sb_t *)DAX_START;
        pm->umount = 0;
        flush_byte(&pm->umount);

        if (recovered) {
            ret = journal_replay();
            assert(ret == 0);
        }
        
        __sync_synchronize();
        atomic_store(&sbi->status, STATUS_READY);
    } else {
        logger_info("%d: not first instance\n", getpid());        
        do {
            __sync_synchronize();
        } while (atomic_load(&sbi->magic) != SUPER_BLOCK_MAGIC);
        
        while (atomic_load(&sbi->status) != STATUS_READY) {
            sleep(1);
        }
        __sync_synchronize();
    }

    runtime_init();

    btree_module_init(btree_node_alloc, btree_node_free);

    atomic_fetch_add(&sbi->instance, 1);

    return 0;
}