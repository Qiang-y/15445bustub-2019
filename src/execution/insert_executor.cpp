//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  is_raw_insert_ = plan->IsRawInsert();
  table_metadata_ = exec_ctx->GetCatalog()->GetTable(plan->TableOid());
}

const Schema *InsertExecutor::GetOutputSchema() { return plan_->OutputSchema(); }

void InsertExecutor::Init() {}

bool InsertExecutor::Next([[maybe_unused]] Tuple *tuple) {
  if(is_raw_insert_)
    return RawInsert(tuple);
  else
    return NoRawInsert(tuple);
}

bool InsertExecutor::RawInsert(Tuple *tuple) {
  auto raw_insert_values = plan_->RawValues();
  for (const auto &insert_values_it: raw_insert_values) {
    Tuple new_tuple(insert_values_it, &table_metadata_->schema_);
    RID rid_t;
    // 这里目前感觉插入返回的Rid要保存到插入的tuple里面保存起来，但是Tuple没有给出设置Rid的api
    if(table_metadata_->table_->InsertTuple(new_tuple, &rid_t, exec_ctx_->GetTransaction()) == false) {
      return false;
    }
  }
  return true;
}

bool InsertExecutor::NoRawInsert(Tuple *tuple) {
  child_executor_->Init();
  RID rid_t;
  while(child_executor_->Next(tuple)) {
    if(table_metadata_->table_->InsertTuple(*tuple, &rid_t, exec_ctx_->GetTransaction()) == false) {
      return false;
    }
  }
  return true;
}

}  // namespace bustub
