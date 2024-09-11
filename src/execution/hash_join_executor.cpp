//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/hash_join_executor.h"

#include <memory>
#include <vector>

// #include "../../cmake-build-debug/googletest-src/googletest/include/gtest/gtest-param-test.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left, std::unique_ptr<AbstractExecutor> &&right)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      jht_("SimpleHashTable", exec_ctx->GetBufferPoolManager(), jht_comp_, jht_num_buckets_, jht_hash_fn_),
      left_child_executor_(std::move(left)),
      right_child_executor_(std::move(right)){}

/** @return the JHT in use. Do not modify this function, otherwise you will get a zero. */
// Uncomment me! const HT *GetJHT() const { return &jht_; }

void HashJoinExecutor::Init() {
  left_child_executor_->Init();
  right_child_executor_->Init();
  Tuple left_tuple;
  while(left_child_executor_->Next(&left_tuple)) {
    auto hash_value = HashValues(&left_tuple, left_child_executor_->GetOutputSchema(), plan_->GetLeftKeys());
    jht_.Insert(exec_ctx_->GetTransaction(), hash_value, left_tuple);
  }
}

bool HashJoinExecutor::Next(Tuple *tuple) {
  while (GetNextTuples()) {
    Tuple *left_tuple = &(*left_tuples_)[left_tuple_index_];
    auto predicate = plan_->Predicate();

    if (predicate
            ->EvaluateJoin(left_tuple, left_child_executor_->GetOutputSchema(), right_tuple_,
                           right_child_executor_->GetOutputSchema())
            .GetAs<bool>()) {
      std::vector<Value> values{};
      // 对要输出的每个列进行join评估，得到合适的value
      for (size_t column_idx = 0; column_idx < plan_->OutputSchema()->GetColumnCount(); ++column_idx) {
        auto column_expression = plan_->OutputSchema()->GetColumn(column_idx).GetExpr();
        auto join_column_value =
            column_expression->EvaluateJoin(left_tuple, left_child_executor_->GetOutputSchema(), right_tuple_,
                                            right_child_executor_->GetOutputSchema());
        values.emplace_back(join_column_value);
      }
      // 生成可用于输出的tuple
      *tuple = Tuple(values, plan_->OutputSchema());
      return true;
    }
  }
  return false;
}

bool HashJoinExecutor::GetNextTuples(){
  ++left_tuple_index_;
  if(left_tuple_index_ >= left_tuples_->size()) {
    left_tuple_index_ = 0;
    left_tuples_ = nullptr;
  }
  // 空表示第一查询或者上次查询的left已经查完，empty表示本次right对应的letft_hash里没有tuple
  while (left_tuples_ == nullptr || left_tuples_->empty()) {
    if (right_child_executor_->Next(right_tuple_) == false) {
      return false;
    }
    auto r_hash_value = HashValues(right_tuple_, right_child_executor_->GetOutputSchema(), plan_->GetRightKeys());
    // 得到和右tuple匹配的左tuples
    jht_.GetValue(exec_ctx_->GetTransaction(), r_hash_value, left_tuples_);
  }
  return true;
}
}  // namespace bustub
