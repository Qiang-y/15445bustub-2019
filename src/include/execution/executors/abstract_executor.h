//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// abstract_executor.h
//
// Identification: src/include/execution/executors/abstract_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include "execution/executor_context.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * AbstractExecutor implements the Volcano tuple-at-a-time iterator model.
 */
class AbstractExecutor {
 public:
  /**
   * Constructs a new AbstractExecutor.
   * @param exec_ctx the executor context that the executor runs with
   */
  explicit AbstractExecutor(ExecutorContext *exec_ctx) : exec_ctx_{exec_ctx} {}

  /** Virtual destructor. */
  virtual ~AbstractExecutor() = default;

  /**
   * Initializes this executor.
   * @warning This function must be called before Next() is called!
   * 初始化该执行器。
   * @warning 该函数必须在调用 Next() 之前调用！
   */
  virtual void Init() = 0;

  /**
   * Produces the next tuple from this executor.
   * @param[out] tuple the next tuple produced by this executor
   * @return true if a tuple was produced, false if there are no more tuples
   * 从该执行器生成下一个元组。
   * @param[out] tuple 该执行器生成的下一个元组
   * @return true 如果生成了一个元组，如果没有更多的元组则返回 false
   */
  virtual bool Next(Tuple *tuple) = 0;

  /** @return the schema of the tuples that this executor produces */
  virtual const Schema *GetOutputSchema() = 0;

  /** @return the executor context in which this executor runs */
  ExecutorContext *GetExecutorContext() { return exec_ctx_; }

 protected:
  ExecutorContext *exec_ctx_;
};
}  // namespace bustub
