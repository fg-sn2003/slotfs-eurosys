// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "btree.h"
#include "heap/heap.h"

static size_t max_keys[MAX_LEVELS];

static btree_node_t *(*btree_malloc)() = NULL;
static void (*btree_free)(btree_node_t *ptr) = NULL;

static btree_node_t *btree_node_create(btree_t *tree, btree_node_t *parent, size_t branch) {
    btree_node_t *node = btree_malloc();
    if (!node) return NULL;

    memset(node, 0, sizeof(btree_node_t));
    node->parent = parent;
    node->height = parent ? parent->height - 1 : 0;
    if (parent) {
        node->branch_index = branch;
        parent->children[branch] = node;
        node->start = parent->start + branch * max_keys[node->height - 1];
    }

    return node;
}

btree_t *btree_create(btree_t* tree) {
    if (!tree) return NULL;

    tree->height = 0;

    tree->root = btree_node_create(tree, NULL, 0);
    if (!tree->root) {
        return NULL;
    }

    return tree;
}

int btree_next(btree_t *tree, btree_ptr* iter, bool create) {
    btree_node_t *node = iter->node;
    size_t index = iter->index;

    if (index + 1 < BRANCH_NUM) {
        if (!create && !node->children[index + 1]) {
            iter->node = NULL;
            return 0;
        } else {
            iter->node = node;
            iter->index = index + 1;
            return 0;
        }
    }

    size_t parent_index = node->branch_index;
    while (node->parent) {
        parent_index = node->branch_index;
        node = node->parent;
        if (parent_index + 1 < BRANCH_NUM) {
            break;
        }
    }
    
    if (node->parent == NULL && parent_index + 1 == BRANCH_NUM) {
        iter->node = NULL;
        return 0;
    }

    size_t branch = parent_index + 1;
    while (node->height > 0) {
        if (!node->children[branch]) {
            if (!create) {
                iter->node = NULL;
                return 0;
            }
            btree_node_t *new_node = btree_node_create(tree, node, branch);
            if (!new_node) {
                iter->node = NULL;
                return -ENOMEM;
            }
        }
        node = (btree_node_t *)node->children[branch];
        branch = 0;     // go to the leftmost child
    }

    iter->node = node;
    iter->index = 0;
    return 0;
}

static int btree_pos(btree_t *tree, size_t key, btree_ptr* pos, bool create) {
    btree_node_t *node = tree->root;

    while (node->height > 0) {
        size_t branch = key / max_keys[node->height - 1];
        key %= max_keys[node->height - 1];
        if (!node->children[branch]) {
            if (!create) {
                pos->node = NULL;
                return 0;
            }
            btree_node_t *new_node = btree_node_create(tree, node, branch);
            if (!new_node) {
                return -ENOMEM;
            }
        }
        node = (btree_node_t *)node->children[branch];
    }

    pos->node = node;
    pos->index = key;
    return 0;
}

static void btree_destroy_node(btree_t *tree, btree_node_t *node) {
    if (node->height == 0) {
        btree_free(node);
        return;
    }

    for (size_t i = 0; i < BRANCH_NUM; i++) {
        if (node->children[i]) {
            btree_destroy_node(tree, (btree_node_t *)node->children[i]);
        }
    }

    btree_free(node);
}

void btree_destroy(btree_t *tree) {
    if (!tree) return;

    btree_destroy_node(tree, tree->root);
}

static void btree_expand(btree_t *tree) {
    btree_node_t *new_root = btree_node_create(tree, NULL, 0);
    new_root->children[0] = tree->root;
    tree->root->parent = new_root;
    tree->root = new_root;
    tree->height++;

    new_root->height = tree->height;
    new_root->branch_index = 0;
    new_root->start = 0;
}

int btree_set(btree_t *tree, size_t key, void *ptr) {
    if (!tree) return -1;

    while (key >= max_keys[tree->height]) {
        btree_expand(tree);
    }

    btree_ptr pos;
    int ret = btree_pos(tree, key, &pos, true);
    if (ret) {
        return ret;
    }

    pos.node->children[pos.index] = ptr;
    return 0;
}

