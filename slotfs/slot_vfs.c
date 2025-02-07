#include "slotfs.h"
#include "slot_vfs.h"
#include "inode.h"

// return locked inode
inode_t *iget(unsigned long ino) {
    int ret;
    inode_t *inode;
    inode_lock(ino);
    if (sbi->inode_table[ino].inode) {
        inode = sbi->inode_table[ino].inode;
        // logger_trace("iget: ino %lu from cache\n", ino);
        return inode;
    } 
    
    inode = inode_alloc();
    if (!inode) {
        inode_unlock(ino);
        return ERR_PTR(-ENOMEM);
    }

    inode->i_ino = ino;

    ret = inode_rebuild(inode);

    if (ret) {
        inode_unlock(ino);
        inode_free(inode);
        return ERR_PTR(ret);
    }

    sbi->inode_table[ino].inode = inode;
    
    logger_trace("iget: ino %lu by rebuilding\n", ino);
    return inode;
}

void icache_install(inode_t *inode) {
    inode_lock(inode->i_ino);
    sbi->inode_table[inode->i_ino].inode = inode;
    inode_unlock(inode->i_ino);
}

// This function handles symbolic links. Only hold the target's inode lock to avoid deadlock.
// On success, it releases the inode lock and updates nd->current. On failure, the lock remains held.
int handle_symlink(inode_t *inode, nameidata_t *nd) {
    if (nd->flags & ND_NOFOLLOW) {
        return -ELOOP;
    }

    if (nd->depth > MAX_LOOP) {
        return -ELOOP;
    }  
    
    nameidata_t save_nd  = *nd;
    char *link = heap_alloc(&sbi->heap_allocator, inode->i_size + 1);
    if (!link) {
        return -ENOMEM;
    }

    ssize_t len = inode_readlink(inode, link);
    link[len] = '\0';
    nd->path = link;
    nd->depth++;

    if (nd->path[0] == '/') 
        nd->start = sbi->root;
    else
        nd->start = nd->current;

    inode_unlock(inode->i_ino);
    int ret = path_lookup(nd);

    if (ret == 0) {
        save_nd.current = nd->current;
    } else {
        inode_lock(inode->i_ino);
    }
    heap_free(&sbi->heap_allocator, link);
    *nd = save_nd;

    return ret;
}

int path_lookup(nameidata_t *nd) {
    inode_t *inode;
    const char *c = nd->path;;
    dirent_t *d;
    int ret;

    nd->current = nd->start;
    while(*c == '/')
        c++;
    if (unlikely(!c)) {
        assert(0);
    }

    inode_lock(nd->current->i_ino);
    while(1) {
        const char *this = c;
        int len;

        while (*c != '/' && *c != '\0') c++;
        len = c - this;

        while (*c == '/') {c++;}
        
        if (!*c)
            goto last_component;

        d = inode_lookup(nd->current, this, len);
        if (!d) {
            ret = -ENOENT;
            break;
        }

        inode = iget(d->ino);
        if (!inode) {
            ret = -ENOMEM;
            break;
        }

        if (IS_ERR(inode)) {
            ret = PTR_ERR(inode);
            break;
        } 
        
        if (is_symlink(inode)) {
            inode_unlock(nd->current->i_ino);
            ret = handle_symlink(inode, nd);
            if (ret) {
                inode_unlock(inode->i_ino);
                return ret;
            }
            continue;
        } 

        if (!is_dir(inode)) {
            inode_unlock(inode->i_ino);
            ret = -ENOTDIR;
            break;
        }

        inode_unlock(nd->current->i_ino);
        nd->current = inode;
        continue;

last_component:     // handle the last component specially
        nd->last = this;
        nd->last_len = len;
        if (nd->flags & ND_PARENT) {
            nd->parent = nd->current;
            nd->current = NULL;
            return 0;
        }

        d = inode_lookup(nd->current, this, len);
        
        if (!d) {
            ret = -ENOENT;
            break;
        }

        inode = iget(d->ino);
        if (!inode) {
            ret = -ENOMEM;
            break;
        }

        if (IS_ERR(inode)) {
            ret = PTR_ERR(inode);
            break;
        } 
        
        if (is_symlink(inode)) {
            inode_unlock(nd->current->i_ino);
            ret = handle_symlink(inode, nd);
            if (ret) {
                inode_unlock(inode->i_ino);
                return ret;
            }
            return 0;
        }
        inode_unlock(nd->current->i_ino);
        nd->current = inode;
        return 0;
    }

    inode_unlock(nd->current->i_ino);
    return ret;
}