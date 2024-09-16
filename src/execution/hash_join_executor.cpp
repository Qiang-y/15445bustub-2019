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
      jht_("LinearProbeHashTable", exec_ctx->GetBufferPoolManager(), jht_comp_, jht_num_buckets_, jht_hash_fn_),
      left_child_executor_(std::move(left)),
      right_child_executor_(std::move(right)){}

/** @return the JHT in use. Do not modify this function, otherwise you will get a zero. */
// Uncomment me! const HT *GetJHT() const { return &jht_; }

void HashJoinExecutor::Init() {
  left_child_executor_->Init();
  right_child_executor_->Init();

  BufferPoolManager * buffer_pool_manager = exec_ctx_->GetBufferPoolManager();
  TmpTuple tmp_tuple;
  page_id_t tmp_tuple_page_id;
  TmpTuplePage* tmp_tuple_page = static_cast<TmpTuplePage*>(buffer_pool_manager->NewPage(&tmp_tuple_page_id));
  tmp_tuple_page->Init(tmp_tuple_page_id, PAGE_SIZE);
  tmp_page_ids_.emplace_back(tmp_tuple_page_id);

  Tuple left_tuple;
  while(left_child_executor_->Next(&left_tuple)) {
    auto hash_value = HashValues(&left_tuple, left_child_executor_->GetOutputSchema(), plan_->GetLeftKeys());
    // jht_.Insert(exec_ctx_->GetTransaction(), hash_value, left_tuple);    //simpleHashTable的

    // 将从下层取得的tuple存入本次创建的tmp_tuple_page中并获取对应的tmp_tuple, 并判断是否插入成功，若失败则新建一个tmp_tuple_page
    if(!tmp_tuple_page->Insert(left_tuple, &tmp_tuple)) {
      buffer_pool_manager->UnpinPage(tmp_tuple_page_id, true);

      tmp_tuple_page = static_cast<TmpTuplePage *>(buffer_pool_manager->NewPage(&tmp_tuple_page_id));
      tmp_tuple_page->Init(tmp_tuple_page_id, PAGE_SIZE);
      tmp_page_ids_.emplace_back(tmp_tuple_page_id);

      tmp_tuple_page->Insert(left_tuple, &tmp_tuple);     // 重新插入
    }

    jht_.Insert(exec_ctx_->GetTransaction(), hash_value, tmp_tuple);
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
  Destory();
  return false;
}

bool HashJoinExecutor::GetNextTuples() {
  ++left_tuple_index_;
  if (left_tuple_index_ >= left_tuples_->size()) {
    left_tuple_index_ = 0;
    left_tuples_->clear();
  }
  // 空表示第一查询或者上次查询的left已经查完，empty表示本次right对应的letft_hash里没有tuple
  while (left_tuples_ == nullptr || left_tuples_->empty()) {
    if (right_child_executor_->Next(right_tuple_) == false) {
      return false;
    }

    auto r_hash_value = HashValues(right_tuple_, right_child_executor_->GetOutputSchema(), plan_->GetRightKeys());
    // 得到和右tuple匹配的左tuples(注意此时拿出的是tmptuple，还要去page里找真正的tuple)
    std::vector<TmpTuple> left_tmp_tuples{};
    jht_.GetValue(exec_ctx_->GetTransaction(), r_hash_value, &left_tmp_tuples);

    auto buffer_pool_manager = exec_ctx_->GetBufferPoolManager();
    for (auto left_tmp_tuple : left_tmp_tuples) {
      TmpTuplePage *tmp_tuple_page =
          static_cast<TmpTuplePage *>(buffer_pool_manager->FetchPage(left_tmp_tuple.GetPageId()));
      auto page_data = tmp_tuple_page->GetData();
      size_t pointer = left_tmp_tuple.GetOffset();

      // 获取tuple
      Tuple tuple;
      auto tuple_data = page_data + pointer;
      tuple.DeserializeFrom(tuple_data);            // 这个函数用来解析出Tuple

      left_tuples_->emplace_back(tuple);
      buffer_pool_manager->UnpinPage(left_tmp_tuple.GetPageId(), false);
    }
  }
  return true;
}

void HashJoinExecutor::Destory(){
  std::thread thread([this] {
    auto buffer_pool_manager = exec_ctx_->GetBufferPoolManager();
    for(auto page_id : tmp_page_ids_) {
      if(buffer_pool_manager->FetchPage(page_id)) {
        buffer_pool_manager->UnpinPage(page_id, false);
        buffer_pool_manager->DeletePage(page_id);
      }
    }
  }
    );
  thread.join();
}

template class LinearProbeHashTable<hash_t, TmpTuple, HashComparator>;
template class HashTableBlockPage<hash_t, TmpTuple, HashComparator>;
}  // namespace bustub