int btree_set_range_hint_callbak(btree_t *tree, size_t start, size_t end, void *ptr, 
    btree_ptr *hint, int (*callback)(void *, void *), void *ctx) {
    if (!tree || start >= end) return -1;

    int ret;
    while (end >= max_keys[tree->height]) {
        btree_expand(tree);
    }

    void **ret_ptr;
    ret_ptr = btree_get_ptr_hint(tree, start, hint);
    if (!ret_ptr) return -1;

    btree_ptr pos = *hint;
    do {
        void *old_ptr = pos.node->children[pos.index];
        if (old_ptr && callback && callback(old_ptr, ctx)) {
            return -1;
        }
        pos.node->children[pos.index] = ptr;
        start++;
        *hint = pos;
        ret = btree_next(tree, &pos, true);
        if (ret) return ret;
    } while (start < end);

    return 0;
}

int btree_set_range_callback(btree_t *tree, size_t start, size_t end, 
    void *ptr, int (*callback)(void *, int, void *), void* ctx) {
    if (!tree || start >= end) return -1;

    while (end >= max_keys[tree->height]) {
        btree_expand(tree);
    }

    btree_ptr pos;
    int ret = btree_pos(tree, start, &pos, true);
    if (ret) return ret;

    do {
        void *old_ptr = pos.node->children[pos.index];
        if (old_ptr && callback && callback(old_ptr, start, ctx)) {
            return -1;
        }
        pos.node->children[pos.index] = ptr;
        start++;
        ret = btree_next(tree, &pos, true);
        if (ret) return ret;
    } while (start < end);

    return 0;
}

int btree_set_range(btree_t *tree, size_t start, size_t end, void *ptr) {
    return btree_set_range_callback(tree, start, end, ptr, NULL, NULL);
}

void *btree_get(btree_t *tree, size_t key) {
    if (!tree) return NULL;

    btree_node_t *node = tree->root;

    while (node->height > 0) {
        size_t branch = key / max_keys[node->height - 1];
        key %= max_keys[node->height - 1];
        if (!node->children[branch]) {
            return NULL;
        }
        node = (btree_node_t *)node->children[branch];
    }
    
    return node->children[key];
}

void **btree_get_ptr_hint(btree_t *tree, size_t key, btree_ptr *hint) {
    btree_ptr pos;
    int ret;
    if (!tree || !hint) return NULL;

    if (!hint->node) {
        ret = btree_pos(tree, key, &pos, true);
        if (ret) return NULL;
        if (!pos.node) return NULL;
        hint->node = pos.node;
        hint->index = pos.index;
        hint->key = key;
        return &pos.node->children[pos.index];
    }

    if (hint->node && hint->key == key) {
        return &hint->node->children[hint->index];
    }

    if (hint->node && key == hint->key + 1) {
        pos.index = hint->index;
        pos.node = hint->node;
        ret = btree_next(tree, &pos, true);
        if (ret) return NULL;
        if (!pos.node) return NULL;

        hint->node = pos.node;
        hint->index = pos.index;
        hint->key = key;
        return &pos.node->children[pos.index];
    }

    ret = btree_pos(tree, key, &pos, true);
    if (ret) return NULL;
    if (!pos.node) return NULL;
    hint->node = pos.node;
    hint->index = pos.index;
    hint->key = key;
    return &pos.node->children[pos.index];
}

void *btree_get_hint(btree_t *tree, size_t key, btree_ptr *hint) {
    void **ptr = btree_get_ptr_hint(tree, key, hint);
    if (!ptr) return NULL;
    return *ptr;
}

static void init_max_keys() {
    size_t max_key = BRANCH_NUM;
    for (int i = 0; i < MAX_LEVELS; i++) {
        max_keys[i] = max_key;
        max_key *= BRANCH_NUM;
    }
}

static btree_node_t *libc_btree_malloc() {
    return (btree_node_t* )malloc(sizeof(btree_node_t));
}

static void libc_btree_free(btree_node_t *ptr) {
    free(ptr);
}

void btree_module_init_default() {
    btree_module_init(libc_btree_malloc, libc_btree_free);
}

void btree_module_init(btree_node_t *(*malloc)(), void (*free)(btree_node_t *ptr)) {
    btree_malloc = malloc;
    btree_free = free;
    init_max_keys();
}
