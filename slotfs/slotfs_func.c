#define _GNU_SOURCE
#include "slotfs.h"
#include "slot_vfs.h"
#include "runtime.h"
#include "slotfs_func.h"
#include <sys/stat.h>
#include <dirent.h>

// todo: lock
static inode_t* file_table_get(int fd) {
    if (unlikely(fd < 0 || fd >= FD_MAX))
        return ERR_PTR(-EBADFD);
    
    if (unlikely(!rt.f_table[fd].valid)) {
        return ERR_PTR(-EBADFD);
    } else {
        return rt.f_table[fd].file;
    }
}

static void nameidata_init(nameidata_t *nd, int fd, const char *path, int flags) {
    nd->flags = 0;
    if (*path == '/') {
        nd->start = sbi->root;
    } else if (fd == AT_FDCWD) {
        nd->start = rt.cwd;
    } else {
        nd->start = file_table_get(fd);    
    }
    if (flags & O_NOFOLLOW) {
        nd->flags |= ND_NOFOLLOW;
    }
    nd->path = path;
    nd->depth = 0;
}

static inode_t* do_slotfs_open(int dfd, const char *path, int flags, mode_t mode) {
    nameidata_t nd;
    int ret;

    nameidata_init(&nd, dfd, path, flags);
    if (unlikely(IS_ERR(nd.start))) {
        return nd.start;
    }

    if (!(flags & O_CREAT)) {
        ret = path_lookup(&nd);
        if (unlikely(ret)) {
            return ERR_PTR(ret);
        }

        if (flags & O_DIRECTORY) {
            if (!is_dir(nd.current)) {
                inode_unlock(nd.current->i_ino);
                return ERR_PTR(-ENOTDIR);
            }
        }

        nd.current->ref++;
        inode_unlock(nd.current->i_ino);
        return nd.current;      
    }

    nd.flags |= ND_PARENT;
    ret = path_lookup(&nd);
    if (ret)
        return ERR_PTR(ret);

    inode_t *parent = nd.parent;

    dirent_t *d = inode_lookup(parent, nd.last, nd.last_len);
    inode_t *inode;
    if (unlikely(d)) {
        inode = iget(d->ino);
        debug_assert(inode);
        inode_unlock(inode->i_ino);
        inode_unlock(parent->i_ino);
        return inode;
    }

    inode = inode_create(parent, nd.last, nd.last_len, mode);
    inode->ref++;
    icache_install(inode);
    inode_unlock(parent->i_ino);

    return inode;
}

int slotfs_open(int dfd, const char *path, int flags, mode_t mode) {
    f_header_t *fh;
    inode_t *inode;

    fh = fd_alloc();
    if (!fh) {
        set_errno(-EMFILE);
        return -1;
    }

    inode = do_slotfs_open(dfd, path, flags, mode);
    if (IS_ERR(inode)) {
        set_errno(PTR_ERR(inode));
        return -1;
    }

    fh->file = inode;
    fh->flags = flags;

    return fh->fd;
}

int slotfs_link(const char *target, const char *link) {
    inode_t *parent, *target_inode;
    nameidata_t nd;
    int ret = 0;
    dirent_t *d;

    nameidata_init(&nd, AT_FDCWD, target, 0);
    if (unlikely(IS_ERR(nd.start))) {
        ret = PTR_ERR(nd.start);
        goto out;
    }
    ret = path_lookup(&nd);
    if (ret)
        goto out;

    target_inode = nd.current;
    if (unlikely(is_dir(target_inode))) {
        ret = -EPERM;
        goto out_inode;
    }

    nameidata_init(&nd, AT_FDCWD, link, 0);
    if (unlikely(IS_ERR(nd.start))) {
        ret = PTR_ERR(nd.start);
        goto out_inode;
    }

    nd.flags |= ND_PARENT;
    ret = path_lookup(&nd);
    if (unlikely(ret))
        goto out_inode;

    parent = nd.parent;
    d = inode_lookup(parent, nd.last, nd.last_len);
    if (unlikely(d)) {
        ret = -EEXIST;
        goto out_parent;
    }

    d = dir_append_entry(parent, target_inode, nd.last, nd.last_len);
    if (unlikely(IS_ERR(d))) {
        ret = PTR_ERR(d);
        goto out_parent;
    }

out_parent:
    inode_unlock(parent->i_ino);
out_inode:
    inode_unlock(target_inode->i_ino);
out:
    return ret;
}

