//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.h
//
// Identification: src/include/execution/executors/insert_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/insert_plan.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * InsertExecutor executes an insert into a table.
 * Inserted values can either be embedded in the plan itself ("raw insert") or come from a child executor.
 * InsertExecutor 执行对表的插入。
 * 插入的值可以嵌入计划本身（“原始插入”）或来自子执行器。
 */
class InsertExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new insert executor.
   * @param exec_ctx the executor context
   * @param plan the insert plan to be executed
   * @param child_executor the child executor to obtain insert values from, can be nullptr
   */
  InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  const Schema *GetOutputSchema() override;

  void Init() override;

  // Note that Insert does not make use of the tuple pointer being passed in.
  // We return false if the insert failed for any reason, and return true if all inserts succeeded.
  // 请注意，插入不使用传入的元组指针。
  // 如果由于任何原因插入失败，我们返回 false，如果所有插入成功，我们返回 true。
  bool Next([[maybe_unused]] Tuple *tuple) override;

  bool RawInsert(Tuple *tuple);

  bool NoRawInsert(Tuple *tuple);
 private:
  /** The insert plan node to be executed. */
  const InsertPlanNode *plan_;
  // 要插入表的元数据
  TableMetadata* table_metadata_;
  // 直接插入/从子计划中插入的标志， true：直接插入 ；false：从子计划
  bool is_raw_insert_;
  // 子查询执行器
  std::unique_ptr<AbstractExecutor> child_executor_;
};
}  // namespace bustub
