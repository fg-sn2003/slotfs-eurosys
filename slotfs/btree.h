// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

#define BRANCH_NUM (128)
#define MAX_LEVELS (10)

typedef struct btree_node {
    struct btree_node *parent;
    size_t height;
    size_t branch_index;
    size_t start;
    void *children[BRANCH_NUM];
} btree_node_t;

typedef struct btree {
    btree_node_t *root;
    size_t height;
} btree_t;

typedef struct btree_ptr {
    btree_node_t *node;
    size_t index;
    size_t key;
} btree_ptr;

void        btree_module_init(btree_node_t *(*malloc)(), void (*free)(btree_node_t *ptr));
void        btree_module_init_default();
btree_t*    btree_create(btree_t *tree);
void        btree_destroy(btree_t *tree);
int         btree_set_range(btree_t *tree, size_t start, size_t end, void *ptr);
int         btree_set_range_callback(btree_t *tree, size_t start, size_t end, 
    void *ptr, int (*callback)(void *, void *), void* ctx); 
int         btree_set_range_hint_callbak(btree_t *tree, size_t start, size_t end, void *ptr, 
    btree_ptr *hint, int (*callback)(void *, void *), void *ctx);
int         btree_set(btree_t *tree, size_t key, void *ptr);
void*       btree_get(btree_t *tree, size_t key);
void*       btree_get_hint(btree_t *tree, size_t key, btree_ptr *hint);
void**      btree_get_ptr_hint(btree_t *tree, size_t key, btree_ptr *hint);
int         btree_test();
#endif // BTREE_H