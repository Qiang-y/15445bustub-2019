//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// log_manager.cpp
//
// Identification: src/recovery/log_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "recovery/log_manager.h"

#include <common/logger.h>

namespace bustub {
/*
 * set enable_logging = true
 * Start a separate thread to execute flush to disk operation periodically
 * The flush can be triggered when timeout or the log buffer is full or buffer
 * pool manager wants to force flush (it only happens when the flushed page has
 * a larger LSN than persistent LSN)
 *
 * This thread runs forever until system shutdown/StopFlushThread
 * 设置enable_logging = true
 * 启动一个单独的线程定期执行flush到磁盘操作
 * 当超时 或者日志缓冲区满 或者缓冲池管理器想要强制刷新（仅当要刷新的page的page_LSN比persistent_LSN大） 时可以触发flush
 *
 * 该线程永远运行直到系统关闭/StopFlushThread
 */
void LogManager::RunFlushThread() {
  if (enable_logging) return;
  enable_logging = true;

  flush_thread_ =
      new std::thread(&LogManager::FlushTask, this);  // 在这里新线程就开始执行了（thread构造函数新建即执行）
  // flush_thread_->detach();
}

// 后台刷新任务
void LogManager::FlushTask() {
  while (enable_logging) {
    // 获取latch所有权，休眠等待条件满足被重新唤醒，休眠时释放latch
    std::unique_lock<std::mutex> lock(this->latch_);
    cv_.wait_for(lock, std::chrono::seconds(log_timeout), [&] { return need_flush_.load(); });
    if (!enable_logging) {
      LOG_INFO("enable_logging is false");
      need_flush_ = false;
      cv_respond_.notify_all();
      return;
    }  // 当条件满足时线程唤醒重新取得latch所有权
    // 判断是否有新log被写入
    if (log_buffer_offset_ > 0) {
      // 交换两个buff
      std::swap(flush_buffer_, log_buffer_);
      std::swap(flush_buffer_offset_, log_buffer_offset_);
      disk_manager_->WriteLog(flush_buffer_, flush_buffer_offset_);
      flush_buffer_offset_ = 0;
      SetPersistentLSN(next_lsn_ - 1);
      // is_running_.set_value(false);
    }
    need_flush_ = false;
    cv_respond_.notify_all();
  }
}

void LogManager::Flush() {
  std::unique_lock<std::mutex> lock(latch_);
  Flush(lock);
}

// 唤醒后台进程
void LogManager::Flush(std::unique_lock<std::mutex> &lock) {
  need_flush_ = true;
  cv_.notify_one();
  cv_respond_.wait(lock, [&] { return !need_flush_.load(); });
}

/*
 * Stop and join the flush thread, set enable_logging = false
 */
void LogManager::StopFlushThread() {
  std::unique_lock<std::mutex> lock(latch_);
  enable_logging = false;
  Flush(lock);
  LOG_INFO("end last Flush");
  // lock.unlock();
  flush_thread_->join();  // 阻塞当前线程直到join的线程返回
  // lock.lock();
  LOG_INFO("end join");
  delete flush_thread_;
  flush_thread_ = nullptr;
}

/*
 * append a log record into log buffer
 * you MUST set the log record's lsn within this method
 * @return: lsn that is assigned to this log record
 *
 *
 * example below
 * // First, serialize the must have fields(20 bytes in total)
 * log_record.lsn_ = next_lsn_++;
 * memcpy(log_buffer_ + offset_, &log_record, 20);
 * int pos = offset_ + 20;
 *
 * if (log_record.log_record_type_ == LogRecordType::INSERT) {
 *    memcpy(log_buffer_ + pos, &log_record.insert_rid_, sizeof(RID));
 *    pos += sizeof(RID);
 *    // we have provided serialize function for tuple class
 *    log_record.insert_tuple_.SerializeTo(log_buffer_ + pos);
 *  }
 *将日志记录追加到日志缓冲区中
 * 您必须在此方法中设置日志记录的lsn
 * @return: 分配给该日志记录的lsn
 *
 *
 * 下面的例子
 * // 首先，序列化必须有的字段（共20字节）
 * log_record.lsn_ = next_lsn_++;
 * memcpy(log_buffer_ + offset_, &log_record, 20);
 * int pos = offset_ + 20;
 *
 * if (log_record.log_record_type_ == LogRecordType::INSERT) {
 * memcpy(log_buffer_ + pos, &log_record.insert_rid_, sizeof(RID));
 * pos += sizeof(RID);
 * // 我们为元组类提供了序列化函数
 * log_record.insert_tuple_.SerializeTo(log_buffer_ + pos);
 * }
 */
lsn_t LogManager::AppendLogRecord(LogRecord *log_record) {
  std::unique_lock<std::mutex> lock(latch_);
  if (log_buffer_offset_ + log_record->GetSize() > LOG_BUFFER_SIZE) {
    Flush(lock);
  }
  log_record->lsn_ = next_lsn_++;
  // 写入头部信息
  memcpy(log_buffer_ + log_buffer_offset_, &log_record, bustub::LogRecord::HEADER_SIZE);
  int pos = log_buffer_offset_ + bustub::LogRecord::HEADER_SIZE;

  // 根据log类型不同写入不同的log
  switch (log_record->GetLogRecordType()) {
    case LogRecordType::MARKDELETE:
    case LogRecordType::APPLYDELETE:
    case LogRecordType::ROLLBACKDELETE: {
      memcpy(log_buffer_ + pos, &log_record->delete_rid_, sizeof(RID));
      pos += sizeof(RID);
      log_record->delete_tuple_.SerializeTo(log_buffer_ + pos);
      break;
    }
    case LogRecordType::INSERT: {
      memcpy(log_buffer_ + pos, &log_record->insert_rid_, sizeof(RID));
      pos += sizeof(RID);
      log_record->insert_tuple_.SerializeTo(log_buffer_ + pos);
      break;
    }
    case LogRecordType::UPDATE: {
      memcpy(log_buffer_ + pos, &log_record->update_rid_, sizeof(RID));
      pos += sizeof(RID);
      log_record->old_tuple_.SerializeTo(log_buffer_ + pos);
      pos = pos + 4 + log_record->old_tuple_.GetLength();
      log_record->new_tuple_.SerializeTo(log_buffer_ + pos);
      break;
    }
    case LogRecordType::NEWPAGE: {
      memcpy(log_buffer_ + pos, &log_record->prev_page_id_, sizeof(page_id_t));
      pos += sizeof(page_id_t);
      memcpy(log_buffer_ + pos, &log_record->page_id_, sizeof(page_id_t));
      break;
    }
    // BEGIN/COMMIT/ABORT 只有log头没有log体
    default: break;
  }

  // 移动log_offset
  log_buffer_offset_ += log_record->GetSize();
  return log_record->lsn_;
}



}  // namespace bustub
