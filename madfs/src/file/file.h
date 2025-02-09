#pragma once

#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <tbb/concurrent_unordered_map.h>

#include <cstdint>
#include <ctime>
#include <iostream>
#include <stdexcept>

#include "alloc/alloc.h"
#include "bitmap.h"
#include "blk_table.h"
#include "block/block.h"
#include "config.h"
#include "const.h"
#include "entry.h"
#include "idx.h"
#include "mem_table.h"
#include "offset.h"
#include "posix.h"
#include "shm.h"
#include "tx/lock.h"
#include "utils/utils.h"

// try to open a file with checking whether the given file is in MadFS format
static bool try_open(int& fd, struct stat& stat_buf, const char* pathname,
                      int flags, mode_t mode) {
  madfs::TimerGuard<madfs::Event::OPEN_SYS> timer_guard;

  if ((flags & O_ACCMODE) == O_WRONLY) {
    LOG_INFO("File \"%s\" opened with O_WRONLY. Changed to O_RDWR.",
              pathname);
    flags &= ~O_WRONLY;
    flags |= O_RDWR;
  }

  fd = SAFE_CALL_POSIX_FN(open, pathname, flags, mode);
  if (unlikely(fd < 0)) {
    LOG_WARN("File \"%s\" open failed: %m", pathname);
    return false;
  }

  if (!(flags & O_CREAT)) {
    // a non-empty file w/o shm_path cannot be a MadFS file
    ssize_t rc = fgetxattr(fd, madfs::SHM_XATTR_NAME, nullptr, 0);
    if (rc == -1) return false;
  }

  int rc = SAFE_CALL_POSIX_FN(fstat, _STAT_VER, fd, &stat_buf);
  // posix::fstat(fd, &stat_buf);
  if (unlikely(rc < 0)) {
    LOG_WARN("File \"%s\" fstat failed: %m. Fallback to syscall.", pathname);
    return false;
  }

  // we don't handle non-normal file (e.g., socket, directory, block dev)
  if (unlikely(!S_ISREG(stat_buf.st_mode) && !S_ISLNK(stat_buf.st_mode))) {
    LOG_WARN("Non-normal file \"%s\". Fallback to syscall.", pathname);
    return false;
  }

  if (!IS_ALIGNED(stat_buf.st_size, madfs::BLOCK_SIZE)) {
    LOG_WARN("File size not aligned for \"%s\". Fallback to syscall",
              pathname);
    return false;
  }

  return true;
}

namespace madfs::utility {
class Converter;
}  // namespace madfs::utility

// data structure under this namespace must be in volatile memory (DRAM)
namespace madfs::dram {

class File {
 public:
  BitmapMgr bitmap_mgr;
  MemTable mem_table;
  OffsetMgr offset_mgr;
  BlkTable blk_table;
  ShmMgr shm_mgr;
  pmem::MetaBlock* const meta;
  Lock lock;         // nop lock is used by default
  const char* path;  // only set at debug mode
  int fd;            // only used in destructor, can set to -1 to prevent close
  const bool can_read;
  const bool can_write;

 private:
  // each thread tid has its local allocator
  // the allocator is a per-thread per-file data structure
  tbb::concurrent_unordered_map<pid_t, Allocator> allocators;

 public:
  File(int fd, const struct stat& stat, int flags, const char* pathname);
  ~File();

  /*
   * POSIX I/O operations
   */
  ssize_t pwrite(const char* buf, size_t count, size_t offset);
  ssize_t write(const char* buf, size_t count);
  ssize_t pread(char* buf, size_t count, size_t offset);
  ssize_t read(char* buf, size_t count);
  off_t lseek(off_t offset, int whence);
  void* mmap(void* addr, size_t length, int prot, int flags,
             size_t offset) const;
  int fsync();
  void stat(struct stat* buf) {
    buf->st_size = static_cast<off_t>(blk_table.update_unsafe());
  }

  [[nodiscard]] Allocator* get_local_allocator() {
    if (auto it = allocators.find(tid); it != allocators.end()) {
      return &it->second;
    }

    auto [it, ok] = allocators.emplace(
        std::piecewise_construct, std::forward_as_tuple(tid),
        std::forward_as_tuple(&mem_table, &bitmap_mgr,
                              shm_mgr.alloc_per_thread_data()));
    PANIC_IF(!ok, "insert to thread-local allocators failed");
    return &it->second;
  }

  friend std::ostream& operator<<(std::ostream& out, File& f);
};

}  // namespace madfs::dram
