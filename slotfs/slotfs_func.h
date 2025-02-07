#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <inttypes.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <stdarg.h> 
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <linux/stat.h>

int     slotfs_close(int fd);
ssize_t slotfs_read(int fd, void *buf, size_t count);
ssize_t slotfs_write(int fd, const void *buf, size_t count);
int     slotfs_lseek(int fd, off_t offset, int whence);
int     slotfs_open(int dfd, const char *path, int flags, mode_t mode);
int     slotfs_close(int fd);
ssize_t slotfs_read(int fd, void *buf, size_t count); 
ssize_t slotfs_write(int fd, const void *buf, size_t count);
int     slotfs_lseek(int fd, off_t offset, int whence);
int     slotfs_stat(int fd, const char *path, struct stat *buf, int flags);
DIR*    slotfs_opendir(const char *path);
int     slotfs_link(const char *old_path, const char *new_path);
int     slotfs_unlink(const char *path);
int     slotfs_symlink(const char *target, const char *link);
ssize_t slotfs_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t slotfs_pwrite(int fd, const void *buf, size_t count, off_t offset);
int     slotfs_xstat(int fd, const char *path, int flags, unsigned int mask, struct statx *statxbuf);
int     slotfs_rename(const char *oldpath, const char *newpath);
int     slotfs_ftruncate(int fd, off_t length);
void*   slotfs_mmap(void *addr, size_t len, int prot, int flags, int file, off_t off);
struct dirent * slotfs_readdir(DIR *dirp);