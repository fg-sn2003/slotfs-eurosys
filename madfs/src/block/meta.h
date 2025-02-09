#pragma once

#include <pthread.h>

#include <atomic>
#include <cassert>
#include <cstring>

#include "const.h"
#include "entry.h"
#include "idx.h"
#include "posix.h"
#include "utils/utils.h"

namespace madfs::dram {
struct TxCursor;
}

namespace madfs::pmem {

/*
 * LogicalBlockIdx 0 -> MetaBlock; other blocks can be any type of blocks
 */
class MetaBlock : public noncopyable {
 private:
  // contents in the first cache line
  union {
    struct {
      // file signature
      char signature[SIGNATURE_SIZE];

      // all tx entries before it must be flushed
      std::atomic<TxEntryIdx> flushed_tx_tail;
      static_assert(std::atomic<TxEntryIdx>::is_always_lock_free);

      // if inline tx_entries are used up, this points to the next log block
      std::atomic<LogicalBlockIdx> next_tx_block;

      // orphan but not yet freed tx blocks are organized as a linked list;
      // these blocks are freed once they are not referenced by others
      std::atomic<LogicalBlockIdx> next_orphan_block;
    } cl1_meta;

    // padding avoid cache line contention
    char cl1[CACHELINE_SIZE];
  };

  static_assert(sizeof(cl1_meta) <= CACHELINE_SIZE);

  // move mutex to another cache line to avoid contention on reading the
  // metadata above
  union {
    struct {
      // this lock is ONLY used for bitmap rebuild
      pthread_mutex_t mutex;

      // total number of blocks actually in this file (including unused ones)
      // modifications to this should be through the getter/setter functions
      // that use atomic instructions
      std::atomic<uint32_t> num_logical_blocks;
    } cl2_meta;

    // padding
    char cl2[CACHELINE_SIZE];
  };

  static_assert(sizeof(cl2_meta) <= CACHELINE_SIZE);

  // 62 cache lines for tx log (~120 txs)
  std::atomic<TxEntry> tx_entries[NUM_INLINE_TX_ENTRY];

  static_assert(sizeof(tx_entries) == 62 * CACHELINE_SIZE);

 public:
  /**
   * only called if a new file is created
   * We can assume that all other fields are zero-initialized upon fallocate
   */
  void init() {
    // initialize the mutex
    init_robust_mutex(&cl2_meta.mutex);

    // initialize the signature
    memcpy(cl1_meta.signature, FILE_SIGNATURE, SIGNATURE_SIZE);
    persist_cl_fenced(&cl1);
  }

  // check whether the meta block is valid
  [[nodiscard]] bool is_valid() const {
    return std::strncmp(cl1_meta.signature, FILE_SIGNATURE, SIGNATURE_SIZE) ==
           0;
  }

  // acquire/release meta lock (usually only during allocation)
  // we don't need to call persistence since mutex is robust to crash
  void lock() {
    int rc = pthread_mutex_lock(&cl2_meta.mutex);
    if (rc == EOWNERDEAD) {
      LOG_WARN("Mutex owner died");
      rc = pthread_mutex_consistent(&cl2_meta.mutex);
      PANIC_IF(rc != 0, "pthread_mutex_consistent failed");
    }
    VALGRIND_PMC_SET_CLEAN(&cl2_meta.mutex, sizeof(cl2_meta.mutex));
  }

  void unlock() {
    int rc = pthread_mutex_unlock(&cl2_meta.mutex);
    PANIC_IF(rc != 0, "Mutex unlock failed");
    VALGRIND_PMC_SET_CLEAN(&cl2_meta.mutex, sizeof(cl2_meta.mutex));
  }

  /*
   * Getters and setters
   */
  // called by other public functions with lock held
  void set_num_logical_blocks_if_larger(uint32_t new_num_blocks) {
    uint32_t old_num_blocks =
        cl2_meta.num_logical_blocks.load(std::memory_order_acquire);
  retry:
    if (unlikely(old_num_blocks >= new_num_blocks)) return;
    if (!cl2_meta.num_logical_blocks.compare_exchange_strong(
            old_num_blocks, new_num_blocks, std::memory_order_acq_rel,
            std::memory_order_acquire))
      goto retry;
    // we do not persist the num_logical_blocks field since it's fine if the
    // value is out-of-date in the worst case, we just do unnecessary fallocate
    VALGRIND_PMC_SET_CLEAN(&cl2_meta.num_logical_blocks,
                           sizeof(cl2_meta.num_logical_blocks));
  }

