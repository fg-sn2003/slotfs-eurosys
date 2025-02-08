#define _GNU_SOURCE
// #include "glibc/ffile.h"
#include "lib.h"
#include "shm.h"
#include "slotfs.h"
#include "config.h"
#include "slotfs_func.h"
#include "glibc/ffile.h"
#include "runtime.h"
#include <dlfcn.h>
#include <stdarg.h>

static struct real_ops {
    #define DEC_REL_SYSCALL(op) 	real_##op##_t op;
    #define DEC_REL_SYSCALL_WRAP(r, data, elem) 	DEC_REL_SYSCALL(elem)
    BOOST_PP_SEQ_FOR_EACH(DEC_REL_SYSCALL_WRAP, placeholder, SLOTFS_ALL_OPS)
} real_ops;

void insert_real_op() {
    #define FILL_REL_SYSCALL(op) 	real_ops.op = dlsym(RTLD_NEXT, MK_STR3(ALIAS_##op));
    #define FILL_REL_SYSCALL_WRAP(r, data, elem) 	FILL_REL_SYSCALL(elem)
    BOOST_PP_SEQ_FOR_EACH(FILL_REL_SYSCALL_WRAP, placeholder, SLOTFS_ALL_OPS)
}


/*******************************************************
 * todo functions
 *******************************************************/
// OP_DEFINE(MKDIRAT) {assert(0);}
// OP_DEFINE(CREAT) {assert(0);}
// OP_DEFINE(FTRUNC) {assert(0);}
// OP_DEFINE(DUP) {assert(0);}
// OP_DEFINE(DUP2) {assert(0);}
// OP_DEFINE(READV) {assert(0);}
// OP_DEFINE(WRITEV) {assert(0);}
// OP_DEFINE(IOCTL) {assert(0);}
// OP_DEFINE(MKNOD) {assert(0);}
// OP_DEFINE(MKNODAT) {assert(0);}
// OP_DEFINE(FTRUNC64) {assert(0);}
// OP_DEFINE(POSIX_FALLOCATE) {assert(0);}
// OP_DEFINE(POSIX_FALLOCATE64) {assert(0);}
// OP_DEFINE(FALLOCATE) {assert(0);}
// OP_DEFINE(ACCESS) {assert(0);}
// OP_DEFINE(FCNTL) {assert(0);}
// OP_DEFINE(FCNTL2) {assert(0);}
// OP_DEFINE(FADVISE) {assert(0);}


/*******************************************************
 * other functions
 *******************************************************/

OP_DEFINE(SEEK) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_lseek(file - FD_OFFSET, offset, whence);
    }
    syscall_trace("SEEK: %d, %ld, %d\n", file, offset, whence);
    return real_ops.SEEK(file, offset, whence);
}

OP_DEFINE(SEEK64) {
    return lseek(file, offset, whence);
}

OP_DEFINE(FTRUNC) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("FTRUNC: %d, %ld\n", file, length);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_ftruncate(file - FD_OFFSET, length);
    }
    return real_ops.FTRUNC(file, length);
}

OP_DEFINE(FTRUNC64) {
    return ftruncate(file, length);
}

OP_DEFINE(MMAP) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    syscall_trace("MMAP: %p, %ld, %d, %d, %d, %ld\n", addr, len, prot, flags, file, off);
    
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_mmap(addr, len, prot, flags, file - FD_OFFSET, off);
    }
    return real_ops.MMAP(addr, len, prot, flags, file, off);
}

OP_DEFINE(MMAP64) {
    return mmap(addr, len, prot, flags, file, off);
}

/*******************************************************
 * sync functions
 *******************************************************/
OP_DEFINE(FSYNC) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    syscall_trace("FSYNC: %d\n", file);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return 0;
    }
    return real_ops.FSYNC(file);
}

OP_DEFINE(FDATASYNC) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (fd >= FD_OFFSET && fd < (FD_MAX + FD_OFFSET)) {
        return 0;
    }
    return real_ops.FDATASYNC(fd);
}

OP_DEFINE(SYNC_FILE_RANGE) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (fd >= FD_OFFSET && fd < (FD_MAX + FD_OFFSET)) {
        return 0;
    }
    return real_ops.SYNC_FILE_RANGE(fd, offset, nbytes, flags);
}

/*******************************************************
 * dir functions
 *******************************************************/
OP_DEFINE(MKDIR) {
    int ret;
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("MKDIR: %s\n", path);

    if (*path == '\\' || *path != '/' || path[1] == '\\') {
        const char *p = path;
        while (*p == '\\') p++;

        ret = slotfs_open(AT_FDCWD, p, O_CREAT, mode | S_IFDIR);
        if (ret < 0 && *path != '\\') {
            return real_ops.MKDIR(path, mode);
        }
        return ret >= 0 ? 0 : -1;
    }
    return real_ops.MKDIR(path, mode);
}