int slotfs_unlink(const char *path) {
    inode_t *parent;
    nameidata_t nd;
    int ret = 0;
    dirent_t *d;

    nameidata_init(&nd, AT_FDCWD, path, 0);
    if (unlikely(IS_ERR(nd.start))) {
        ret = PTR_ERR(nd.start);
        goto out;
    }

    nd.flags |= ND_PARENT;
    ret = path_lookup(&nd);
    if (unlikely(ret))
        goto out;

    parent = nd.parent;
    d = inode_lookup(parent, nd.last, nd.last_len);
    if (unlikely(!d)) {
        ret = -ENOENT;
        goto out_parent;
    }

    inode_t* inode = iget(d->ino);
    if (unlikely(!inode)) {
        ret = -ENOENT;
        goto out_parent;
    }

    if (unlikely(is_dir(inode))) {
        ret = -EISDIR;
        goto out_inode;
    }

    ret = inode_remove(parent, d, inode);
    if (unlikely(ret)) {
        goto out_inode;
    }

    if (inode->ref == 0 && inode->i_link == 0) {
        index_t ino = inode->i_ino;
        inode_release(inode);
        inode_unlock(ino);
    } else {
        inode_unlock(inode->i_ino);
    }

    inode_unlock(parent->i_ino);
    return 0;
out_inode:
    inode_unlock(inode->i_ino);
    dirent_free(d);
out_parent:
    inode_unlock(parent->i_ino);
out:
    return ret;
}

int slotfs_linkat(int olddfd, const char *oldpath, int newdfd, const char *newpath, int flags) {
    return 0;
}

int slotfs_unlinkat(int dfd, const char *path, int flags) {
    return 0;
}

int slotfs_symlink(const char *target, const char *link) {
    inode_t *parent;
    nameidata_t nd;
    int ret = 0;

    nameidata_init(&nd, AT_FDCWD, link, 0);
    if (unlikely(IS_ERR(nd.start))) {
        ret = PTR_ERR(nd.start);
        goto out;
    }

    nd.flags |= ND_PARENT;
    ret = path_lookup(&nd);
    if (unlikely(ret))
        goto out;

    parent = nd.parent;
    dirent_t *d = inode_lookup(parent, nd.last, nd.last_len);
    if (unlikely(d)) {
        ret = -EEXIST;
        goto out;
    }

    //1: create the link inode
    inode_t *inode = inode_create(parent, nd.last, nd.last_len, S_IFLNK);
    if (IS_ERR(inode)) {
        ret = PTR_ERR(inode);
        goto out_parent;
    }

    //2: write the link to inode
    int len = strlen(target);
    ret = inode_write(inode, target, len, 0);
    if (ret != len) {
        ret = -EIO;
        goto out_inode;
    }

    icache_install(inode);
    inode_unlock(inode->i_ino);
    inode_unlock(parent->i_ino);
    return 0;

out_inode:
    inode_unlock(inode->i_ino);
    slot_inode_free(parent, inode->i_ino);
    inode_free(inode);
out_parent:
    inode_unlock(parent->i_ino);
out:
    return ret;
}

int slotfs_symlinkat(const char *from, int dfd, const char *to) {
    return 0;
}

