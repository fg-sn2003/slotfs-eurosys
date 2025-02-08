#include "slotfs.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "arena.h"
#include "lock.h"

static int arena_number = 0;

void arena_init(arena_t* allocator, void *mem, size_t size, size_t item_size) {
    allocator->mem  = mem;
    allocator->size = ALIGN(size, 8);
    allocator->item_size = ALIGN(item_size, 8);
    allocator->free_list = NULL;
    allocator->used = ATOMIC_VAR_INIT(0);
    allocator->type = arena_number++;
    spin_lock_init(&allocator->lock);
    
    // Initialize the free list
    for (size_t i = 0; i < size / item_size; i++) {
        void* item = (char*)mem + i * item_size;
        *(void**)item = allocator->free_list;
        allocator->free_list = item;
    }
}

void* arena_alloc(arena_t* allocator) {
    spin_lock(&allocator->lock);
    if (unlikely(allocator->free_list == NULL)) {
        spin_unlock(&allocator->lock);
        printf("type %d\n", allocator->type);
        debug_assert(0);
        return NULL;
    }
#ifdef SLOTFS_DEBUG
    atomic_fetch_add(&allocator->used, 1);
    if (allocator->used > (allocator->size / allocator->item_size)) {
        fprintf(stderr, "arena_alloc: allocator is full\n");
        abort();
    }
#endif
    void* item = allocator->free_list;
    allocator->free_list = *(void**)item;
    spin_unlock(&allocator->lock);
    return item;
}

void arena_free(arena_t* allocator, void* ptr) {
    spin_lock(&allocator->lock);
    *(void**)ptr = allocator->free_list;

#ifdef SLOTFS_DEBUG
    atomic_fetch_sub(&allocator->used, 1);
#endif
    allocator->free_list = ptr;
    spin_unlock(&allocator->lock);
}

