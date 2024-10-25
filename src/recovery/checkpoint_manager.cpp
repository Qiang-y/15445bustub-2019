//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// checkpoint_manager.cpp
//
// Identification: src/recovery/checkpoint_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "recovery/checkpoint_manager.h"

namespace bustub {

void CheckpointManager::BeginCheckpoint() {
  // Block all the transactions and ensure that both the WAL and all dirty buffer pool pages are persisted to disk,
  // creating a consistent checkpoint. Do NOT allow transactions to resume at the end of this method, resume them
  // in CheckpointManager::EndCheckpoint() instead. This is for grading purposes.
  // 阻止所有事务并确保 WAL 和所有脏缓冲池页面都持久到磁盘，
  // 创建一致的检查点。不允许事务在此方法结束时恢复，请恢复它们
  // 改为在 CheckpointManager::EndCheckpoint() 中。这是出于评分目的。

  transaction_manager_->BlockAllTransactions();
  log_manager_->Flush();
  buffer_pool_manager_->FlushAllPages();
}

void CheckpointManager::EndCheckpoint() {
  // Allow transactions to resume, completing the checkpoint.

  transaction_manager_->ResumeTransactions();
}

}  // namespace bustub
