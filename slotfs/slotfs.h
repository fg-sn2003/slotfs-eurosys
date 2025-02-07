#ifndef SLOTFS_H
#define SLOTFS_H

#include <stdbool.h>
#include <immintrin.h>
#include <assert.h>
#include <errno.h>
#include "config.h"
#include "slot.h"
#include "arena.h"
#include "heap/heap.h"
#include "lock.h"
#include "bitops.h"
#include "journal.h"

extern void memcpy_nt(char *dest, const char *src, int n);

int slotfs_init();
void slotfs_exit();
int dax_map(char *dax);
enum {
    SLOT_INODE = 0,
    SLOT_DIR,
    SLOT_FILE,
    SLOT_DATA,
    SLOT_TYPE_NUM
};

typedef struct inode_entry {
    inode_t *inode;
    spinlock_t lock;
} inode_entry_t;

typedef inode_entry_t* inode_table_t;

typedef struct pm_slot {
    relptr_t slot_table;    // slot table start
    size_t   slot_num;  
    size_t   slot_size;     // each slot size in bytes
    size_t   slot_pages;
    relptr_t bitmap;
    size_t   bitmap_pages;
} pm_slot_t;

typedef struct pm_super_block {
    uint64_t    magic;
    time_t      mtime;  // last mount time
    time_t      ctime;  // create time
    uint64_t    size;
	uint64_t    ts;
    
    pm_slot_t   slots[SLOT_TYPE_NUM];

    /* consistency */
    bool         umount;
    uint32_t     csm;
} pm_sb_t;

typedef struct dram_super_block {
    uint64_t    magic;
	int 		status;
	int			user;
    int         cpus;
    char        device[MAX_STR_LEN];
    char        mountpoint[MAX_STR_LEN];
    uint64_t    device_size;  
	
    pm_sb_t*    super;      // in-dram copy of pm super block
	inode_t*    root;
	int			initing;
	uint64_t 	ts;
	
	heap_t		heap_allocator;
	spinlock_t  heap_lock;
	arena_t	    btree_arena;
	arena_t	    inode_arena;
	arena_t	    entry_arena;
	ebuf_mgr_t*	ebuf_mgr;
	jnl_mgr_t*	jnl_mgr;

	list_head_t 	release_list;
	pthread_t  		release_thread;
	pthread_t 		gather_thread;
	spinlock_t		release_lock;
	inode_table_t   inode_table;
    dram_bitmap_t *maps[SLOT_TYPE_NUM];
} dram_sb_t;

extern dram_sb_t *sbi;

enum {
	STATUS_UNINIT = 0,
	STATUS_INITING,
	STATUS_READY,
	STATUS_EXIT,
};

static inline uint64_t slotfs_timestamp() {
	return ++sbi->ts;
}

int hook_dram_super_block();

static inline btree_node_t* btree_node_alloc() {
	return (btree_node_t *)arena_alloc(&sbi->btree_arena);
}

static inline void btree_node_free(btree_node_t *node) {
	arena_free(&sbi->btree_arena, node);
}

static inline inode_t *inode_alloc() {
	return (inode_t *)arena_alloc(&sbi->inode_arena);
}

static inline void inode_free(inode_t *inode) {
	arena_free(&sbi->inode_arena, inode);
}

static inline dirent_t *dirent_alloc() {
	return (dirent_t *)arena_alloc(&sbi->entry_arena);
}

static inline void dirent_free(dirent_t *dirent) {
	arena_free(&sbi->entry_arena, dirent);
}

static inline filent_t *filent_alloc() {
	return (filent_t *)arena_alloc(&sbi->entry_arena);
}

static inline void filent_free(filent_t *firent) {
	arena_free(&sbi->entry_arena, firent);
}


