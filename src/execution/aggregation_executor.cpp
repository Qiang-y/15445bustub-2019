//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child)
    : AbstractExecutor(exec_ctx), plan_(plan), child_(std::move(child)),
      aht_(plan->GetAggregates(), plan->GetAggregateTypes()),
      aht_iterator_(aht_.End()){}

const AbstractExecutor *AggregationExecutor::GetChildExecutor() const { return child_.get(); }

const Schema *AggregationExecutor::GetOutputSchema() { return plan_->OutputSchema(); }

void AggregationExecutor::Init() {
  child_->Init();
  Tuple tuple;
  while(child_->Next(&tuple)) {
    const auto tuple_key = MakeKey(&tuple);
    const auto tuple_value = MakeVal(&tuple);
    aht_.InsertCombine(tuple_key, tuple_value);
  }
  aht_iterator_ = aht_.Begin();
}

bool AggregationExecutor::Next(Tuple *tuple) {
  while(aht_iterator_ != aht_.End()) {
    AggregateKey aggregate_key = aht_iterator_.Key();
    AggregateValue aggregate_value = aht_iterator_.Val();
    ++aht_iterator_;

    if(plan_->GetHaving()->EvaluateAggregate(aggregate_key.group_bys_, aggregate_value.aggregates_).GetAs<bool>()) {
      std::vector<Value> values{};
      for(size_t column_it = 0; column_it < plan_->OutputSchema()->GetColumnCount(); ++column_it) {
        const AbstractExpression *column_expr = plan_->OutputSchema()->GetColumn(column_it).GetExpr();
        values.emplace_back(column_expr->EvaluateAggregate(aggregate_key.group_bys_, aggregate_value.aggregates_));
      }
      *tuple = Tuple{values, GetOutputSchema()};
      return true;
    }
  }

  return false;
}

}  // namespace bustub