OP_DEFINE(OPENDIR) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("OPENDIR: %s\n", path);

    if(*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        DIR* ret = slotfs_opendir(path);
        if (ret == NULL && *path != '\\') {
            return real_ops.OPENDIR(path);
        }
        return ret;
    } 

    return real_ops.OPENDIR(path);
}

OP_DEFINE(READDIR) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("READDIR: %lu\n", (uint64_t)dirp);

    if ((uint64_t)dirp >= FD_OFFSET && (uint64_t)dirp < (FD_MAX + FD_OFFSET)) {
		return slotfs_readdir((DIR*) ((uint64_t)dirp - FD_OFFSET));
	}
	else{
		return real_ops.READDIR(dirp);
	}
}

OP_DEFINE(RENAME) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("RENAME: %s -> %s\n", old, new);
    if (*old == '\\' || *old != '/' || old[1] == '\\') {
        if (*new == '\\' || *new != '/' || new[1] == '\\') {
            const char *op = old;
            while (*op == '\\') op++;
            const char *np = new;
            while (*np == '\\') np++;

            int ret = slotfs_rename(op, np);
            if (ret < 0 && (*old != '\\' || *new != '\\')) {
                return real_ops.RENAME(old, new);
            }
            return ret;
        }
    }
    return real_ops.RENAME(old, new);
}

/*******************************************************
 * stat functions
 *******************************************************/
OP_DEFINE(STAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    
    syscall_trace("STAT: %s\n", path);
    if(*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        int ret = slotfs_stat(AT_FDCWD, p, buf, 0);
        if (ret < 0 && *path != '\\') {
            return real_ops.STAT(path, buf);
        }
        return ret;
    }

    return real_ops.STAT(path, buf);
}

OP_DEFINE(STAT64) {
    return stat(path, (struct stat*)buf);
}

OP_DEFINE(XSTAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("XSTAT: %s\n", path);
    if (*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        int ret = slotfs_xstat(AT_FDCWD, p, 0, 0, buf);
        if (ret < 0 && *path != '\\') {
            return real_ops.XSTAT(0, path, buf);
        }
        return ret;
    }
    return real_ops.XSTAT(0, path, buf);
}

OP_DEFINE(XSTAT64) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("XSTAT64: %s\n", path);
    if (*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        int ret = slotfs_xstat(AT_FDCWD, p, 0, 0, buf);
        if (ret < 0 && *path != '\\') {
            return real_ops.XSTAT(0, path, buf);
        }
        return ret;
    }
    return real_ops.XSTAT(0, path, buf);
}

OP_DEFINE(FSTAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    
    syscall_trace("FSTAT: %d\n", file);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_stat(file - FD_OFFSET, NULL, buf, 0);
    }
    return real_ops.FSTAT(file, buf);
}

OP_DEFINE(FSTAT64) {
    INSERT_LATENCY
    return fstat(file, (struct stat*)buf);
}

OP_DEFINE(NEWFSTATAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("NEWFSTATAT: %d, %s\n", dirfd, path);
    if (*path == '\\' || *path != '/') {
        int fd;
        const char *p = path;
        while (*p == '\\') p++;

        if (dirfd == AT_FDCWD) {
            fd = AT_FDCWD;
        } else if (dirfd >= FD_OFFSET && dirfd < (FD_MAX + FD_OFFSET)) {
            fd = dirfd - FD_OFFSET;
        } else {
            return real_ops.NEWFSTATAT(dirfd, path, buf, flags);
        }
        int ret = slotfs_stat(fd, p, buf, flags);
        if (ret < 0 && *path != '\\') {
            return real_ops.NEWFSTATAT(dirfd, path, buf, flags);
        }
        return ret;
    }
    return real_ops.NEWFSTATAT(dirfd, path, buf, flags);
}

OP_DEFINE(LSTAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("LSTAT: %s\n", path);
    if(*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        int ret = slotfs_stat(AT_FDCWD, p, buf, AT_SYMLINK_NOFOLLOW);
        if (ret < 0 && *path != '\\') {
            return real_ops.LSTAT(path, buf);
        }
        return ret;
    }

    return real_ops.LSTAT(path, buf);
}

OP_DEFINE(LSTAT64) {
    return lstat(path, (struct stat*)buf);
}


/*******************************************************
 * open functions
 *******************************************************/