  [[nodiscard]] uint32_t get_num_logical_blocks() const {
    return cl2_meta.num_logical_blocks.load(std::memory_order_acquire);
  }

  [[nodiscard]] static uint32_t get_tx_seq() { return 0; }

  /**
   * Set the next tx block index
   * No flush+fence but leave it to flush_tx_block
   * @return true on success, false if there is a race condition
   */
  bool try_set_next_tx_block(LogicalBlockIdx block_idx) {
    LogicalBlockIdx expected = 0;
    return cl1_meta.next_tx_block.compare_exchange_strong(
        expected, block_idx, std::memory_order_acq_rel,
        std::memory_order_acquire);
  }

  void set_next_tx_block(LogicalBlockIdx block_idx) {
    cl1_meta.next_tx_block.store(block_idx, std::memory_order_release);
    persist_cl_unfenced(&cl1_meta);
  }

  [[nodiscard]] LogicalBlockIdx get_next_tx_block() const {
    return cl1_meta.next_tx_block.load(std::memory_order_acquire);
  }

  void set_next_orphan_block(LogicalBlockIdx block_idx) {
    cl1_meta.next_orphan_block.store(block_idx, std::memory_order_release);
    persist_cl_unfenced(&cl1_meta);
  }

  [[nodiscard]] LogicalBlockIdx get_next_orphan_block() const {
    return cl1_meta.next_orphan_block.load(std::memory_order_acquire);
  }

  /**
   * Set the flushed tx tail
   * tx_tail is mostly just a hint, so it's fine to be not up-to-date; thus by
   * default, we don't do concurrency control and no fence by default
   *
   * @param tx_tail tail value to set
   * @param fenced whether use fence
   */
  void set_flushed_tx_tail(TxEntryIdx flushed_tx_tail) {
    cl1_meta.flushed_tx_tail.store(flushed_tx_tail, std::memory_order_relaxed);
    VALGRIND_PMC_SET_CLEAN(&cl1_meta.flushed_tx_tail,
                           sizeof(cl1_meta.flushed_tx_tail));
  }

  [[nodiscard]] TxEntryIdx get_flushed_tx_tail() const {
    return cl1_meta.flushed_tx_tail.load(std::memory_order_relaxed);
  }

  /**
   * similar to the one in TxBlock:
   * flush the current block starting from `begin_idx` (including two pointers)
   *
   * @param begin_idx where to start flush
   */
  void flush_tx_block(TxLocalIdx begin_idx = 0) {
    persist_unfenced(&tx_entries[begin_idx],
                     sizeof(TxEntry) * (NUM_INLINE_TX_ENTRY - begin_idx));
    persist_cl_unfenced(cl1);
  }

  /**
   * flush a range of tx entries
   *
   * @param begin_idx
   */
  void flush_tx_entries(TxLocalIdx begin_idx, TxLocalIdx end_idx) {
    persist_unfenced(&tx_entries[begin_idx],
                     sizeof(TxEntry) * (end_idx - begin_idx));
  }

  /*
   * Methods for inline metadata
   */
  [[nodiscard]] TxLocalIdx find_tail(TxLocalIdx hint = 0) const {
    return TxEntry::find_tail<NUM_INLINE_TX_ENTRY>(tx_entries, hint);
  }

  // for garbage collection
  void invalidate_tx_entries() {
    for (auto& entry : tx_entries)
      entry.store(TxEntry::TxEntryDummy, std::memory_order_relaxed);
    persist_fenced(tx_entries, sizeof(TxEntry) * NUM_INLINE_TX_ENTRY);
  }

  friend struct ::madfs::dram::TxCursor;

  friend std::ostream& operator<<(std::ostream& out, const MetaBlock& block) {
    out << "MetaBlock: \n";
    out << "\tnum_logical_blocks: "
        << block.cl2_meta.num_logical_blocks.load(std::memory_order_acquire)
        << "\n";
    out << "\tnext_tx_block: "
        << block.cl1_meta.next_tx_block.load(std::memory_order_acquire) << "\n";
    out << "\ttx_tail: "
        << block.cl1_meta.flushed_tx_tail.load(std::memory_order_acquire)
        << "\n";
    return out;
  }
};

static_assert(sizeof(MetaBlock) == BLOCK_SIZE,
              "MetaBlock must be of size BLOCK_SIZE");

}  // namespace madfs::pmem
