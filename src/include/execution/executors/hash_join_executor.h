//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "container/hash/hash_function.h"
#include "container/hash/linear_probe_hash_table.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/index/hash_comparator.h"
#include "storage/table/tmp_tuple.h"
#include "storage/table/tuple.h"

namespace bustub {
/**
 * IdentityHashFunction hashes everything to itself, i.e. h(x) = x.
 * IdentityHashFunction 将所有内容散列到自身，即 h(x) = x。
 */
class IdentityHashFunction : public HashFunction<hash_t> {
 public:
  /**
   * Hashes the key.
   * @param key the key to be hashed
   * @return the hashed value
   */
  uint64_t GetHash(size_t key) override { return key; }
};

/**
 * A simple hash table that supports hash joins.
 */
class SimpleHashJoinHashTable {
 public:
  /** Creates a new simple hash join hash table. */
  SimpleHashJoinHashTable(const std::string &name, BufferPoolManager *bpm, HashComparator cmp, uint32_t buckets,
                          const IdentityHashFunction &hash_fn) {}

  /**
   * Inserts a (hash key, tuple) pair into the hash table.
   * @param txn the transaction that we execute in
   * @param h the hash key
   * @param t the tuple to associate with the key
   * @return true if the insert succeeded
   */
  bool Insert(Transaction *txn, hash_t h, const Tuple &t) {
    hash_table_[h].emplace_back(t);
    return true;
  }

  /**
   * Gets the values in the hash table that match the given hash key.
   * @param txn the transaction that we execute in
   * @param h the hash key
   * @param[out] t the list of tuples that matched the key
   */
  void GetValue(Transaction *txn, hash_t h, std::vector<Tuple> *t) { *t = hash_table_[h]; }

 private:
  std::unordered_map<hash_t, std::vector<Tuple>> hash_table_;
};

// TODO(student): when you are ready to attempt task 3, replace the using declaration!
// 当你准备尝试任务 3 时，请替换掉 using 声明！
using HT = SimpleHashJoinHashTable;

// using HashJoinKeyType = ???;
// using HashJoinValType = ???;
// using HT = LinearProbeHashTable<HashJoinKeyType, HashJoinValType, HashComparator>;

/**
 * HashJoinExecutor executes hash join operations.
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  /**
   * Creates a new hash join executor.
   * @param exec_ctx the context that the hash join should be performed in
   * @param plan the hash join plan node
   * @param left the left child, used by convention to build the hash table
   * @param right the right child, used by convention to probe the hash table
   */
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan, std::unique_ptr<AbstractExecutor> &&left,
                   std::unique_ptr<AbstractExecutor> &&right);

  /** @return the JHT in use. Do not modify this function, otherwise you will get a zero.
   * @return正在使用的JHT。不要修改这个函数，否则你会得到零 */
  // Uncomment me! const HT *GetJHT() const { return &jht_; }
  const HT *GetJHT() const { return &jht_; }

  const Schema *GetOutputSchema() override { return plan_->OutputSchema(); }

  void Init() override;

  bool Next(Tuple *tuple) override;

  /**
   * Hashes a tuple by evaluating it against every expression on the given schema, combining all non-null hashes.
   * @param tuple tuple to be hashed
   * @param schema schema to evaluate the tuple on
   * @param exprs expressions to evaluate the tuple with
   * @return the hashed tuple
   * 通过针对给定架构上的每个表达式对元组进行评估，并组合所有非 null 哈希值，对元组进行哈希处理。
   * @param 要进行哈希处理的元组元组
   * @param schema schema 来评估
   * @param exprs 表达式来计算元组
   * @return哈希元组
   */
  hash_t HashValues(const Tuple *tuple, const Schema *schema, const std::vector<const AbstractExpression *> &exprs) {
    hash_t curr_hash = 0;
    // For every expression,
    for (const auto &expr : exprs) {
      // We evaluate the tuple on the expression and schema.    我们根据表达式和模式评估元组。
      Value val = expr->Evaluate(tuple, schema);
      // If this produces a value,     如果这产生一个值，
      if (!val.IsNull()) {
        // We combine the hash of that value into our current hash.     我们将该值的哈希值合并到当前的哈希值中。
        curr_hash = HashUtil::CombineHashes(curr_hash, HashUtil::HashValue(&val));
      }
    }
    return curr_hash;
  }

  // 工具函数，得到下两条tuple（左和右）
  bool GetNextTuples();
 private:
  /** The hash join plan node. */
  const HashJoinPlanNode *plan_;
  /** The comparator is used to compare hashes.
   * 比较器用于比较哈希值 */
  [[maybe_unused]] HashComparator jht_comp_{};

  /** The identity hash function. */
  IdentityHashFunction jht_hash_fn_{};

  /** The hash table that we are using. */
  // Uncomment me! HT jht_;
 // 取消注释我！ HT jht_；
  HT jht_;
  /** The number of buckets in the hash table. */
  static constexpr uint32_t jht_num_buckets_ = 2;

  std::unique_ptr<AbstractExecutor> left_child_executor_;
  std::unique_ptr<AbstractExecutor> right_child_executor_;

  // 记录当前匹配的左右tuple
  std::vector<Tuple>* left_tuples_{};
  size_t left_tuple_index_{};
  Tuple* right_tuple_{};
};
}  // namespace bustub
