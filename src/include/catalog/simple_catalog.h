#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "buffer/buffer_pool_manager.h"
#include "catalog/schema.h"
#include "storage/index/index.h"
#include "storage/table/table_heap.h"

namespace bustub {

/**
 * Typedefs
 */
using table_oid_t = uint32_t;
using column_oid_t = uint32_t;

/**
 * Metadata about a table.
 */
struct TableMetadata {
  TableMetadata(Schema schema, std::string name, std::unique_ptr<TableHeap> &&table, table_oid_t oid)
      : schema_(std::move(schema)), name_(std::move(name)), table_(std::move(table)), oid_(oid) {}
  Schema schema_;
  std::string name_;
  std::unique_ptr<TableHeap> table_;
  table_oid_t oid_;
};

/**
 * SimpleCatalog is a non-persistent catalog that is designed for the executor to use.
 * It handles table creation and table lookup.
 *
 * SimpleCatalog是一个非持久性目录，专为执行器使用而设计。
 * 它处理表创建和表查找。
 */
class SimpleCatalog {
 public:
  /**
   * Creates a new catalog object.
   * @param bpm the buffer pool manager backing tables created by this catalog
   * @param lock_manager the lock manager in use by the system
   * @param log_manager the log manager in use by the system
   * 创建一个新的目录对象。
   * @param bpm 该目录创建的缓冲池管理器支持表
   * @param lock_manager 系统使用的锁管理器
   * @param log_manager 系统使用的日志管理器
   */
  SimpleCatalog(BufferPoolManager *bpm, LockManager *lock_manager, LogManager *log_manager)
      : bpm_{bpm}, lock_manager_{lock_manager}, log_manager_{log_manager} {}

  /**
   * Create a new table and return its metadata.
   * @param txn the transaction in which the table is being created
   * @param table_name the name of the new table
   * @param schema the schema of the new table
   * @return a pointer to the metadata of the new table
   * 创建新表并返回其元数据。
   * @param txn 正在创建表的事务
   * @param table_name新表的名称
   * @param schema 新表的 schema
   * @return指向新表元数据的指针
   */
  TableMetadata *CreateTable(Transaction *txn, const std::string &table_name, const Schema &schema) {
    BUSTUB_ASSERT(names_.count(table_name) == 0, "Table names should be unique!");
    auto oid = next_table_oid_++;
    std::unique_ptr<TableHeap> table_heap = std::make_unique<TableHeap>(bpm_, lock_manager_, log_manager_, txn);
    tables_[oid] = std::make_unique<TableMetadata>(schema, table_name, std::move(table_heap), oid);
    names_[table_name] = oid;
    return tables_[oid].get();
  }

  /** @return table metadata by name */
  TableMetadata *GetTable(const std::string &table_name) {
   if(names_.count(table_name) == 0) {
    throw std::out_of_range("Table not have the name");
   }
   table_oid_t table_oid = names_[table_name];
   // return tables_[table_oid].get();
   return GetTable(table_oid);
  }

  /** @return table metadata by oid */
  TableMetadata *GetTable(table_oid_t table_oid) {
   if(tables_.count(table_oid) == 0) {
    throw std::out_of_range("Don't have this table");
   }
   return tables_[table_oid].get();
  }

 private:
  [[maybe_unused]] BufferPoolManager *bpm_;
  [[maybe_unused]] LockManager *lock_manager_;
  [[maybe_unused]] LogManager *log_manager_;

  /** tables_ : table identifiers -> table metadata. Note that tables_ owns all table metadata.
   * tables_ ：表标识符 -> 表元数据。请注意，tables_ 拥有所有表元数据。*/
  std::unordered_map<table_oid_t, std::unique_ptr<TableMetadata>> tables_;

  /** names_ : table names -> table identifiers
   * name_ : 表名 -> 表标识符*/
  std::unordered_map<std::string, table_oid_t> names_;

  /** The next table identifier to be used.
   * 要使用的下一个表标识符。*/
  std::atomic<table_oid_t> next_table_oid_{0};
};
}  // namespace bustub
