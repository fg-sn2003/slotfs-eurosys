#ifndef __WRAPPER_H
#define __WRAPPER_H

#include <boost/preprocessor/seq/for_each.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>

#define SLOTFS_ALL_OPS    (OPEN) (CREAT) (LIBC_OPEN64) (OPEN64) (MKDIR) (CLOSE) (SEEK) \
                        (READ) (WRITE) (PREAD) (PREAD64) (PWRITE) (PWRITE64) \
                        (STAT) (STAT64) (FSTAT) (FSTAT64) (LSTAT) (LSTAT64) (LSTAT64_TIME64) (FSTATFS) (XSTAT) (XSTAT64) (NEWFSTATAT)\
                        (FOPEN) (FOPEN64) (FPUTS) (FGETS) (FWRITE) (FREAD) (FCLOSE) (FSEEK) \
                        (OPENAT) (OPENAT64) (ACCESS) (TRUNC) (FTRUNC) (FSYNC) \
                        (READ2) (RENAME) (RMDIR) (FDATASYNC) (FCNTL) (FCNTL2) (FFLUSH) \
                        (OPENDIR) (CLOSEDIR) (READDIR) (READDIR64) (ERROR) (SYNC_FILE_RANGE) \
                        (LINK) (UNLINK) (UNLINKAT) (SYMLINK) (SYMLINKAT) \
                        (IOCTL) (FADVISE) (MMAP) (MMAP64)


# define EMPTY(...)
# define DEFER(...) __VA_ARGS__ EMPTY()
# define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
# define EXPAND(...) __VA_ARGS__

#define MK_STR(arg) #arg
#define MK_STR2(x) MK_STR(x)
#define MK_STR3(x) MK_STR2(x)

#define ALIAS_OPEN   open
#define ALIAS_CREAT  creat
#define ALIAS_EXECVE execve
#define ALIAS_EXECVP execvp
#define ALIAS_EXECV execv
#define ALIAS_MKNOD __xmknod
#define ALIAS_MKNODAT __xmknodat

#define ALIAS_FOPEN  	fopen
#define ALIAS_FOPEN64  	fopen64
#define ALIAS_FREAD  	fread
#define ALIAS_FEOF 	 	feof
#define ALIAS_FERROR 	ferror
#define ALIAS_CLEARERR 	clearerr
#define ALIAS_FWRITE 	fwrite
#define ALIAS_FSEEK  	fseek
#define ALIAS_FTELL  	ftell
#define ALIAS_FTELLO 	ftello
#define ALIAS_FCLOSE 	fclose
#define ALIAS_FPUTS		fputs
#define ALIAS_FGETS		fgets
#define ALIAS_FFLUSH	fflush

#define ALIAS_FSTATFS	fstatfs
#define ALIAS_FDATASYNC	fdatasync
#define ALIAS_FCNTL		fcntl
#define ALIAS_FCNTL2	__fcntl64_nocancel_adjusted
#define ALIAS_OPENDIR		opendir
#define ALIAS_READDIR	readdir
#define ALIAS_READDIR64	readdir64
#define ALIAS_CLOSEDIR	closedir
#define ALIAS_ERROR		__errno_location
#define ALIAS_SYNC_FILE_RANGE	sync_file_range

