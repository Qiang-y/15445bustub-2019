//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// linear_probe_hash_table.cpp
//
// Identification: src/container/hash/linear_probe_hash_table.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "container/hash/linear_probe_hash_table.h"

#include <storage/index/linear_probe_hash_table_index.h>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"

namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_TYPE::LinearProbeHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                      const KeyComparator &comparator, size_t num_buckets,
                                      HashFunction<KeyType> hash_fn)
    : buffer_pool_manager_(buffer_pool_manager), comparator_(comparator), hash_fn_(std::move(hash_fn)) {

  table_latch_.WLock();

  buck_size_ = num_buckets;
  block_size_ = (num_buckets - 1) / BLOCK_ARRAY_SIZE + 1;
  used_size_ = 0;

  auto page = buffer_pool_manager_->NewPage(&header_page_id_);
  auto hash_header = reinterpret_cast<HashTableHeaderPage*>(page->GetData());

  InitHeader(hash_header, 0);

  buffer_pool_manager_->UnpinPage(header_page_id_, true);
  table_latch_.WUnlock();
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) {
  // 先加一个全局的表锁，防止在查询时别的线程触发扩容
  table_latch_.RLock();
  result->clear();
  auto key_slot = hash_fn_.GetHash(key);
  auto key_block_index = key_slot / BLOCK_ARRAY_SIZE;
  auto key_bucket_index = key_slot % BLOCK_ARRAY_SIZE;
  size_t max_step = buck_size_ + key_slot;    // 最多可行进次数，key_slot 需要小于此数

  // 得到Block
  //这么写似乎不能加锁
  // HASH_TABLE_BLOCK_TYPE* block = GetHashBlock(block_page_ids_[key_block_index]);
  auto page = buffer_pool_manager_->FetchPage(block_page_ids_[key_block_index]);
  page->RLatch();
  auto* block = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(page->GetData());

  // 查找下标
  while(block->IsOccupied(key_bucket_index) && key_slot < max_step) {
    // 可读且key相同
    if(block->IsReadable(key_bucket_index) && !comparator_(key, block->KeyAt(key_bucket_index))) {
      result->emplace_back(block->ValueAt(key_bucket_index));
    }
    // 更新下标
    ++key_slot;
    UpdateIndex(key_block_index, key_bucket_index, page, block, false);
  }
  page->RUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetPageId(), false);

  table_latch_.RUnlock();
  return !result->empty();
}
/*****************************************************************************
 * INSERTION
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Insert(Transaction *transaction, const KeyType &key, const ValueType &value) {
  // 为了在操作block中数据时表结构不会更改，所以用table的读锁
  table_latch_.RLock();

  auto success = InsertImpl(transaction, key, value);

  table_latch_.RUnlock();
  // 如果成功则增加used_bucket
  if(success) {
    ++used_size_;
    auto page = buffer_pool_manager_->FetchPage(header_page_id_);
    auto header = reinterpret_cast<HashTableHeaderPage*>(page->GetData());
    page->WLatch();
    header->SetUsedSize(used_size_.load());
    page->WUnlatch();
    buffer_pool_manager_->UnpinPage(header_page_id_, true);
  }
  // 负载因子大于设置值时需要扩容
  if(static_cast<float>(used_size_) / buck_size_ > HASH_LOAD_FACTOR) {
    Resize(buck_size_);
  }
  return success;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Remove(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.RLock();
  auto key_slot = hash_fn_.GetHash(key);
  auto key_block_index = key_slot / BLOCK_ARRAY_SIZE;
  auto key_bucket_index = key_slot % BLOCK_ARRAY_SIZE;
  size_t max_step = buck_size_ + key_slot;    // 最多可行进次数，key_slot 需要小于此数
  bool success = false;

  auto page = buffer_pool_manager_->FetchPage(block_page_ids_[key_block_index]);
  page->WLatch();
  auto* block = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(page->GetData());
  while(block->IsOccupied(key_bucket_index) && key_slot < max_step) {
    if(block->IsReadable(key_bucket_index) &&
       !comparator_(key, block->KeyAt(key_bucket_index)) &&
       block->ValueAt(key_bucket_index) == value) {
      success = true;
      break;
    }
    // 更新下标
    ++key_slot;
    UpdateIndex(key_block_index, key_bucket_index, page, block, true);
  }
  // 成功找到位置则删除
  if(success)
    block->Remove(key_bucket_index);

  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetPageId(), true);
  table_latch_.RUnlock();
  return success;
}

/*****************************************************************************
 * RESIZE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::Resize(size_t initial_size) {
  table_latch_.WLock();
  // 在等待锁的时候已被别的线程扩容
  if(buck_size_ != initial_size && static_cast<float>(used_size_.load()) < buck_size_ * HASH_LOAD_FACTOR) {
    table_latch_.WUnlock();
    return;
  }
  // 扩容
  buck_size_ = initial_size * 2;
  block_size_ = (buck_size_ - 1) / BLOCK_ARRAY_SIZE + 1;

  // 备份旧数据
  std::vector<page_id_t> old_block_page_ids(block_page_ids_);
  auto old_header_page_id = header_page_id_;

  // 删除旧header
  buffer_pool_manager_->DeletePage(old_header_page_id);

  // 得到新header
  page_id_t new_header_id;
  auto new_header_page = buffer_pool_manager_->NewPage(&new_header_id);
  new_header_page->WLatch();
  auto new_header = reinterpret_cast<HashTableHeaderPage*>(new_header_page->GetData());
  header_page_id_ = new_header_id;
  // 初始化新header
  InitHeader(new_header, used_size_);

  for(auto& block_it : old_block_page_ids) {
    // 获取旧的block
    auto old_block_page = buffer_pool_manager_->FetchPage(block_it);
    old_block_page->RLatch();
    auto old_block = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(old_block_page->GetData());

    // 对每个元素重新插入新表
    for(size_t bucket_it = 0; bucket_it < BLOCK_ARRAY_SIZE; ++bucket_it) {
      if(old_block->IsReadable(bucket_it)) {
        InsertImpl(nullptr, old_block->KeyAt(bucket_it), old_block->ValueAt(bucket_it));
      }
    }

    // 删除旧block
    old_block_page->RUnlatch();
    buffer_pool_manager_->UnpinPage(block_it, false);
    buffer_pool_manager_->DeletePage(block_it);
  }

  new_header_page->WUnlatch();
  buffer_pool_manager_->UnpinPage(new_header_id, true);
  table_latch_.WUnlock();
}

/*****************************************************************************
 * GETSIZE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
size_t HASH_TABLE_TYPE::GetSize() {
  return buck_size_;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void LinearProbeHashTable<KeyType, ValueType, KeyComparator>::InitHeader(HashTableHeaderPage *hash_header, size_t used_size) {
  assert(hash_header != nullptr);
  hash_header->SetSize(buck_size_);
  hash_header->SetPageId(header_page_id_);
  hash_header->SetUsedSize(used_size);
  block_page_ids_.clear();
  for (size_t i = 0; i < buck_size_; ++i) {
    page_id_t page_id_temp;
    buffer_pool_manager_->NewPage(&page_id_temp);
    hash_header->AddBlockPageId(page_id_temp);
    block_page_ids_.emplace_back(page_id_temp);
    buffer_pool_manager_->UnpinPage(page_id_temp, false);
  }
}
template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_BLOCK_TYPE*
LinearProbeHashTable<KeyType, ValueType, KeyComparator>::GetHashBlock(page_id_t page_id) {
  auto page = buffer_pool_manager_->FetchPage(page_id);
  buffer_pool_manager_->UnpinPage(page_id, false);
  return reinterpret_cast<HASH_TABLE_BLOCK_TYPE *>(page->GetData());
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool LinearProbeHashTable<KeyType, ValueType, KeyComparator>::InsertImpl(Transaction *transaction, const KeyType &key,
                                                                         const ValueType &value) {
  auto key_slot = hash_fn_.GetHash(key);
  auto key_block_index = key_slot / BLOCK_ARRAY_SIZE;
  auto key_bucket_index = key_slot % BLOCK_ARRAY_SIZE;
  bool success = true;
  size_t max_step = buck_size_ + key_slot;  // 最多可行进次数，key_slot 需要小于此数
  auto page = buffer_pool_manager_->FetchPage(block_page_ids_[key_block_index]);
  // 写锁
  page->WLatch();
  auto *block = reinterpret_cast<HASH_TABLE_BLOCK_TYPE *>(page->GetData());

  // 不断尝试插入，直到插入成功，或者失败（没有空位 | 已经有相同的KV了）
  while (!block->Insert(key_bucket_index, key, value)) {
    if (!comparator_(key, block->KeyAt(key_bucket_index)) && block->ValueAt(key_bucket_index) == value) {
      success = false;
      break;
    }
    // 更新下标
    UpdateIndex(key_block_index, key_bucket_index, page, block, false);

    //* 目前认为不用在这里扩，可以在别的地方设计 *//
    // // 又回到起点，说明要进行扩容
    if (++key_slot == max_step) {
      success = false;
      break;
    }
  }
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetPageId(), true);
  return success;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void LinearProbeHashTable<KeyType, ValueType, KeyComparator>::UpdateIndex(
  size_t &block_index, size_t &bucket_index, Page *page,
  HashTableBlockPage<KeyType, ValueType, KeyComparator> *block,
  bool WR_flag){

  if(++bucket_index == BLOCK_ARRAY_SIZE) {
    bucket_index = 0;
    if(++block_index >= buck_size_) {
      block_index = 0;
    }

    if(WR_flag == true)
      page->WUnlatch();
    else if(WR_flag == false)
      page->RUnlatch();

    buffer_pool_manager_->UnpinPage(page->GetPageId(), false);

    // 得到新的block
    // block = GetHashBlock(key_block_index);
    page = buffer_pool_manager_->FetchPage(block_page_ids_[block_index]);

    if(WR_flag == true)
      page->WLatch();
    else if(WR_flag == false)
      page->RLatch();

    block = reinterpret_cast<HASH_TABLE_BLOCK_TYPE*>(page->GetData());
    assert(block != nullptr);     // 检查block， 同时（主要）为了防止编译器报错，因为block重新赋值后没有使用，而---Werror会让编译器认为这是错误
  }
}

template class LinearProbeHashTable<int, int, IntComparator>;

template class LinearProbeHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class LinearProbeHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class LinearProbeHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class LinearProbeHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class LinearProbeHashTable<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
