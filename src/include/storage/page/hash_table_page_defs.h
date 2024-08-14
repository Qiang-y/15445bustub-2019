//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_page_defs.h
//
// Identification: src/include/storage/page/hash_table_page_defs.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#define MappingType std::pair<KeyType, ValueType>

/** BLOCK_ARRAY_SIZE is the number of (key, value) pairs that can be stored in   * a block page. It is an approximate
 * calculation based on the size of MappingType (which is a std::pair of KeyType and ValueType). For each key/value
 * pair, we need two additional bits for occupied_ and readable_. 4 * PAGE_SIZE / (4 * sizeof (MappingType) + 1) =
 * PAGE_SIZE/(sizeof (MappingType) + 0.25) because 0.25 bytes = 2 bits is the space required to maintain the occupied
 * and readable flags for a key value pair.
 * 
 * BLOCK_ARRAY_SIZE 是线性探测哈希块页面中可以存储的（键、值）对的数量。这是一个
 * 根据 MappingType 的大小（即 KeyType 和 ValueType 的 std：:p air）进行近似计算。对于每个
 * 键/值对，我们需要两个额外的位来用于 occupied_ 和 readable_。4 * BUSTUB_PAGE_SIZE / （4 * 尺寸
 * （MappingType） + 1） = BUSTUB_PAGE_SIZE/（sizeof （MappingType） + 0.25），因为 0.25 字节 = 2 位是所需的空间
 * 维护键值对的占用和可读标志。
 * */
#define BLOCK_ARRAY_SIZE (4 * PAGE_SIZE / (4 * sizeof(MappingType) + 1))

#define HASH_TABLE_BLOCK_TYPE HashTableBlockPage<KeyType, ValueType, KeyComparator>