#define ALIAS_ACCESS access
#define ALIAS_READ   read
#define ALIAS_READ2		__libc_read
#define ALIAS_WRITE  write
#define ALIAS_SEEK   lseek
#define ALIAS_CLOSE  close
#define ALIAS_FTRUNC ftruncate
#define ALIAS_TRUNC  truncate
#define ALIAS_DUP    dup
#define ALIAS_DUP2   dup2
#define ALIAS_FORK   fork
#define ALIAS_VFORK  vfork
#define ALIAS_MMAP   mmap
#define ALIAS_READV  readv
#define ALIAS_WRITEV writev
#define ALIAS_PIPE   pipe
#define ALIAS_SOCKETPAIR   socketpair
#define ALIAS_IOCTL  ioctl
#define ALIAS_MUNMAP munmap
#define ALIAS_MSYNC  msync
#define ALIAS_CLONE  __clone
#define ALIAS_PREAD  pread
#define ALIAS_PREAD64  pread64
#define ALIAS_PWRITE64 pwrite64
#define ALIAS_PWRITE pwrite
//#define ALIAS_PWRITESYNC pwrite64_sync
#define ALIAS_FSYNC  fsync
#define ALIAS_FDSYNC fdatasync
#define ALIAS_FTRUNC64 ftruncate64
#define ALIAS_OPEN64  open64
#define ALIAS_LIBC_OPEN64 __libc_open64
#define ALIAS_SEEK64  lseek64
#define ALIAS_MMAP64  mmap64
#define ALIAS_MKSTEMP mkstemp
#define ALIAS_MKSTEMP64 mkstemp64
#define ALIAS_ACCEPT  accept
#define ALIAS_SOCKET  socket
#define ALIAS_UNLINK  unlink
#define ALIAS_POSIX_FALLOCATE posix_fallocate
#define ALIAS_POSIX_FALLOCATE64 posix_fallocate64
#define ALIAS_FALLOCATE fallocate
#define ALIAS_STAT stat
#define ALIAS_STAT64 stat64
#define ALIAS_XSTAT __xstat
#define ALIAS_XSTAT64 __xstat64
#define ALIAS_FSTAT fstat
#define ALIAS_FSTAT64 fstat64
#define ALIAS_LSTAT lstat
#define ALIAS_LSTAT64 lstat64
#define ALIAS_LSTAT64_TIME64 __lstat64_time64
#define ALIAS_NEWFSTATAT newfstatat
/* Now all the metadata operations */
#define ALIAS_MKDIR mkdir
#define ALIAS_RENAME rename
#define ALIAS_LINK link
#define ALIAS_SYMLINK symlink
#define ALIAS_RMDIR rmdir
/* All the *at operations */
#define ALIAS_OPENAT openat
#define ALIAS_OPENAT64 openat64
#define ALIAS_SYMLINKAT symlinkat
#define ALIAS_MKDIRAT mkdirat
#define ALIAS_UNLINKAT  unlinkat

#define ALIAS_FADVISE   posix_fadvise64

// The function return type
#define RETT_OPEN   int
#define RETT_LIBC_OPEN64 int
#define RETT_CREAT  int
#define RETT_EXECVE int
#define RETT_EXECVP int
#define RETT_EXECV int
#define RETT_SHM_COPY void
#define RETT_MKNOD int
#define RETT_MKNODAT int

// #ifdef TRACE_FP_CALLS
#define RETT_FOPEN  FILE*
#define RETT_FOPEN64  FILE*
#define RETT_FWRITE size_t
#define RETT_FSEEK  int
#define RETT_FTELL  long int
#define RETT_FTELLO off_t
#define RETT_FCLOSE int
#define RETT_FPUTS	int
#define RETT_FGETS	char*
#define RETT_FFLUSH	int
// #endif

#define RETT_FSTATFS	int
#define RETT_FDATASYNC	int
#define RETT_FCNTL		int
#define RETT_FCNTL2		int
#define RETT_OPENDIR	DIR *
#define RETT_READDIR	struct dirent *
#define RETT_READDIR64	struct dirent64 *
#define RETT_CLOSEDIR	int
#define RETT_ERROR		int *
#define RETT_SYNC_FILE_RANGE int

