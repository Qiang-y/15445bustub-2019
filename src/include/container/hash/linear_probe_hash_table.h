//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// linear_probe_hash_table.h
//
// Identification: src/include/container/hash/linear_probe_hash_table.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <queue>
#include <string>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "concurrency/transaction.h"
#include "container/hash/hash_function.h"
#include "container/hash/hash_table.h"
#include "storage/page/hash_table_block_page.h"
#include "storage/page/hash_table_header_page.h"
#include "storage/page/hash_table_page_defs.h"

namespace bustub {

#define HASH_TABLE_TYPE LinearProbeHashTable<KeyType, ValueType, KeyComparator>

/**
 * Implementation of linear probing hash table that is backed by a buffer pool
 * manager. Non-unique keys are supported. Supports insert and delete. The
 * table dynamically grows once full.
 *
 * 实现由缓冲池支持的线性探测哈希表.
 * 支持非唯一键。支持插入和删除。
 * table一旦满就动态增长。
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
class LinearProbeHashTable : public HashTable<KeyType, ValueType, KeyComparator> {
 public:
  /**
   * Creates a new LinearProbeHashTable
   *
   * @param buffer_pool_manager buffer pool manager to be used
   * @param comparator comparator for keys
   * @param num_buckets initial number of buckets contained by this hash table
   * @param hash_fn the hash function
   *
   * 创建一个新的 LinearProbeHashTable
   * @param buffer_pool_manager 要使用的缓冲池管理器
   * @param comparator 键的比较器
   * @param num_buckets 该哈希表包含的初始桶数
   * @param hash_fn 哈希函数
   */
  explicit LinearProbeHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                const KeyComparator &comparator, size_t num_buckets, HashFunction<KeyType> hash_fn);

  /**
   * Inserts a key-value pair into the hash table.
   * @param transaction the current transaction
   * @param key the key to create
   * @param value the value to be associated with the key
   * @return true if insert succeeded, false otherwise
   *
   * 将键值对插入哈希表。
   * @param transaction 当前交易
   * @param key 要创建的密钥
   * @param value 与键关联的值
   * @如果插入成功则返回 true，否则返回 false
   */
  bool Insert(Transaction *transaction, const KeyType &key, const ValueType &value) override;

  /**
   * Deletes the associated value for the given key.
   * @param transaction the current transaction
   * @param key the key to delete
   * @param value the value to delete
   * @return true if remove succeeded, false otherwise
   *
   * 删除给定键的关联值。
   * @param transaction 当前交易
   * @param key 要删除的键
   * @param value 要删除的值
   * @remove 成功则返回 true，否则返回 false
   */
  bool Remove(Transaction *transaction, const KeyType &key, const ValueType &value) override;

  /**
   * Performs a point query on the hash table.
   * @param transaction the current transaction
   * @param key the key to look up
   * @param[out] result the value(s) associated with a given key
   * @return the value(s) associated with the given key
   *
   * 对哈希表执行点查询。
   * @param transaction 当前交易
   * @param key 查找的键
   * @param[out] 结果与给定键关联的值
   * @return 与给定键关联的值
   */
  bool GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) override;

  /**
   * Resizes the table to at least twice the initial size provided.
   * @param initial_size the initial size of the hash table
   *
   * 将表的大小至少调整为所提供的初始大小的两倍。
   * @paraminitial_size 哈希表的初始大小
   */
  void Resize(size_t initial_size);

  /**
   * Gets the size of the hash table
   * @return current size of the hash table
   */
  size_t GetSize();

 private:
  // member variable
  page_id_t header_page_id_;
  BufferPoolManager *buffer_pool_manager_;
  KeyComparator comparator_;

  // Readers includes inserts and removes, writer is only resize
  // 读取器包括插入和删除，写入器仅调整大小
  ReaderWriterLatch table_latch_;

  // Hash function
  HashFunction<KeyType> hash_fn_;

  /* -------------------自定义--------------------------- */
  // 可存储的slot数量
  size_t buck_size_;
  // 当前header中的block数量
  size_t block_size_;
  // 在本地存储的block_page对应副本，减少对header_page的IO操作
  std::vector<page_id_t> block_page_ids_;
  // hash表中已使用的bucket数量，来计算何时扩容
  std::atomic<size_t> used_size_;

  void InitHeader(HashTableHeaderPage* hash_header, size_t used_size);

  HASH_TABLE_BLOCK_TYPE* GetHashBlock(page_id_t page_id);

  // 实际工作的insert
  bool InsertImpl(Transaction *transaction, const KeyType &key, const ValueType &value);

  // 更新下标啥的, WRflag = 1: 加/解 写锁    WRflag = 0：加/解 读锁
  void UpdateIndex(size_t& block_index, size_t& bucket_index, Page* page, HASH_TABLE_BLOCK_TYPE* block, bool WR_flag);
};

}  // namespace bustub
