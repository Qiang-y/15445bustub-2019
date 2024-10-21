//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// log_manager.h
//
// Identification: src/include/recovery/log_manager.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <common/logger.h>

#include <algorithm>
#include <condition_variable>  // NOLINT
#include <future>              // NOLINT
#include <mutex>               // NOLINT

#include "recovery/log_record.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

/**
 * LogManager maintains a separate thread that is awakened whenever the log buffer is full or whenever a timeout
 * happens. When the thread is awakened, the log buffer's content is written into the disk log file.
 * LogManager维护一个单独的线程，每当日志缓冲区发生已满或超时时，该线程就会被唤醒。
 * 当线程被唤醒时，日志缓冲区的内容被写入磁盘日志文件。
 */
class LogManager {
 public:
  explicit LogManager(DiskManager *disk_manager)
      : next_lsn_(0), persistent_lsn_(INVALID_LSN), disk_manager_(disk_manager) {
    log_buffer_ = new char[LOG_BUFFER_SIZE];
    flush_buffer_ = new char[LOG_BUFFER_SIZE];
  }

  ~LogManager() {
    LOG_INFO("begin end logmanager");
    delete[] log_buffer_;
    delete[] flush_buffer_;
    log_buffer_ = nullptr;
    flush_buffer_ = nullptr;
    std::cout <<" end logmanager" << std::endl;
  }

  void RunFlushThread();
  void StopFlushThread();

  lsn_t AppendLogRecord(LogRecord *log_record);

  inline lsn_t GetNextLSN() { return next_lsn_; }
  inline lsn_t GetPersistentLSN() { return persistent_lsn_; }
  inline void SetPersistentLSN(lsn_t lsn) { persistent_lsn_ = lsn; }
  inline char *GetLogBuffer() { return log_buffer_; }

  // 强制执行对外使用
  void Flush();
 private:
  // TODO(students): you may add your own member variables
  void FlushTask();
  // 强制执行
  void Flush(std::unique_lock<std::mutex> &lock);

  // 标志是否要flush
  std::atomic_bool need_flush_{false};
  // 页面上目前有的记录位置
  size_t log_buffer_offset_{0};
  size_t flush_buffer_offset_{0};

  /** The atomic counter which records the next log sequence number.
   * 记录下一个日志序列号的原子计数器。
   */
  std::atomic<lsn_t> next_lsn_;
  /** The log records before and including the persistent lsn have been written to disk.
   *  persisten_lsn 之前（包括持久 lsn）的日志记录已写入磁盘。
   */
  std::atomic<lsn_t> persistent_lsn_;

  char *log_buffer_;
  char *flush_buffer_;

  std::mutex latch_;

  std::thread *flush_thread_ __attribute__((__unused__));

  std::condition_variable cv_;

  std::condition_variable cv_respond_;  // 控制后台线程回复

  DiskManager *disk_manager_ __attribute__((__unused__));
};

}  // namespace bustub