#define RETT_ACCESS int
#define RETT_READ   ssize_t
#define RETT_READ2   ssize_t
#define RETT_FREAD  size_t
#define RETT_FEOF   int
#define RETT_FERROR int
#define RETT_CLEARERR void
#define RETT_WRITE  ssize_t
#define RETT_SEEK   off_t
#define RETT_CLOSE  int
#define RETT_FTRUNC int
#define RETT_TRUNC  int
#define RETT_DUP    int
#define RETT_DUP2   int
#define RETT_FORK   pid_t
#define RETT_VFORK  pid_t
#define RETT_MMAP   void*
#define RETT_READV  ssize_t
#define RETT_WRITEV ssize_t
#define RETT_PIPE   int
#define RETT_SOCKETPAIR   int
#define RETT_IOCTL  int
#define RETT_MUNMAP int
#define RETT_MSYNC  int
#define RETT_CLONE  int
#define RETT_PREAD  ssize_t
#define RETT_PREAD64  ssize_t
#define RETT_PWRITE ssize_t
#define RETT_PWRITE64 ssize_t
//#define RETT_PWRITESYNC ssize_t
#define RETT_FSYNC  int
#define RETT_FDSYNC int
#define RETT_FTRUNC64 int
#define RETT_OPEN64  int
#define RETT_SEEK64  off64_t
#define RETT_MMAP64  void*
#define RETT_MKSTEMP int
#define RETT_MKSTEMP64 int
#define RETT_ACCEPT  int
#define RETT_SOCKET  int
#define RETT_UNLINK  int
#define RETT_POSIX_FALLOCATE int
#define RETT_POSIX_FALLOCATE64 int
#define RETT_FALLOCATE int
#define RETT_STAT int
#define RETT_STAT64 int
#define RETT_XSTAT int
#define RETT_XSTAT64 int
#define RETT_FSTAT int
#define RETT_FSTAT64 int
#define RETT_LSTAT int
#define RETT_LSTAT64 int
#define RETT_LSTAT64_TIME64 int
#define RETT_NEWFSTATAT int
/* Now all the metadata operations */
#define RETT_MKDIR int
#define RETT_RENAME int
#define RETT_LINK int
#define RETT_SYMLINK int
#define RETT_RMDIR int
/* All the *at operations */
#define RETT_OPENAT int
#define RETT_OPENAT64 int
#define RETT_SYMLINKAT int
#define RETT_MKDIRAT int
#define RETT_UNLINKAT int

#define RETT_FADVISE int

// The function interface
#define INTF_OPEN const char *path, int oflag, ...
#define INTF_LIBC_OPEN64 const char *path, int oflag, ...

#define INTF_CREAT const char *path, mode_t mode
#define INTF_EXECVE const char *filename, char *const argv[], char *const envp[]
#define INTF_EXECVP const char *file, char *const argv[]
#define INTF_EXECV const char *path, char *const argv[]
#define INTF_SHM_COPY void
#define INTF_MKNOD int ver, const char* path, mode_t mode, dev_t* dev
#define INTF_MKNODAT int ver, int dirfd, const char* path, mode_t mode, dev_t* dev

// #ifdef TRACE_FP_CALLS
#define INTF_FOPEN  const char* __restrict path, const char* __restrict mode
#define INTF_FOPEN64  const char* __restrict path, const char* __restrict mode
#define INTF_FREAD  void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_CLEARERR FILE* fp
#define INTF_FEOF   FILE* fp
#define INTF_FERROR FILE* fp
#define INTF_FWRITE const void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_FSEEK  FILE* fp, long int offset, int whence
#define INTF_FTELL  FILE* fp
#define INTF_FTELLO FILE* fp
#define INTF_FCLOSE FILE* fp
#define INTF_FPUTS	const char *str, FILE *stream
#define INTF_FGETS	char *str, int n, FILE *stream
#define INTF_FFLUSH	FILE* fp
// #endif

#define INTF_FSTATFS	int fd, struct statfs *buf
#define INTF_FDATASYNC	int fd
#define INTF_FCNTL		int fd, int cmd, ...
#define INTF_FCNTL2		int fd, int cmd, void *arg
#define INTF_OPENDIR	const char *path
#define INTF_READDIR	DIR *dirp
#define INTF_READDIR64	DIR *dirp
#define INTF_CLOSEDIR	DIR *dirp
#define INTF_ERROR		void
#define INTF_SYNC_FILE_RANGE int fd, off64_t offset, off64_t nbytes, unsigned int flags


