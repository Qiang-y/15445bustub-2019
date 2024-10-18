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
  auto src_record = reinterpret_cast<const LogRecord*>(data);

  if(data + src_record->size_ > log_buffer_ + LOG_BUFFER_SIZE || src_record->size_ < 20) {
    return false;
  }

  // copy the header
  memcpy(reinterpret_cast<char*>(log_record), data, LogRecord::HEADER_SIZE);

  // copy the body depend on type
  int pos  = LogRecord::HEADER_SIZE;

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
    default:  break;
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

}

/*
 *undo phase on TABLE PAGE level(table/table_page.h)
 *iterate through active txn map and undo each operation
 */
void LogRecovery::Undo() {}

}  // namespace bustub