// Overwriting entries is not supported. If you need to 
// overwrite an entry, just unlink it first.
int slotfs_rename(const char *oldpath, const char *newpath) {
    inode_t* old_dir, *new_dir;
    dirent_t *old_dentry, *new_dentry, *d;
    nameidata_t old_nd, new_nd;
    int same_dir = 0;

    nameidata_init(&old_nd, AT_FDCWD, oldpath, 0);
    if (unlikely(IS_ERR(old_nd.start))) {
        return PTR_ERR(old_nd.start);
    }

    old_nd.flags |= ND_PARENT;
    int ret = path_lookup(&old_nd);
    if (unlikely(ret)) {
        return ret;
    }
    old_dir = old_nd.parent;
    inode_unlock(old_dir->i_ino);

    nameidata_init(&new_nd, AT_FDCWD, newpath, 0);
    if (unlikely(IS_ERR(new_nd.start))) {
        return PTR_ERR(new_nd.start);
    }

    new_nd.flags |= ND_PARENT;
    ret = path_lookup(&new_nd);
    if (unlikely(ret)) {
        return ret;
    }
    new_dir = new_nd.parent;

    if (new_dir == old_dir) {
        same_dir = 1;
        if (strcmp(old_nd.last, new_nd.last) == 0) {
            inode_unlock(new_dir->i_ino);
            return -EINVAL;
        }
    } else {
        same_dir = 0;
        inode_lock(old_dir->i_ino);
    }

    old_dentry = inode_lookup(old_dir, old_nd.last, old_nd.last_len);
    if (unlikely(!old_dentry)) {
        ret = -ENOENT;
        goto out;
    }

    new_dentry = inode_lookup(new_dir, new_nd.last, new_nd.last_len);
    if (new_dentry) {
        ret = -EEXIST;
        goto out;
    }

    inode_t fake_inode;
    fake_inode.i_ino = old_dentry->ino;
    
    relptr_t jnl = journal_alloc();
    journal_rename(jnl, old_dir->i_ino, new_dir->i_ino, 
        old_nd.last, old_nd.last_len, new_nd.last, new_nd.last_len);

    d = dir_append_entry(new_dir, &fake_inode, new_nd.last, new_nd.last_len);
    if (unlikely(IS_ERR(d))) {
        goto out;
    }
    dir_remove_entry(old_dir, old_dentry);
    
    journal_commit(jnl);
out:
    inode_unlock(new_dir->i_ino);
    if (!same_dir) {
        inode_unlock(old_dir->i_ino);
    }
    return ret;

}

int slotfs_truncate(const char *path, off_t length) {
    return 0;
}

int slotfs_ftruncate(int fd, off_t length) {
    f_header_t *fh = &rt.f_table[fd];
    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return -1;
    }

    int ret;
    inode_t *inode = fh->file;
    ret = inode_resize(inode, length);
    if (ret) {
        set_errno(ret);
    }
    return ret;
}

int slotfs_close(int fd) {
    if (unlikely(!rt.f_table[fd].file)) {
        set_errno(-EBADFD);
        return -1;
    }

    if (unlikely(!rt.f_table[fd].valid)) {
        set_errno(-EBADFD);
        return -1;
    }

    inode_t *inode = rt.f_table[fd].file;
    inode_lock(inode->i_ino);
    inode->ref--;
    if (inode->ref == 0 && inode->i_link == 0) {
        index_t ino = inode->i_ino;
        inode_release(inode);
        sbi->inode_table[ino].inode = NULL;
        // do_inode_release(inode);
        inode_unlock(ino);
    } else if (inode->ref == 0) {
        inode_flush(inode);
        inode_unlock(inode->i_ino);
    } else {
        inode_unlock(inode->i_ino);
    }
    fd_free(fd);
    return 0;
}

ssize_t slotfs_pread(int fd, void *buf, size_t count, off_t offset) {
    f_header_t *fh = &rt.f_table[fd];

    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return -1;
    }

    inode_t *inode = fh->file;
    int ret = inode_read(inode, buf, count, offset);
    if (ret > 0) 
        fh->offset += ret;
    return ret;
}

