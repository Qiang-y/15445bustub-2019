//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.h
//
// Identification: src/include/execution/executors/seq_scan_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * SeqScanExecutor executes a sequential scan over a table.
 * SeqScanExecutor 对表执行顺序扫描。
 */
class SeqScanExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new sequential scan executor.
   * @param exec_ctx the executor context
   * @param plan the sequential scan plan to be executed
   * 创建一个新的顺序扫描执行器。
   * @param exec_ctx 执行器上下文
   * @param plan 要执行的顺序扫描计划
   */
  SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan);

  void Init() override;

  bool Next(Tuple *tuple) override;

  const Schema *GetOutputSchema() override { return plan_->OutputSchema(); }

 private:
  /** The sequential scan plan node to be executed.
   * 要执行的顺序扫描计划节点 */
  const SeqScanPlanNode *plan_;

  // 要查询的表
  TableHeap* table_heap_;
  // 迭代器
  TableIterator table_iterator_;
};
}  // namespace bustub
