//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      table_metadata_(nullptr),
      table_iterator_(nullptr, RID(), nullptr) {}

void SeqScanExecutor::Init() {
  table_metadata_ = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
  table_iterator_ = table_metadata_->table_->Begin(exec_ctx_->GetTransaction());
}

bool SeqScanExecutor::Next(Tuple *tuple) {
  if(table_iterator_ == table_metadata_->table_->End()) {
    return false;
  }
  while(table_iterator_ != table_metadata_->table_->End()) {
    Tuple* cur_tuple = table_iterator_.operator->();
    ++table_iterator_;
    if(plan_->GetPredicate()->Evaluate(cur_tuple, plan_->OutputSchema()).GetAs<bool>()) {
      tuple = cur_tuple;
      return true;
    }
  }
  return false;
}

}  // namespace bustub
