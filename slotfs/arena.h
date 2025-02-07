#ifndef _ARENA_H
#define _ARENA_H

#include <stddef.h>
#include <stdatomic.h>

typedef struct arena {
    void    *mem;
    size_t  size;
    size_t  item_size;
    void    *free_list;
    atomic_uint_fast64_t used;
    spinlock_t lock;
    int     type;
} arena_t;

void  arena_init(arena_t* allocator, void *mem, size_t size, size_t item_size);
void* arena_alloc(arena_t* allocator);
void  arena_free(arena_t* allocator, void* ptr);

#endif // _ARENA_H