OP_DEFINE(OPEN) {
    int ret;
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("OPEN: %s, create: %s, oflag: %x\n", 
        path, (oflag & O_CREAT) ? "yes" : "no", oflag);
    if (*path == '\\' || *path != '/' || path[1] == '\\') {
        if(oflag & O_CREAT) { 
            va_list ap;
            mode_t mode;
            va_start(ap, oflag);
            mode = va_arg(ap, mode_t);

            const char *p = path;
            while (*p == '\\') p++;

            ret = slotfs_open(AT_FDCWD, p, oflag, mode);
            if (ret < 0 && *path != '\\') {
                return real_ops.OPEN(path, oflag, mode);
            }
        } else {
            ret = slotfs_open(AT_FDCWD, (*path == '\\')? path + 1 : path, oflag, 0);
            if (ret < 0  && *path != '\\') {
                return real_ops.OPEN(path, oflag);
            }
        }
        
        if(ret < 0){
			return ret;
		}

        return ret + FD_OFFSET;
    } else {
        if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPEN(path, oflag, mode);
		}
		else{
			return real_ops.OPEN(path, oflag);
		}
    }
}

OP_DEFINE(OPENAT) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("OPENAT: %s, create: %s, oflag: %x\n", 
        path, (oflag & O_CREAT) ? "yes" : "no", oflag);

    if (*path == '\\' || *path != '/') {
        const char *p = path;
        while (*p == '\\') p++;

        if (oflag & O_CREAT) {
            va_list ap;
            mode_t mode;
            va_start(ap, oflag);
            mode = va_arg(ap, mode_t);
            if (dirfd == AT_FDCWD) {
                dirfd = AT_FDCWD;
            } else if (dirfd >= FD_OFFSET && dirfd < (FD_MAX + FD_OFFSET)) {
                dirfd = dirfd - FD_OFFSET;
            } else {
                return real_ops.OPENAT(dirfd, path, oflag, mode);
            }
            int ret = slotfs_open(dirfd, p, oflag, mode);
            if (ret < 0 && *path != '\\') {
                return real_ops.OPENAT(dirfd, path, oflag, mode);
                return ret;
            }
            return ret + FD_OFFSET;
        } else {
            if (dirfd == AT_FDCWD) {
                dirfd = AT_FDCWD;
            } else if (dirfd >= FD_OFFSET && dirfd < (FD_MAX + FD_OFFSET)) {
                dirfd = dirfd - FD_OFFSET;
            } else {
                return real_ops.OPENAT(dirfd, path, oflag, 0);
            }
            int ret = slotfs_open(dirfd, p, oflag, 0);
            if (ret < 0 && *path != '\\') {
                return real_ops.OPENAT(dirfd, path, oflag);
            }
            return ret + FD_OFFSET;
        }


    } 

    if(oflag & O_CREAT){
        va_list ap;
        mode_t mode;
        va_start(ap, oflag);
        mode = va_arg(ap, mode_t);
        return real_ops.OPEN(path, oflag, mode);
    }
    else{
        return real_ops.OPEN(path, oflag);
    }
}

OP_DEFINE(OPEN64) {
    va_list ap;
    mode_t mode;
    va_start(ap, oflag);
    mode = va_arg(ap, mode_t);
    
    return open(path, oflag, mode);
}


OP_DEFINE(LIBC_OPEN64) {
    va_list ap;
    mode_t mode;
    va_start(ap, oflag);
    mode = va_arg(ap, mode_t);
    
    return open(path, oflag, mode);
}

OP_DEFINE(CLOSE) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("CLOSE: %d\n", file);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_close(file - FD_OFFSET);
    } else {
        return real_ops.CLOSE(file);
    }
}

/*******************************************************
 * link functions
 *******************************************************/
OP_DEFINE(LINK) {
    int ret;
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("LINK: %s -> %s\n", newpath, oldpath);

    if (*newpath == '\\' || *newpath != '/' || newpath[1] == '\\') {
        if (*oldpath == '\\' || *oldpath != '/' || oldpath[1] == '\\') {
            const char* op = oldpath;
            while (*op == '\\') op++;
            const char *np = newpath;
            while (*np == '\\') np++;

            ret = slotfs_link(op, np);
            if (ret < 0 && *oldpath != '\\' && *newpath != '\\') {
                return real_ops.LINK(oldpath, newpath);
            }
            return ret;
        }
    
    }

    return real_ops.LINK(oldpath, newpath);
}

OP_DEFINE(UNLINK) {
    int ret;
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();

    syscall_trace("UNLINK: %s\n", path);

    if (*path == '\\' || *path != '/' || path[1] == '\\') {
        const char *p = path;
        while (*p == '\\') p++;

        ret = slotfs_unlink(p);
        if (ret < 0 && *path != '\\') {
            return real_ops.UNLINK(path);
        }
        return ret;
    }

    return real_ops.UNLINK(path);
}

