//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// log_recovery.cpp
//
// Identification: src/recovery/log_recovery.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "recovery/log_recovery.h"

#include "storage/page/table_page.h"

namespace bustub {
/*
 * deserialize a log record from log buffer
 * @return: true means deserialize succeed, otherwise can't deserialize cause
 * incomplete log record
 */
bool LogRecovery::DeserializeLogRecord(const char *data, LogRecord *log_record) {
  auto src_record = reinterpret_cast<const LogRecord *>(data);

  if (data + src_record->size_ > log_buffer_ + LOG_BUFFER_SIZE || src_record->size_ < 20) {
    return false;
  }

  // copy the header
  memcpy(reinterpret_cast<char *>(log_record), data, LogRecord::HEADER_SIZE);

  // copy the body depend on type
  int pos = LogRecord::HEADER_SIZE;

  switch (log_record->GetLogRecordType()) {
    case LogRecordType::MARKDELETE:
    case LogRecordType::APPLYDELETE:
    case LogRecordType::ROLLBACKDELETE: {
      memcpy(&log_record->delete_rid_, data + pos, sizeof(RID));
      pos += sizeof(RID);
      log_record->delete_tuple_.DeserializeFrom(data + pos);
      break;
    }
    case LogRecordType::INSERT: {
      memcpy(&log_record->insert_rid_, data + pos, sizeof(RID));
      pos += sizeof(RID);
      log_record->insert_tuple_.DeserializeFrom(data + pos);
      break;
    }
    case LogRecordType::UPDATE: {
      memcpy(&log_record->update_rid_, data + pos, sizeof(RID));
      pos += sizeof(RID);
      log_record->old_tuple_.DeserializeFrom(data + pos);
      pos = pos + log_record->old_tuple_.GetLength() + sizeof(int32_t);
      log_record->new_tuple_.DeserializeFrom(data + pos);
      break;
    }
    case LogRecordType::NEWPAGE: {
      memcpy(&log_record->prev_page_id_, data + pos, sizeof(page_id_t));
      pos += sizeof(page_id_t);
      memcpy(&log_record->page_id_, data + pos, sizeof(page_id_t));
      break;
    }
    case LogRecordType::INVALID: {
      return false;
    }
    default:
      break;
  }
  return true;
}

/*
 *redo phase on TABLE PAGE level(table/table_page.h)
 *read log file from the beginning to end (you must prefetch log records into
 *log buffer to reduce unnecessary I/O operations), remember to compare page's
 *LSN with log_record's sequence number, and also build active_txn_ table &
 *lsn_mapping_ table
 *表页级别的重做阶段(table/table_page.h)
 *从头到尾读取日志文件（必须将日志记录预取到
 *日志缓冲区以减少不必要的I/O操作），记得比较页面的
 *LSN 带有log_record的序号，同时建立active_txn_表&
 *lsn_mapping_表
 */
void LogRecovery::Redo() {
  offset_ = 0;
  while (disk_manager_->ReadLog(log_buffer_, LOG_BUFFER_SIZE, offset_)) {
    int buffer_offset{0};
    LogRecord record;
    while (DeserializeLogRecord(log_buffer_ + buffer_offset, &record)) {
      // 添加lsn位置映射
      lsn_mapping_[record.GetLSN()] = offset_ + buffer_offset;
      // 添加ATT
      active_txn_[record.GetTxnId()] = record.GetLSN();
      buffer_offset += record.GetSize();
      switch (record.GetLogRecordType()) {
        case LogRecordType::INSERT: {
          RedoInsert(record);
          break;
        }
        case LogRecordType::MARKDELETE: {
          RedoMarkDelete(record);
          break;
        }
        case LogRecordType::APPLYDELETE: {
          RedoApplyDelete(record);
          break;
        }
        case LogRecordType::ROLLBACKDELETE: {
          RedoRollbackDelete(record);
          break;
        }
        case LogRecordType::UPDATE: {
          RedoUpdate(record);
          break;
        }
        case LogRecordType::COMMIT:
        case LogRecordType::ABORT: {
          RedoCommitAbort(record);
          break;
        }
        case LogRecordType::NEWPAGE: {
          RedoNewPage(record);
          break;
        }
        default:  break;
      }
    }
    offset_ += buffer_offset;
  }
}

/*
 *undo phase on TABLE PAGE level(table/table_page.h)
 *iterate through active txn map and undo each operation
 */
void LogRecovery::Undo() {}

void LogRecovery::RedoInsert(LogRecord &record) {
  RID rid = record.GetInsertRID();
  page_id_t page_id = rid.GetPageId();
  TablePage * page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->InsertTuple(record.insert_tuple_, &record.insert_rid_, nullptr, nullptr, nullptr);
    page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(page_id, is_dirty);
}

void LogRecovery::RedoMarkDelete(LogRecord &record) {
  RID rid = record.GetDeleteRID();
  page_id_t page_id = rid.GetPageId();
  TablePage * page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->MarkDelete(rid, nullptr, nullptr, nullptr);
    page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(page_id, is_dirty);

}
void LogRecovery::RedoApplyDelete(LogRecord &record) {
  RID rid = record.GetDeleteRID();
  page_id_t page_id = rid.GetPageId();
  TablePage * page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->ApplyDelete(rid, nullptr, nullptr);
    page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(page_id, is_dirty);
}
void LogRecovery::RedoRollbackDelete(LogRecord &record) {
  RID rid = record.GetDeleteRID();
  page_id_t page_id = rid.GetPageId();
  TablePage * page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->RollbackDelete(rid, nullptr, nullptr);
    page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(page_id, is_dirty);
}
void LogRecovery::RedoUpdate(LogRecord &record) {
  RID rid = record.GetDeleteRID();
  page_id_t page_id = rid.GetPageId();
  TablePage * page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->UpdateTuple(record.new_tuple_, &record.old_tuple_, rid, nullptr, nullptr, nullptr);
    page->WUnlatch();
  }
  buffer_pool_manager_->UnpinPage(page_id, is_dirty);
}
void LogRecovery::RedoCommitAbort(LogRecord &record) {
  active_txn_.erase(record.GetTxnId());
}
void LogRecovery::RedoNewPage(LogRecord &record) {
  page_id_t new_page_id = record.page_id_;
  page_id_t prev_page_id = record.prev_page_id_;
  auto page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(new_page_id));
  bool is_dirty{false};
  // 依据page上的最后一次lsn和日记中的lsn比较查看日志之前是否成功
  if(page->GetLSN() < record.GetLSN()) {
    is_dirty = true;
    page->WLatch();
    page->Init(new_page_id, PAGE_SIZE, prev_page_id, nullptr, nullptr);
    page->WUnlatch();

    // 修改上个页面
    if(prev_page_id != INVALID_PAGE_ID) {
      auto prev_page =reinterpret_cast<TablePage*>(buffer_pool_manager_->FetchPage(prev_page_id));
      prev_page->WLatch();
      prev_page->SetNextPageId(new_page_id);
      prev_page->WUnlatch();
      buffer_pool_manager_->UnpinPage(prev_page_id, true);
    }
  }
  buffer_pool_manager_->UnpinPage(new_page_id, is_dirty);
}

}  // namespace bustub
