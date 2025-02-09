#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#define SUPPORT_CLWB 1
// #define SLOTFS_DEBUG
// #define SLOTFS_LOOPFILE

#ifdef SLOTFS_LOOPFILE
#define DEVICE "/tmp/dax"
#define DAX_SIZE    0x100000000                 
#else
#define DEVICE "/dev/dax1.0"
#define DAX_SIZE    (48UL * 1024 * 1024 * 1024)                 
#endif

#define SLOTFS_SHM_NAME "slotfs_shm"
#define SLOTFS_SHM_PATH "/dev/shm/"SLOTFS_SHM_NAME

#define APPEND_THRESHOLD ((float)1)

#define SUPER_BLOCK_MAGIC 0xdead404dbaddead

#define DBGMASK_NONE           	(0x00000000)
#define DBGMASK_ALL           	(0xffffffff)
#define DBGMASK_VERBOSE         (0x00000001)
#define DBGMASK_TRACE		    (0x00000002)
#define DBGMASK_DEBUG	        (0x00000004)
#define DBGMASK_INFO           	(0x00000008)
#define DBGMASK_WARN           	(0x00000010)
#define DBGMASK_FAIL           	(0x00000020)
#define DBGMASK_TEMP           	(0x00000040)
#define DBGMASK_SYSCALL         (0x00000080)
#define DBGMASK DBGMASK_NONE
// #define DBGMASK (DBGMASK_NONE)

#define MAX_STR_LEN     128     // < 4096 bytes
#define MAX_NAME_LEN    64      // < sizeof(pm_dirent_t)
#define MAX_LOOP        10      
#define FD_OFFSET	    1024
#define FD_MAX		    (4096 * 2)
#define MAP_NUM         1
/* shm layout */
// TODO: 
#define SHM_BASE    0x700000000000
#define SHM_SIZE    (2UL * 1024 * 1024 * 1024)					
#define HEAP_SIZE   (20UL * 1024 * 1024)
#define HEAP_START  (SHM_BASE + SHM_SIZE - HEAP_SIZE)
#define BNODE_ARENA (256UL * 1024 * 1024)
#define INODE_ARENA (256UL * 1024 * 1024)
#define ENTRY_ARENA (1024UL * 1024 * 1024)
#define DAX_START   0x780000000000

/* pm layout*/
#define INODE_SLOT_SIZE     128
#define DIR_SLOT_SIZE       256
#define FILE_SLOT_SIZE      64
#define INODE_BITMAP_PAGES  10
#define DIR_BITMAP_PAGES    10
#define FILE_BITMAP_PAGES   10

// ||sb||slotmeta||journal||bitmapmeta||bitmap||log area||data||
#define SUPER_BLOCK_SIZE    4096
#define SLOT_META_SIZE      4096
#define JOURNAL_NUM         256
#define JOURNAL_SIZE        (4096 * JOURNAL_NUM)
#define BITMAP_META_SIZE    4096
#define SLOT_META_START     SUPER_BLOCK_SIZE
#define JOURNAL_START       (SLOT_META_START + SLOT_META_SIZE)
#define BITMAP_META_START   (JOURNAL_START + JOURNAL_SIZE)
#define BITMAP_START        (BITMAP_META_START + BITMAP_META_SIZE)

typedef uint64_t relptr_t;   
typedef int64_t index_t;

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define PAGE_MASK   (((uint64_t)0x0fff))
#define PAGE_SHIFT  12
#define PAGE_SIZE   4096

#define PM_CACHELINE_SIZE           (256)
#define CACHELINE_SIZE				(64)
#define CACHELINE_MASK  (~(CACHELINE_SIZE - 1))
#define CACHELINE_ALIGN(addr) (((addr)+CACHELINE_SIZE-1) & CACHELINE_MASK)

#define ABS2REL(abs) ((relptr_t)((uint64_t)(abs) - DAX_START))
#define REL2ABS(rel) ((void *)((uint64_t)(rel) + DAX_START))

#define ROUND_UP(n, d)      (((n) + (d) - 1) / (d) * (d))
#define ROUND_DOWN(n, d)    ((n) / (d) * (d))
#define ALIGN(x, a)         (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))
#define max(x, y)           ((x) > (y) ? (x) : (y))
#define min(x, y)           ((x) < (y) ? (x) : (y))

#ifdef SLOTFS_DEBUG
#define debug_assert(expr) assert(expr)
#else
#define debug_assert(expr) ((void)0)
#endif

#define MAX_ERRNO 4095
#define ERR_PTR(err) ((void *)(long)(err))
#define PTR_ERR(ptr) ((long)(ptr))
#define IS_ERR_VALUE(x) unlikely((unsigned long)(x) >= (unsigned long)-MAX_ERRNO)
#define IS_ERR(ptr) IS_ERR_VALUE((unsigned long)(ptr))

#define	READDIR_END			(ULONG_MAX)
static inline void sfence(void) 
{	
    asm volatile ("sfence\n" : : );
}




#endif // CONFIG_H