#include "lib.h"
#include "utils/timer.h"

namespace madfs {
extern "C" {
int close(int fd) {
  if (auto file = get_file(fd)) {
    TimerGuard<Event::CLOSE> guard;
    LOG_DEBUG("madfs::close(%s)", file->path);
    files.unsafe_erase(fd);
    return 0;
  } else {
    LOG_DEBUG("posix::close(%d)", fd);
    // posix::close(fd);
    return SAFE_CALL_POSIX_FN(close, fd);
  }
}

int fclose(FILE* stream) {
  int fd = fileno(stream);
  if (auto file = get_file(fd)) {
    LOG_DEBUG("madfs::fclose(%s)", file->path);
    file->fd = -1;
    files.unsafe_erase(fd);
    return SAFE_CALL_POSIX_FN(fclose, stream);
  } else {
    LOG_DEBUG("posix::fclose(%p)", stream);
    return SAFE_CALL_POSIX_FN(fclose, stream);
  }
}
}
}  // namespace madfs
