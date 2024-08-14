//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_block_page.h
//
// Identification: src/include/storage/page/hash_table_block_page.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <atomic>
#include <utility>
#include <vector>

#include "common/config.h"
#include "storage/index/int_comparator.h"
#include "storage/page/hash_table_page_defs.h"

namespace bustub {
/**
 * Store indexed key and and value together within block page. Supports
 * non-unique keys.
 *
 * Block page format (keys are stored in order):
 *  ----------------------------------------------------------------
 * | KEY(1) + VALUE(1) | KEY(2) + VALUE(2) | ... | KEY(n) + VALUE(n)
 *  ----------------------------------------------------------------
 *
 *  Here '+' means concatenation.
 * 
 * 将索引键和值一起存储在块页内。支持
 * 非唯一键。
 *
 * 块页格式（密钥按顺序存储）：
 * ------------------------------------------------- ----------------
 * |键(1) + 值(1) |键(2) + 值(2) | ... |键(n) + 值(n)
 * ------------------------------------------------- ----------------
 *
 * 这里'+'表示连接。
 *
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
class HashTableBlockPage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  HashTableBlockPage() = delete;

  /**
   * Gets the key at an index in the block.
   *
   * @param bucket_ind the index in the block to get the key at
   * @return key at index bucket_ind of the block
   * 
   * 获取块中索引处的键。
   * @parambucket_ind 块中获取密钥的索引
   * @return key位于块的索引bucket_ind处
   */
  KeyType KeyAt(slot_offset_t bucket_ind) const;

  /**
   * Gets the value at an index in the block.
   *
   * @param bucket_ind the index in the block to get the value at
   * @return value at index bucket_ind of the block
   * 
   * 获取块中索引处的值。
   * @parambucket_ind 块中获取值的索引
   * @return 块索引bucket_ind处的值
   */
  ValueType ValueAt(slot_offset_t bucket_ind) const;

  /**
   * Attempts to insert a key and value into an index in the block.
   * The insert is thread safe. It uses compare and swap to claim the index,
   * and then writes the key and value into the index, and then marks the
   * index as readable.
   *
   * @param bucket_ind index to write the key and value to
   * @param key key to insert
   * @param value value to insert
   * @return If the value is inserted successfully, it returns true. If the
   * index is marked as occupied before the key and value can be inserted,
   * Insert returns false.
   * 
   * 尝试将键和值插入到块中的索引中。
   * insert是线程安全的。它使用比较和交换（CAS）来声明索引，
   * 然后将key和value写入索引，然后标记
   * 索引可读。
   *
   * @param bucket_ind 用于写入键和值的索引
   * @param key 要插入的键
   * @param value 要插入的值
   * @return 如果值插入成功，则返回true。如果在可以插入键和值之前，索引被标记为已占用，
   * 插入返回 false。
   */
  bool Insert(slot_offset_t bucket_ind, const KeyType &key, const ValueType &value);

  /**
   * Removes a key and value at index.
   *
   * @param bucket_ind ind to remove the value
   * 
   * 在索引处删除键和值。
   *
   * @param bucket_ind 删除值的index
   */
  void Remove(slot_offset_t bucket_ind);

  /**
   * Returns whether or not an index is occupied (key/value pair or tombstone)
   *
   * @param bucket_ind index to look at
   * @return true if the index is occupied, false otherwise
   */
  bool IsOccupied(slot_offset_t bucket_ind) const;

  /**
   * Returns whether or not an index is readable (valid key/value pair)
   *
   * @param bucket_ind index to look at
   * @return true if the index is readable, false otherwise
   */
  bool IsReadable(slot_offset_t bucket_ind) const;

 private:
  std::atomic_char occupied_[(BLOCK_ARRAY_SIZE - 1) / 8 + 1];

  // 0 if tombstone/brand new (never occupied), 1 otherwise.
  std::atomic_char readable_[(BLOCK_ARRAY_SIZE - 1) / 8 + 1];
  MappingType array_[0];

};

}  // namespace bustub