#define INTF_ACCESS const char *pathname, int mode
#define INTF_READ   int file, void* buf, size_t length
#define INTF_READ2   int file, void* buf, size_t length
#define INTF_WRITE  int file, const void* buf, size_t length
#define INTF_SEEK   int file, off_t offset, int whence
#define INTF_CLOSE  int file
#define INTF_FTRUNC int file, off_t length
#define INTF_TRUNC  const char* path, off_t length
#define INTF_DUP    int file
#define INTF_DUP2   int file, int fd2
#define INTF_FORK   void
#define INTF_VFORK  void
#define INTF_MMAP   void *addr, size_t len, int prot, int flags, int file, off_t off
#define INTF_READV  int file, const struct iovec *iov, int iovcnt
#define INTF_WRITEV int file, const struct iovec *iov, int iovcnt
#define INTF_PIPE   int file[2]
#define INTF_SOCKETPAIR   int domain, int type, int protocol, int sv[2]
#define INTF_IOCTL  int file, unsigned long int request, ...
#define INTF_MUNMAP void *addr, size_t len
#define INTF_MSYNC  void *addr, size_t len, int flags
#define INTF_CLONE  int (*fn)(void *a), void *child_stack, int flags, void *arg
#define INTF_PREAD  int file,       void *buf, size_t count, off_t offset
#define INTF_PREAD64  int file,       void *buf, size_t count, off_t offset
#define INTF_PWRITE int file, const void *buf, size_t count, off_t offset
#define INTF_PWRITE64 int file, const void *buf, size_t count, off_t offset
//#define INTF_PWRITESYNC int file, const void *buf, size_t count, off_t offset
#define INTF_FSYNC  int file
#define INTF_FDSYNC int file
#define INTF_FTRUNC64 int file, off64_t length
#define INTF_OPEN64  const char* path, int oflag, ...
#define INTF_SEEK64  int file, off64_t offset, int whence
#define INTF_MMAP64  void *addr, size_t len, int prot, int flags, int file, off64_t off
#define INTF_MKSTEMP char* file
#define INTF_MKSTEMP64 char* file
#define INTF_ACCEPT  int file, struct sockaddr *addr, socklen_t *addrlen
#define INTF_SOCKET  int domain, int type, int protocol
#define INTF_UNLINK  const char* path
#define INTF_POSIX_FALLOCATE int file, off_t offset, off_t len
#define INTF_POSIX_FALLOCATE64 int file, off_t offset, off_t len
#define INTF_FALLOCATE int file, int mode, off_t offset, off_t len
#define INTF_STAT const char *path, struct stat *buf
#define INTF_STAT64 const char *path, struct stat64 *buf
#define INTF_XSTAT int val, const char *path, struct stat *buf
#define INTF_XSTAT64 int val, const char *path, struct stat64 *buf
#define INTF_FSTAT int file, struct stat *buf
#define INTF_FSTAT64 int file, struct stat64 *buf
#define INTF_LSTAT const char *path, struct stat *buf
#define INTF_LSTAT64 const char *path, struct stat64 *buf
#define INTF_LSTAT64_TIME64 const char *path, struct stat64 *buf
#define INTF_NEWFSTATAT int dirfd, const char *path, struct stat *buf, int flags
/* Now all the metadata operations */
#define INTF_MKDIR const char *path, uint32_t mode
#define INTF_RENAME const char *old, const char *new
#define INTF_LINK const char *oldpath, const char *newpath
#define INTF_SYMLINK const char *target, const char *link
#define INTF_RMDIR const char *path
/* All the *at operations */
#define INTF_OPENAT int dirfd, const char* path, int oflag, ...
#define INTF_OPENAT64 int dirfd, const char* path, int oflag, ...
#define INTF_UNLINKAT  int dirfd, const char* path, int flags
#define INTF_SYMLINKAT const char* old_path, int newdirfd, const char* new_path
#define INTF_MKDIRAT int dirfd, const char* path, mode_t mode

#define INTF_FADVISE int fd, off_t offset, off_t len, int advice

#define TYPE_REL_SYSCALL(op) 		typedef RETT_##op (*real_##op##_t)(INTF_##op);
#define TYPE_REL_SYSCALL_WRAP(r, data, elem) 		TYPE_REL_SYSCALL(elem)

BOOST_PP_SEQ_FOR_EACH(TYPE_REL_SYSCALL_WRAP, placeholder, SLOTFS_ALL_OPS)

#define OP_DEFINE(op)		RETT_##op ALIAS_##op(INTF_##op)

#define INSERT_LATENCY     (void)insert_hodor_latency();

int pkey = 1;
void insert_hodor_latency() {
    unsigned int pkru;
    asm volatile("rdpkru" : "=a"(pkru) : "c"(0), "d"(0));
    pkru &= ~(1U << 3);
    asm volatile("wrpkru" :: "a"(pkru), "c"(0), "d"(0));
    // Insert 105 CPU cycles of delay
    asm volatile(
        ".rept 105\n\t"
        "nop\n\t"
        ".endr\n\t"
    );
}

void insert_syscall_latency() {
    volatile int pid = getpid(); 
}
#endif // __WRAPPER_H