#pragma once

#include <dlfcn.h>
#include <fcntl.h>
#include <gnu/lib-names.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "config.h"

struct posix_ops {
  int (*lseek)(int fd, off_t offset, int whence);
  ssize_t (*write)(int fd, const void* buf, size_t count);
  ssize_t (*pwrite)(int fd, const void* buf, size_t count, off_t offset);
  ssize_t (*read)(int fd, void* buf, size_t count);
  ssize_t (*pread)(int fd, void* buf, size_t count, off_t offset);
  int (*open)(const char* pathname, int flags, ...);
  FILE* (*fopen)(const char* pathname, const char* mode);
  int (*close)(int fd);
  int (*fclose)(FILE* stream);
  void* (*mmap)(void* addr, size_t length, int prot, int flags, int fd,
                off_t offset);
  void* (*mremap)(void* old_address, size_t old_size, size_t new_size,
                  int flags);
  int (*munmap)(void* addr, size_t length);
  int (*fallocate)(int fd, int mode, off_t offset, off_t len);
  int (*ftruncate)(int fd, off_t length);
  int (*fsync)(int fd);
  int (*fdatasync)(int fd);
  int (*fcntl)(int fd, int cmd, ...);
  int (*unlink)(const char* pathname);
  int (*rename)(const char* oldpath, const char* newpath);
  int (*fstat)(int ver, int fd, struct stat* buf);
  int (*stat)(int ver, const char* pathname, struct stat* buf);
};

static struct posix_ops posix_ops = {};
static void* glibc_handle = NULL;

#define LIBC_SO_LOC "/usr/lib64/libc.so.6"

#ifdef _STAT_VER

#define SAFE_CALL_POSIX_FN(fn, ...)                              \
  ({                                                             \
    if (!posix_ops.fn) {                                         \
      if (!glibc_handle) {                                       \
        glibc_handle = dlopen(LIBC_SO_LOC, RTLD_LAZY);           \
      }                                                          \
      if (strcmp(#fn, "fstat") == 0) {                           \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, "__fxstat"));                    \
      } else if (strcmp(#fn, "stat") == 0) {                     \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, "__xstat"));                     \
      } else {                                                   \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, #fn));                           \
        printf("fn: %s, %p\n", #fn, posix_ops.fn);               \
      }                                                          \
      assert(posix_ops.fn != nullptr);                           \
    }                                                            \
    posix_ops.fn(__VA_ARGS__);                                   \
  })

#else

#define SAFE_CALL_POSIX_FN(fn, ...)                              \
  ({                                                             \
    if (!posix_ops.fn) {                                         \
      if (!glibc_handle) {                                       \
        glibc_handle = dlopen(LIBC_SO_LOC, RTLD_LAZY);           \
      }                                                          \
      if (strcmp(#fn, "fstat") == 0) {                           \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, "fstat"));                       \
      } else if (strcmp(#fn, "stat") == 0) {                     \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, "stat"));                        \
      } else {                                                   \
        posix_ops.fn = reinterpret_cast<decltype(posix_ops.fn)>( \
            dlsym(glibc_handle, #fn));                           \
      }                                                          \
      assert(posix_ops.fn != nullptr);                           \
    }                                                            \
    posix_ops.fn(__VA_ARGS__);                                   \
  })

#endif

namespace madfs::posix {

/*
 * To use a posix function `fn`, you need to add a line DEFINE_FN(fn); It
 * declares the function pointer `posix::fn` of type `&::fn`, which is declared
 * in the system header. The value of is initialized to the value returned by
 * `dlsym` during runtime. The function pointer is declared as `inline` to avoid
 * multiple definitions in different translation units.
 *
 * The functions declared in the system headers are defined in `ops`, which
 * contains our implementation of the posix functions.
 *
 * For example, `::open` in the global namespace is declared in <fcntl.h> and
 * defined in our `ops/open.cpp`. The use of `extern "C"` makes sure that the
 * symbol for `madfs::open` is not mangled by C++, and thus is the same as
 * `open`. The posix version `posix::open` is declared and defined below.
 */

#define DEFINE_FN(fn)                                                       \
  inline const decltype(&::fn) fn = []() noexcept {                         \
    if (!glibc_handle) {                                                    \
      glibc_handle = dlopen(LIBC_SO_LOC, RTLD_LAZY);                        \
    }                                                                       \
    auto res = reinterpret_cast<decltype(&::fn)>(dlsym(glibc_handle, #fn)); \
    assert(res != nullptr);                                                 \
    return res;                                                             \
  }()

DEFINE_FN(lseek);
DEFINE_FN(write);
DEFINE_FN(pwrite);
DEFINE_FN(read);
DEFINE_FN(pread);
DEFINE_FN(open);
DEFINE_FN(fopen);
DEFINE_FN(close);
DEFINE_FN(fclose);
DEFINE_FN(mmap);
DEFINE_FN(mremap);
DEFINE_FN(munmap);
DEFINE_FN(fallocate);
DEFINE_FN(ftruncate);
DEFINE_FN(fsync);
DEFINE_FN(fdatasync);
DEFINE_FN(fcntl);
DEFINE_FN(unlink);
DEFINE_FN(rename);

namespace detail {
// See stat.cpp
#ifdef _STAT_VER
DEFINE_FN(__fxstat);
DEFINE_FN(__xstat);
#else
DEFINE_FN(fstat);
DEFINE_FN(stat);
#endif
}  // namespace detail

static int fstat(int fd, struct stat* buf) {
  __msan_unpoison(buf, sizeof(struct stat));
#ifdef _STAT_VER
  return detail::__fxstat(_STAT_VER, fd, buf);
#else
  return detail::fstat(fd, buf);
#endif
}

static int stat(const char* pathname, struct stat* buf) {
  __msan_unpoison(buf, sizeof(struct stat));
#ifdef _STAT_VER
  return detail::__xstat(_STAT_VER, pathname, buf);
#else
  return detail::stat(pathname, buf);
#endif
}

#undef DEFINE_FN

}  // namespace madfs::posix