OP_DEFINE(SYMLINK) {
    int ret;
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) 
        insert_real_op();
    syscall_trace("SYMLINK: %s -> %s\n", link, target);

    if (*target == '\\' || *target != '/' || target[1] == '\\') {
        if (*link == '\\' || *link != '/' || link[1] == '\\') {
            const char *lp = link;
            while (*lp == '\\') lp++;
            const char* tp = target;
            while (*tp == '\\') tp++;

            ret = slotfs_symlink(tp, lp);
            if (ret < 0 && *link != '\\' && *target != '\\') {
                return real_ops.SYMLINK(target, link);
            }
            return ret;
        }
    }

    return real_ops.SYMLINK(target, link);
}


/*******************************************************
 * read/write functions
 *******************************************************/
OP_DEFINE(READ) {
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    INSERT_LATENCY
    syscall_trace("READ: %d, %p, %ld\n", file, buf, length);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_read(file - FD_OFFSET, buf, length);
    }
    return real_ops.READ(file, buf, length);
}

OP_DEFINE(WRITE) {
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    INSERT_LATENCY
    syscall_trace("WRITE: %d, %p, %ld\n", file, buf, length);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_write(file - FD_OFFSET, buf, length);
    }
    return real_ops.WRITE(file, buf, length);
}

OP_DEFINE(READ2) {
    return read(file, buf, length);
}

OP_DEFINE(PREAD) {
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    INSERT_LATENCY
    syscall_trace("PREAD: %d, %p, %ld, %ld\n", file, buf, count, offset);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_pread(file - FD_OFFSET, buf, count, offset);
    }
    return real_ops.PREAD(file, buf, count, offset);
}

OP_DEFINE(PREAD64) {
    return pread(file, buf, count, offset);
}

OP_DEFINE(PWRITE) {
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    INSERT_LATENCY
    syscall_trace("PWRITE: %d, %p, %ld, %ld\n", file, buf, count, offset);
    if (file >= FD_OFFSET && file < (FD_MAX + FD_OFFSET)) {
        return slotfs_pwrite(file - FD_OFFSET, buf, count, offset);
    }
    return real_ops.PWRITE(file, buf, count, offset);
}

OP_DEFINE(PWRITE64) {
    return pwrite(file, buf, count, offset);
}

/*******************************************************
 * File stream functions
 *******************************************************/
OP_DEFINE(FOPEN) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (*path == '\\' || *path != '/') {
        FILE* ret;
        while (*path == '\\') path++;
        ret = _fopen(path, mode);
        if (ret == NULL && *path != '\\') {
            return real_ops.FOPEN(path, mode);
        }
        return ret;
    }
    return real_ops.FOPEN(path, mode);
}

OP_DEFINE(FPUTS) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (1) {
        if ((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fputs(str, stream);
        }
    }
    return real_ops.FPUTS(str, stream);
}

OP_DEFINE(FGETS) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (likely(stream)) {
        if ((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fgets(str, n, stream);
        }
    }
    return real_ops.FGETS(str, n, stream);
}

OP_DEFINE(FWRITE) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (1) {   
        if ((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fwrite(buf, length, nmemb, fp);
        }
    }
    return real_ops.FWRITE(buf, length, nmemb, fp);
}

OP_DEFINE(FREAD) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (likely(fp)) {
        if ((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fread(buf, length, nmemb, fp);
        }
    }
    return real_ops.FREAD(buf, length, nmemb, fp);
}

OP_DEFINE(FCLOSE) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (likely(fp)) {
        if ((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fclose(fp);
        }
    }
    return real_ops.FCLOSE(fp);
}

OP_DEFINE(FSEEK) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }

    syscall_trace("FSEEK: %p, %ld, %d\n", fp, offset, whence);
    if (likely(fp)){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT){;
			return _fseek(fp, offset, whence);
		}
	}
    return real_ops.FSEEK(fp, offset, whence);
}

OP_DEFINE(FFLUSH) {
    INSERT_LATENCY
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (likely(fp)) {
        if ((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_SLOT) {
            return _fflush(fp);
        }
    }
    return real_ops.FFLUSH(fp);
}

static int err;
OP_DEFINE(ERROR) {
    if (unlikely(real_ops.ERROR == NULL)) {
        insert_real_op();
    }
    if (*slotfs_errno() == 0) {
        return real_ops.ERROR();
    } else {
        err = *slotfs_errno();
        *slotfs_errno() = 0;
        return &err;
    }
}


static __attribute__((constructor)) void slotfs_ctor(void)
{
    printf("Initialize SlotFS\n");
    slotfs_init();
}

static __attribute__((destructor)) void slotfs_dtor(void) {
    slotfs_exit();
    printf("Unload Slotfs\n\n");
}