#define logger_trace(s, args ...)		 \
    ((void)((DBGMASK & DBGMASK_TRACE) ? printf(s, ##args) : 0))
#define logger_debug(s, args ...)		 \
    ((void)((DBGMASK & DBGMASK_DEBUG) ? printf(s, ##args) : 0))
#define logger_info(s, args ...)		 \
    ((void)((DBGMASK & DBGMASK_INFO) ? printf(s, ##args) : 0))
#define logger_warn(s, args ...)		 \
    ((void)((DBGMASK & DBGMASK_WARN) ? printf(s, ##args) : 0))
#define logger_fail(s, args ...)		 \
    ((void)((DBGMASK & DBGMASK_FAIL) ? printf(s, ##args) : 0))
#define syscall_trace(s, args ...)		 \
	((void)((DBGMASK & DBGMASK_SYSCALL) ? printf(s, ##args) : 0))

#define IDX2INODE(idx) (sbi->super->slots[SLOT_INODE].slot_table + (idx) * sbi->super->slots[SLOT_INODE].slot_size) 
#define IDX2DENT(idx) (sbi->super->slots[SLOT_DIR].slot_table + ((idx) * sbi->super->slots[SLOT_DIR].slot_size))
#define IDX2FENT(idx) (sbi->super->slots[SLOT_FILE].slot_table + ((idx) * sbi->super->slots[SLOT_FILE].slot_size))
#define IDX2DATA(idx)  (sbi->super->slots[SLOT_DATA].slot_table + ((idx) << PAGE_SHIFT))

#define _mm_clflush(addr)\
	asm volatile("clflush %0" : "+m" (*(volatile char *)(addr)))

#define _mm_clwb(addr)\
	asm volatile(".byte 0x66; xsaveopt %0" : "+m" \
		     (*(volatile char *)(addr)))


static inline void flush_cache(void *buf, uint32_t len, bool fence)
{
	uint32_t i;
	return;
	len = len + ((unsigned long)(buf) & (CACHELINE_SIZE - 1));
	if (SUPPORT_CLWB) {
		for (i = 0; i < len; i += CACHELINE_SIZE)
			_mm_clwb(buf + i);
	} else {
		for (i = 0; i < len; i += CACHELINE_SIZE)
			_mm_clflush(buf + i);
	}

	if (fence)
		sfence();
}

static inline void flush_byte(void *addr) 
{
    if (SUPPORT_CLWB) {
        _mm_clwb(addr);
    } else {
        _mm_clflush(addr);
    }
}

#define FLUSH_ALIGN (uint64_t)64
#define ALIGN_MASK	(FLUSH_ALIGN - 1)

static inline void memset_nt(void *dest, uint32_t dword, size_t length)
{
	uint64_t dummy1, dummy2;
	uint64_t qword = ((uint64_t)dword << 32) | dword;
		
	asm volatile ("movl %%edx,%%ecx\n"
		"andl $63,%%edx\n"
		"shrl $6,%%ecx\n"
		"jz 9f\n"
		"1:	 movnti %%rax,(%%rdi)\n"
		"2:	 movnti %%rax,1*8(%%rdi)\n"
		"3:	 movnti %%rax,2*8(%%rdi)\n"
		"4:	 movnti %%rax,3*8(%%rdi)\n"
		"5:	 movnti %%rax,4*8(%%rdi)\n"
		"8:	 movnti %%rax,5*8(%%rdi)\n"
		"7:	 movnti %%rax,6*8(%%rdi)\n"
		"8:	 movnti %%rax,7*8(%%rdi)\n"
		"leaq 64(%%rdi),%%rdi\n"
		"decl %%ecx\n"
		"jnz 1b\n"
		"9:	movl %%edx,%%ecx\n"
		"andl $7,%%edx\n"
		"shrl $3,%%ecx\n"
		"jz 11f\n"
		"10:	 movnti %%rax,(%%rdi)\n"
		"leaq 8(%%rdi),%%rdi\n"
		"decl %%ecx\n"
		"jnz 10b\n"
		"11:	 movl %%edx,%%ecx\n"
		"shrl $2,%%ecx\n"
		"jz 12f\n"
		"movnti %%eax,(%%rdi)\n"
		"12:\n"
		: "=D"(dummy1), "=d" (dummy2)
		: "D" (dest), "a" (qword), "d" (length)
		: "memory", "rcx");
}


static inline void avx_cpy(void *dest, const void *src, size_t size)
{
	/*
			* Copy the range in the forward direction.
			*
			* This is the most common, most optimized case, used unless
			* the overlap specifically prevents it.
			*/
	/* copy up to FLUSH_ALIGN boundary */

	
	size_t cnt = (uint64_t)dest & ALIGN_MASK;
	if (unlikely(cnt > 0))
	{
		cnt = FLUSH_ALIGN - cnt;
		if(cnt > size){
			cnt = size;
			size = 0;
		}
		else{
			size -= cnt;
		}
		/* never try to copy more the len bytes */
		// register uint32_t d;
		register uint8_t d8;
		// while(cnt > 3){
		// 	d = *(uint32_t*)(src);
		// 	_mm_stream_si32(dest, d);
		// 	src += 4;
		// 	dest += 4;
		// 	cnt -= 4;
		// }
		// if(unlikely(cnt > 0)){
		while(cnt){
			d8 = *(uint8_t*)(src);
			*(uint8_t*)dest = d8;
			cnt --;
			src ++;
			dest ++;
		}
		flush_byte(dest);
		// }
		if(size == 0){
			return;
		}
	}
	assert((uint64_t)dest % 64 == 0);
	register __m512i xmm0;
	while(size >= 64){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_stream_si512(dest, xmm0);
		dest += 64;
		src += 64;
		size -= 64;
	}
	
	/* copy the tail (<512 bit)  */
	size &= ALIGN_MASK;
	if (unlikely(size != 0))
	{
		while(size > 0){
			*(uint8_t*)dest = *(uint8_t*)src;
			size --;
			dest ++;
			src ++;
		}
		flush_byte(dest - 1);
	}
}

inline void avx_cpyt(void *dest, void *src, size_t size)
{
	/*
			* Copy the range in the forward direction.
			*
			* This is the most common, most optimized case, used unless
			* the overlap specifically prevents it.
			*/
	/* copy up to FLUSH_ALIGN boundary */
	
	register __m512i xmm0;
	while(size >= 512){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_storeu_si512(dest, xmm0);
		xmm0 = _mm512_loadu_si512(src + 64);
		_mm512_storeu_si512(dest + 64, xmm0);
		xmm0 = _mm512_loadu_si512(src + 128);
		_mm512_storeu_si512(dest + 128, xmm0);
		xmm0 = _mm512_loadu_si512(src + 192);
		_mm512_storeu_si512(dest + 192, xmm0);
		xmm0 = _mm512_loadu_si512(src + 256);
		_mm512_storeu_si512(dest + 256, xmm0);
		xmm0 = _mm512_loadu_si512(src + 320);
		_mm512_storeu_si512(dest + 320, xmm0);
		xmm0 = _mm512_loadu_si512(src + 384);
		_mm512_storeu_si512(dest + 384, xmm0);
		xmm0 = _mm512_loadu_si512(src + 448);
		_mm512_storeu_si512(dest + 448, xmm0);
		dest += 512;
		src += 512;
		size -= 512;
	}
	while(size >= 64){
		xmm0 = _mm512_loadu_si512(src);
		_mm512_storeu_si512(dest, xmm0);
		dest += 64;
		src += 64;
		size -= 64;
	}
	
	/* copy the tail  */
	size &= ALIGN_MASK;
	if (unlikely(size != 0))
	{
		while(size > 0){
			*(uint8_t*)dest = *(uint8_t*)src;
			size --;
			dest ++;
			src ++;
		}
	}
}

static inline uint32_t crc32c(uint32_t crc, const uint8_t *data, size_t len)
{
	return 0;
	uint8_t *ptr = (uint8_t *) data;
	uint64_t acc = crc; /* accumulator, crc32c value in lower 32b */
	uint32_t csum;

	/* x86 instruction crc32 is part of SSE-4.2 */
	// if (static_cpu_has(X86_FEATURE_XMM4_2)) {
	if (1) {
		/* This inline assembly implementation should be equivalent
		 * to the kernel's crc32c_intel_le_hw() function used by
		 * crc32c(), but this performs better on test machines.
		 */
		while (len > 8) {
			asm volatile(/* 64b quad words */
				"crc32q (%1), %0"
				: "=r" (acc)
				: "r"  (ptr), "0" (acc)
			);
			ptr += 8;
			len -= 8;
		}

		while (len > 0) {
			asm volatile(/* trailing bytes */
				"crc32b (%1), %0"
				: "=r" (acc)
				: "r"  (ptr), "0" (acc)
			);
			ptr++;
			len--;
		}

		csum = (uint32_t) acc;
	} else {
		/* The kernel's crc32c() function should also detect and use the
		 * crc32 instruction of SSE-4.2. But calling in to this function
		 * is about 3x to 5x slower than the inline assembly version on
		 * some test machines.
		 */
		assert(0);
		// csum = crc32c(crc, data, len);
	}

	return csum;
}

static inline size_t count_bits(const void* bitmap, size_t len)
{
    const uint8_t *ptr = (const uint8_t *)bitmap;
    size_t bit_count = 0;

	while (len > sizeof(uint64_t)) {
        bit_count += __builtin_popcountll(*(const uint64_t *)ptr);
        ptr += sizeof(uint64_t);
        len -= sizeof(uint64_t);
	}

    while (len >= sizeof(uint32_t)) {
        bit_count += __builtin_popcount(*(const uint32_t *)ptr);
        ptr += sizeof(uint32_t);
        len -= sizeof(uint32_t);
    }

	while (len > 0) {
        bit_count += __builtin_popcount(*ptr);
        ptr++;
        len--;
    }

    return bit_count;
}

static inline unsigned long BKDRHash(const char *str, int length)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned long hash = 0;
	int i;

	for (i = 0; i < length; i++)
		hash = hash * seed + (*str++);

	return hash;
}

static inline long dax_size_safe(const char *path)
{
	return DAX_SIZE;
    FILE *fp;
    unsigned long long size;
	char sys_path[256];
	const char *dax_name = strrchr(path, '/');
	
	if (dax_name == NULL) {
        fprintf(stderr, "Invalid DAX path: %s\n", path);
        return -1; 
    }

	dax_name++; 

	snprintf(sys_path, sizeof(sys_path), "/sys/class/dax/%s/size", dax_name);
    fp = fopen(sys_path, "r");
    if (fp == NULL) {
        perror("Failed to open /sys/class/dax/dax1.0/size");
        return EXIT_FAILURE;
    }

    if (fscanf(fp, "%llu", &size) != 1) {
        perror("Failed to read size");
        fclose(fp);
        return EXIT_FAILURE;
    }

    fclose(fp);
	return size;
}

static inline int file_exist_safe(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

#endif // SLOTFS_H