ssize_t slotfs_read(int fd, void *buf, size_t count) {
    f_header_t *fh = &rt.f_table[fd];

    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return -1;
    }

    inode_t *inode = fh->file;
    int ret = inode_read(inode, buf, count, fh->offset);
    if (ret > 0) 
        fh->offset += ret;
    return ret;
}

ssize_t slotfs_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    f_header_t *fh = &rt.f_table[fd];

    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return -1;
    }

    inode_t *inode = fh->file;
    int ret = inode_write(inode, buf, count, offset);
    if (ret > 0) 
        fh->offset += ret;
    return ret;
}

ssize_t slotfs_write(int fd, const void *buf, size_t count) {
    int ret = 0;
    f_header_t *fh = &rt.f_table[fd];

    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return -1;
    }

    inode_t *inode = fh->file;
    ret = inode_write(inode, buf, count, fh->offset);
    if (ret > 0) 
        fh->offset += ret;
    return ret;
}

int slotfs_lseek(int fd, off_t offset, int whence) {
    f_header_t *fh = &rt.f_table[fd];
    switch (whence) {
        case SEEK_SET:
            fh->offset = offset;
            break;
        case SEEK_CUR:
            fh->offset += offset;
            break;
        case SEEK_END:
            fh->offset = fh->file->i_size + offset;
            break;
        default:
            set_errno(EINVAL);
            return -1;
    }
    return 0;
}

void* slotfs_mmap(void *addr, size_t len, int prot, int flags, int file, off_t off) {
    inode_t *inode = file_table_get(file);
    if (IS_ERR(inode)) {
        set_errno(-EBADFD);
        return MAP_FAILED;
    }
    
    inode_lock(inode->i_ino);
    
    size_t mapped = 0;
    char *base = addr ? (char*)addr : NULL;
    char *curr_map = NULL;
    
    while (mapped < len) {
        unsigned long block_idx = (off + mapped) >> PAGE_SHIFT;
        filent_t *fe = filent_lookup(inode, block_idx);
        if (!fe) {
            inode_unlock(inode->i_ino);
            set_errno(EIO);
            return MAP_FAILED;
        }
        
        size_t block_offset = (off + mapped) & PAGE_MASK;
        size_t extent_bytes = fe->blocks * PAGE_SIZE - block_offset;
        size_t to_map = (len - mapped < extent_bytes) ? (len - mapped) : extent_bytes;
        
        char *phys_addr = (char*)REL2ABS(IDX2DATA(fe->p_idx)) + block_offset;
        
        char *vaddr = mmap(base ? base + mapped : NULL, to_map, prot, flags | MAP_FIXED, -1, 0);
        if (vaddr == MAP_FAILED) {
            inode_unlock(inode->i_ino);
            return MAP_FAILED;
        }
        
        if (mapped > 0 && ((char*)curr_map + to_map) != vaddr) {
            inode_unlock(inode->i_ino);
            munmap(base, mapped); // Rollback
            set_errno(EFAULT);
            return MAP_FAILED;
        }
        
        curr_map = vaddr;
        mapped += to_map;
    }
    
    inode_unlock(inode->i_ino);
    return base ? base : curr_map;
}

int slotfs_stat(int fd, const char *path, struct stat *buf, int flags) {
    nameidata_t nd;
    inode_t *inode;
    int ret;

    if (path) {
        nameidata_init(&nd, fd, path, flags);
        if (unlikely(IS_ERR(nd.start))) {
            return PTR_ERR(nd.start);
        }

        ret = path_lookup(&nd);
        if (unlikely(ret)) {
            return ret;
        }
        inode = nd.current;
    } else {
        inode_t *inode = file_table_get(fd);
        if (IS_ERR(inode)) {
            return PTR_ERR(inode);
        }
    }
    
    buf->st_ino = inode->i_ino;
    buf->st_mode = inode->i_mode;
    buf->st_nlink = inode->i_link;
    buf->st_uid = inode->i_uid;
    buf->st_gid = inode->i_gid;
    buf->st_size = inode->i_size;
    buf->st_atime = inode->i_atim;
    buf->st_mtime = inode->i_mtim;
    buf->st_ctime = inode->i_ctim;
    buf->st_blksize = PAGE_SIZE;
    
    inode_unlock(inode->i_ino);

    return 0;
}

