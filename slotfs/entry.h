#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>
#include <time.h>
#include <linux/types.h>
#include <stdbool.h>
#include "list.h"

typedef struct {
    list_head_t list;
    index_t     idx;
    time_t      ts;
    ino_t       ino;
    uint64_t    hash;
    rb_node_t   rb_node;
    int         ref;
} dirent_t;

struct pm_dirent {
    uint8_t     name_len;
    char name[MAX_NAME_LEN];
    ino_t       ino;
    
    time_t      ts;
    index_t     next;
    uint32_t    csum;
}; 

typedef struct pm_dirent pm_dirent_t;

typedef struct filent {     //file entry
    list_head_t list;
    list_head_t invalid_list;
    index_t     idx;        // index in the slot tabl
    time_t      ts;
    
    index_t     l_idx;      // logical index
    index_t     p_idx;      // physical index
    size_t      blocks;     // number of blocks
    int         ref;
} filent_t;

// struct pm_filent {
//     uint8_t    l_idx[4];
//     uint8_t    p_idx[4];
//     uint8_t    blocks[2];

//     uint8_t    ts[8];
//     uint8_t    next[4];
//     uint32_t   csum;
// }__attribute((__packed__));

struct pm_filent {
    uint32_t   l_idx;
    uint32_t   p_idx;
    uint32_t   blocks;

    time_t     ts;
    index_t    next;
    uint32_t   csum;
};

typedef struct pm_filent pm_filent_t;

#endif // ENTRY_H