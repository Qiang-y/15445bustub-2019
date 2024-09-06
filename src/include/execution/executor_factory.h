//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// executor_factory.h
//
// Identification: src/include/execution/executor_factory.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>

#include "execution/executors/abstract_executor.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {
/**
 * ExecutorFactory creates executors for arbitrary plan nodes.
 * ExecutorFactory 为任意计划节点创建执行器。
 */
class ExecutorFactory {
 public:
  /**
   * Creates a new executor given the executor context and plan node.
   * @param exec_ctx the executor context for the created executor
   * @param plan the plan node that needs to be executed
   * @return an executor for the given plan and context
   * 给定执行器上下文和计划节点创建一个新的执行器。
   * @param exec_ctx 创建的执行器的执行器上下文
   * @param plan 需要执行的计划节点
   * @return 给定计划和上下文的执行者
   */
  static std::unique_ptr<AbstractExecutor> CreateExecutor(ExecutorContext *exec_ctx, const AbstractPlanNode *plan);
};
}  // namespace bustub
