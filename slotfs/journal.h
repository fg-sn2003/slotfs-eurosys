#ifndef JOURNAL_H
#define JOURNAL_H

#include "config.h"
#include "lock.h"
#include "config.h"

enum {
    JNL_WRITE = 1,
    JNL_RENAME,
    JNL_TRUNCATE,
    JNL_TYPE_NUM,
};

// UNDO
typedef struct write_journal {
    bool        valid;
    int         type;
    index_t     ino;
    relptr_t    begin;
    size_t      len;
    time_t      ts;
} write_jnl_t;

// REDO
typedef struct rename_journal {
    bool    valid;
    int    type;
    index_t old_parent;
    index_t new_parent;
    char   old_name[MAX_NAME_LEN];
    int    old_len;
    char   new_name[MAX_NAME_LEN];
    int    new_len;  

    time_t  ts;
} rename_jnl_t;

// REDO 
typedef struct truncate_journal {
    bool     valid;
    bool     type;
    index_t  ino;
    size_t   size;
} truncate_jnl_t;

typedef struct journal_manager {
    spinlock_t lock;
    bool use[JOURNAL_NUM];
    int count;
} jnl_mgr_t;

typedef struct journal {
    bool valid;
    int  type;
} journal_t;

relptr_t journal_alloc();
void journal_mgr_init(jnl_mgr_t *mgr);
void journal_free(relptr_t jnl);
void journal_commit(relptr_t jnl);
void journal_rename(relptr_t jnl, index_t old_parent, index_t new_parent, 
        const char *old_name, int old_len, const char *new_name, int new_len);
void journal_write(relptr_t jnl, index_t ino, relptr_t begin, size_t len);
void journal_truncate(relptr_t jnl, index_t ino, size_t size);
int  journal_replay();
#endif // JOURNAL_H