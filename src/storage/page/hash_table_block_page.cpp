//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_block_page.cpp
//
// Identification: src/storage/page/hash_table_block_page.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/hash_table_block_page.h"

#include <storage/index/hash_comparator.h>
#include <storage/table/tmp_tuple.h>

#include "storage/index/generic_key.h"

namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
KeyType HASH_TABLE_BLOCK_TYPE::KeyAt(slot_offset_t bucket_ind) const {
  return array_[bucket_ind].first;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
ValueType HASH_TABLE_BLOCK_TYPE::ValueAt(slot_offset_t bucket_ind) const {
  return array_[bucket_ind].second;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BLOCK_TYPE::Insert(slot_offset_t bucket_ind, const KeyType &key, const ValueType &value) {

  auto temp_index = bucket_ind / 8;
  auto temp_offset = 0x1 << (bucket_ind % 8);     // 提取需要的bit位

  char old_readable = readable_[temp_index].load();
  size_t new_readable = old_readable | temp_offset;

  // 目标位置已被占据并且是可读的，意为插入失败
  do{
    if(old_readable & temp_offset){
      return false;
    }
  }while(
    !readable_[temp_index].compare_exchange_weak(old_readable, new_readable)
  );

  auto old_occupied = occupied_[temp_index].load();
  auto new_occupied = old_occupied | temp_offset;

  array_[bucket_ind].first = key;
  array_[bucket_ind].second = value;
  occupied_[temp_index].compare_exchange_strong(old_occupied, new_occupied);

  return true;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BLOCK_TYPE::Remove(slot_offset_t bucket_ind) {
  auto temp_index = bucket_ind / 8;
  auto temp_offset = 0x1 << (bucket_ind % 8);

  auto old_readable = readable_[temp_index].load();
  auto new_readable = old_readable & ~temp_offset;
  readable_[temp_index].compare_exchange_strong(old_readable, new_readable);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BLOCK_TYPE::IsOccupied(slot_offset_t bucket_ind) const {
  auto temp_index = bucket_ind / 8;
  auto temp_offset = 0x1 << (bucket_ind % 8);
  return occupied_[temp_index] & temp_offset;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BLOCK_TYPE::IsReadable(slot_offset_t bucket_ind) const {
  auto temp_index = bucket_ind / 8;
  auto temp_offset = 0x1 << (bucket_ind % 8);
  return readable_[temp_index] & temp_offset;
}

// DO NOT REMOVE ANYTHING BELOW THIS LINE
// 请勿删除该线以下的任何内容
template class HashTableBlockPage<int, int, IntComparator>;
template class HashTableBlockPage<GenericKey<4>, RID, GenericComparator<4>>;
template class HashTableBlockPage<GenericKey<8>, RID, GenericComparator<8>>;
template class HashTableBlockPage<GenericKey<16>, RID, GenericComparator<16>>;
template class HashTableBlockPage<GenericKey<32>, RID, GenericComparator<32>>;
template class HashTableBlockPage<GenericKey<64>, RID, GenericComparator<64>>;

// 模板特例化
template class HashTableBlockPage<hash_t, TmpTuple, HashComparator>;
}  // namespace bustub
