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
      table_heap_(nullptr),
      table_iterator_(nullptr, RID(), nullptr) {}

void SeqScanExecutor::Init() {
  table_heap_ = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid())->table_.get();
  table_iterator_ = table_heap_->Begin(exec_ctx_->GetTransaction());
}

bool SeqScanExecutor::Next(Tuple *tuple) {
  if(table_iterator_ == table_heap_->End()) {
    return false;
  }
  while(table_iterator_ != table_heap_->End()) {
    ++table_iterator_;
    Tuple* cur_tuple = table_iterator_.operator->();
    if(plan_->GetPredicate()->Evaluate(cur_tuple, plan_->OutputSchema()).GetAs<bool>()) {
      tuple = cur_tuple;
      return true;
    }
  }
  return false;
}

}  // namespace bustub
