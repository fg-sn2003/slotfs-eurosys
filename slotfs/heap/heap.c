#include "heap.h"
#include "llist.h"
#include "../slotfs.h"
#include "../lock.h"

uint offset = 8;
static uint overhead = sizeof(footer_t) + sizeof(node_t);

void heap_init(heap_t *heap, void* region, size_t size) {
    node_t *init_region = (node_t *) region;
    init_region->hole = 1;
    init_region->size = size - sizeof(node_t) - sizeof(footer_t);
    spin_lock_init(&heap->lock);
    for (int i = 0; i < BIN_COUNT; i++) {
        heap->bins[i] = &heap->ebed_bins[i];
        memset(heap->bins[i], 0, sizeof(bin_t));
    }
    create_foot(init_region);

    add_node(heap->bins[get_bin_index(init_region->size)], init_region);

    heap->start = (long)region;
    heap->end   = (long)region + size;
}

void *do_heap_alloc(heap_t *heap, size_t size) {
    uint index = get_bin_index(size);
    bin_t *temp = (bin_t *) heap->bins[index];
    node_t *found = get_best_fit(temp, size);

    while (found == NULL) {
        if (index + 1 >= BIN_COUNT)
            return NULL;

        temp = heap->bins[++index];
        found = get_best_fit(temp, size);
    }

    if ((found->size - size) > (overhead + MIN_ALLOC_SZ)) {
        node_t *split = (node_t *) (((char *) found + sizeof(node_t) + sizeof(footer_t)) + size);
        split->size = found->size - size - sizeof(node_t) - sizeof(footer_t);
        split->hole = 1;
   
        create_foot(split);

        uint new_idx = get_bin_index(split->size);

        add_node(heap->bins[new_idx], split); 

        found->size = size; 
        create_foot(found); 
    }

    found->hole = 0; 
    remove_node(heap->bins[index], found); 
    
    node_t *wild = get_wilderness(heap);
    if (wild->size < MIN_WILDERNESS) {
        uint success = expand(heap, 0x1000);
        if (success == 0) {
            return NULL;
        }
    }
    else if (wild->size > MAX_WILDERNESS) {
        contract(heap, 0x1000);
    }

    found->prev = NULL;
    found->next = NULL;
    return &found->next; 
}

void do_heap_free(heap_t *heap, void *p) {
    bin_t *list;
    footer_t *new_foot, *old_foot;

    node_t *head = (node_t *) ((char *) p - offset);
    if (head == (node_t *) (uintptr_t) heap->start) {
        head->hole = 1; 
        add_node(heap->bins[get_bin_index(head->size)], head);
        return;
    }

    node_t *next = (node_t *) ((char *) get_foot(head) + sizeof(footer_t));
    footer_t *f = (footer_t *) ((char *) head - sizeof(footer_t));
    node_t *prev = f->header;

    if (prev->hole) {
        list = heap->bins[get_bin_index(prev->size)];
        remove_node(list, prev);

        prev->size += overhead + head->size;
        new_foot = get_foot(head);
        new_foot->header = prev;

        head = prev;
    }
    if (next->hole) {
        list = heap->bins[get_bin_index(next->size)];
        remove_node(list, next);

        head->size += overhead + next->size;

        old_foot = get_foot(next);
        old_foot->header = 0;
        next->size = 0;
        next->hole = 0;
        
        new_foot = get_foot(head);
        new_foot->header = head;
    }

    head->hole = 1;
    add_node(heap->bins[get_bin_index(head->size)], head);
}

uint expand(heap_t *heap, size_t sz) {
    return 0;
}

void contract(heap_t *heap, size_t sz) {
    return;
}

uint get_bin_index(size_t sz) {
    uint index = 0;
    sz = sz < 4 ? 4 : sz;

    while (sz >>= 1) index++; 
    index -= 2; 
    
    if (index > BIN_MAX_IDX) index = BIN_MAX_IDX; 
    return index;
}

void create_foot(node_t *head) {
    footer_t *foot = get_foot(head);
    foot->header = head;
}

footer_t *get_foot(node_t *node) {
    return (footer_t *) ((char *) node + sizeof(node_t) + node->size);
}

node_t *get_wilderness(heap_t *heap) {
    footer_t *wild_foot = (footer_t *) ((char *) heap->end - sizeof(footer_t));
    return wild_foot->header;
}

void *heap_alloc(heap_t *heap, size_t size) {
    spin_lock(&heap->lock);
    void *ret = do_heap_alloc(heap, size);
    spin_unlock(&heap->lock);
    return ret;
}

void heap_free(heap_t *heap, void *p) {
    spin_lock(&heap->lock);
    do_heap_free(heap, p);
    spin_unlock(&heap->lock);
}