int slotfs_xstat(int fd, const char *path, int flags, 
    unsigned int mask, struct statx *statxbuf) 
{
    nameidata_t nd;
    inode_t *inode;
    int ret;

    if (path) {
        nameidata_init(&nd, fd, path, flags);
        if (unlikely(IS_ERR(nd.start))) {
            return PTR_ERR(nd.start);
        }
        ret = path_lookup(&nd);
        if (unlikely(ret)) {
            return ret;
        }
        inode = nd.current;
    } else {
        inode_t *inode = file_table_get(fd);
        if (IS_ERR(inode)) {
            return PTR_ERR(inode);
        }
    }

    statxbuf->stx_ino = inode->i_ino;
    statxbuf->stx_mode = inode->i_mode;
    statxbuf->stx_nlink = inode->i_link;
    statxbuf->stx_uid = inode->i_uid;
    statxbuf->stx_gid = inode->i_gid;
    statxbuf->stx_size = inode->i_size;
    statxbuf->stx_blocks = (inode->i_size + PAGE_SIZE - 1) / PAGE_SIZE;
    statxbuf->stx_mask = (STATX_TYPE | STATX_MODE | STATX_NLINK |
        STATX_UID | STATX_GID | STATX_INO | STATX_SIZE) & mask;
    
    inode_unlock(inode->i_ino);
    return 0;
}

DIR* slotfs_opendir(const char *path) {
    return (DIR *)(uint64_t)slotfs_open(AT_FDCWD, path, O_RDONLY | O_DIRECTORY, 0);
}

struct dirent *slotfs_readdir(DIR *dirp) {
    int fd = (int)(uint64_t)dirp;
    f_header_t *fh = &rt.f_table[fd];

    if (unlikely(!fh->valid || !fh->file)) {
        set_errno(-EBADFD);
        return NULL;
    }

    if (unlikely(fh->flags & O_WRONLY)) {
        set_errno(-EBADF);
        return NULL;
    }

    inode_t *inode = fh->file;
    if (unlikely(!is_dir(inode))) {
        set_errno(-ENOTDIR);
        return NULL;
    }

    unsigned long pos = fh->offset;
    inode_lock(inode->i_ino);

    struct rb_node *temp = NULL;
    dirent_t *d = NULL;

    if (pos == 0) {
        temp = rb_first(&inode->rb_root);
    } else if (pos == READDIR_END) {
        inode_unlock(inode->i_ino);
        return NULL;
    } else {
        d = dirent_lookup(inode, pos);
        if (d) {
            temp = rb_next(&d->rb_node);
        }
    }

    if (!temp) {
        fh->offset = READDIR_END;
        inode_unlock(inode->i_ino);
        return NULL;
    }

    d = container_of(temp, dirent_t, rb_node);
    fh->offset = d->hash;
    if (!fh->temp_dirent) {
        fh->temp_dirent = heap_alloc(&sbi->heap_allocator, sizeof(struct dirent));
        if (!fh->temp_dirent) {
            inode_unlock(inode->i_ino);
            return NULL;
        }
    }

    struct dirent* dirent = fh->temp_dirent;
    dirent->d_ino = d->ino;
    dirent->d_off = fh->offset;
    dirent->d_reclen = sizeof(struct dirent);
    strncpy(dirent->d_name, ((pm_dirent_t *)REL2ABS(IDX2DENT(d->idx)))->name, sizeof(dirent->d_name) - 1);
    dirent->d_name[sizeof(dirent->d_name) - 1] = '\0';

    inode_unlock(inode->i_ino);
    return dirent;
}