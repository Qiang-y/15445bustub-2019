//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.h
//
// Identification: src/include/buffer/buffer_pool_manager.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>

#include "buffer/clock_replacer.h"
#include "recovery/log_manager.h"
#include "storage/disk/disk_manager.h"
#include "storage/page/page.h"

namespace bustub {

/**
 * BufferPoolManager reads disk pages to and from its internal buffer pool.
 */
class BufferPoolManager {
 public:
  enum class CallbackType { BEFORE, AFTER };
  using bufferpool_callback_fn = void (*)(enum CallbackType, const page_id_t page_id);

  /**
   * Creates a new BufferPoolManager.
   * @param pool_size the size of the buffer pool
   * @param disk_manager the disk manager
   * @param log_manager the log manager (for testing only: nullptr = disable logging)
   */
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager = nullptr);

  /**
   * Destroys an existing BufferPoolManager.
   */
  ~BufferPoolManager();

  /** Grading function. Do not modify! */
  Page *FetchPage(page_id_t page_id, bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, page_id);
    auto *result = FetchPageImpl(page_id);
    GradingCallback(callback, CallbackType::AFTER, page_id);
    return result;
  }

  /** Grading function. Do not modify! */
  bool UnpinPage(page_id_t page_id, bool is_dirty, bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, page_id);
    auto result = UnpinPageImpl(page_id, is_dirty);
    GradingCallback(callback, CallbackType::AFTER, page_id);
    return result;
  }

  /** Grading function. Do not modify! */
  bool FlushPage(page_id_t page_id, bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, page_id);
    auto result = FlushPageImpl(page_id);
    GradingCallback(callback, CallbackType::AFTER, page_id);
    return result;
  }

  /** Grading function. Do not modify! */
  Page *NewPage(page_id_t *page_id, bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, INVALID_PAGE_ID);
    auto *result = NewPageImpl(page_id);
    GradingCallback(callback, CallbackType::AFTER, *page_id);
    return result;
  }

  /** Grading function. Do not modify! */
  bool DeletePage(page_id_t page_id, bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, page_id);
    auto result = DeletePageImpl(page_id);
    GradingCallback(callback, CallbackType::AFTER, page_id);
    return result;
  }

  /** Grading function. Do not modify! */
  void FlushAllPages(bufferpool_callback_fn callback = nullptr) {
    GradingCallback(callback, CallbackType::BEFORE, INVALID_PAGE_ID);
    FlushAllPagesImpl();
    GradingCallback(callback, CallbackType::AFTER, INVALID_PAGE_ID);
  }

  /** @return pointer to all the pages in the buffer pool */
  Page *GetPages() { return pages_; }

  /** @return size of the buffer pool */
  size_t GetPoolSize() { return pool_size_; }

 protected:
  /**
   * Grading function. Do not modify!
   * Invokes the callback function if it is not null.
   * @param callback callback function to be invoked
   * @param callback_type BEFORE or AFTER
   * @param page_id the page id to invoke the callback with
   */
  void GradingCallback(bufferpool_callback_fn callback, CallbackType callback_type, page_id_t page_id) {
    if (callback != nullptr) {
      callback(callback_type, page_id);
    }
  }

  /**
   * Fetch the requested page from the buffer pool.
   * @param page_id id of page to be fetched
   * @return the requested page
   * 
   * @brief 从缓冲池中获取请求的页面。如果需要从磁盘获取 page_id 但所有框架当前都在使用中且不可驱逐（换句话说，是固定的）
   * ，则返回nullptr。
   *
   * 首先在缓冲池中查找 page_id 。如果未找到，请从空闲列表或replacer中选择一个替换 frame
   * （总是首先从空闲列表中查找），通过使用disk_scheduler_->Schedule()调度一个读取磁盘请求 DiskRequest 来从磁盘读取页面
   * ，并替换框架中的旧页面。与NewPage()类似，如果旧页面是脏的，
   * 需要将其写回磁盘并更新新页面的元数据
   *
   * 此外，请记住禁用逐出，并记录 frame 的访问历史记录，就像对 NewPage() 所做的那样。
   *
   * @param page_id 要获取的页面的id
   * @param access_type 页面访问类型，仅用于排行榜测试。
   * @return nullptr 如果无法获取page_id，否则指向所请求页面的指针
   */
  Page *FetchPageImpl(page_id_t page_id);

  /**
   * Unpin the target page from the buffer pool.
   * @param page_id id of page to be unpinned
   * @param is_dirty true if the page should be marked as dirty, false otherwise
   * @return false if the page pin count is <= 0 before this call, true otherwise
   * 
   * @brief 从缓冲池中取消固定目标页面。如果 page_id 不在缓冲池中或者其 pin 计数已经
   * 0，返回假。
   *
   * 减少页面的引脚数。如果引脚数达到 0，则replacer应驱逐该frame。
   * 另外，在页面上设置脏标志以指示该页面是否被修改。
   *
   * @param page_id 要取消固定的页面的 id
   * @param is_dirty 如果页面应标记为脏则为 true，否则为 false
   * @return false 如果该页不在页表中或在此调用之前其 pin 计数 <= 0 ，否则 true
   */
  bool UnpinPageImpl(page_id_t page_id, bool is_dirty);

  /**
   * Flushes the target page to disk.
   * @param page_id id of page to be flushed, cannot be INVALID_PAGE_ID
   * @return false if the page could not be found in the page table, true otherwise
   * 
   * @brief 将目标页面刷新到磁盘。
   *
   * 使用 DiskManager::WritePage() 方法将页面刷新到磁盘，无论脏标志如何。
   * 刷新后清除页面的脏标志。
   *
   * @param page_id 要刷新的页面的id，不能是INVALID_PAGE_ID
   * 如果在页表中找不到该页则返回 false，否则返回 true
   */
  bool FlushPageImpl(page_id_t page_id);

  /**
   * Creates a new page in the buffer pool.
   * @param[out] page_id id of created page
   * @return nullptr if no new pages could be created, otherwise pointer to new page
   * 
   * @brief 在缓冲池中创建一个新页面。将 page_id 设置为新页面的 id，如果所有 frame 都当前正在使用且不可驱逐（换句话说，已固定）设置为 nullptr。
   *
   * 您应该从空闲列表或替换器中选择替换 frame（始终优先从空闲列表中查找）
   * ，然后调用 AllocatePage() 方法获取新的页面 id。如果替换 frame 有脏页，
   * 您应该先将其写回磁盘。您还需要重置新页面的内存和元数据。
   *
   * 请记住通过调用replacer.SetEvictable(frame_id, false)来“固定” frame （这个函数是LRU_K中实现的，相当于clock的pin）
   * 这样替换器就不会在缓冲池管理器“取消固定”帧之前驱逐该帧。
   * 另外，请记住在替换器中记录帧的访问历史记录，以便 lru-k 算法发挥作用。
   *
   * @param[out] page_id 创建页面的id
   * @return nullptr 如果无法创建新页面，否则指向新页面的指针
   */
  Page *NewPageImpl(page_id_t *page_id);

  /**
   * Deletes a page from the buffer pool.
   * @param page_id id of page to be deleted
   * @return false if the page exists but could not be deleted, true if the page didn't exist or deletion succeeded
   * 
   * @brief 从缓冲池中删除页面。如果 page_id 不在缓冲池中，则不执行任何操作并返回 true。如果
   * 页面已固定且无法删除，立即返回 false。
   *
   * 从页表中删除该页后，停止在replacer中跟踪该框架并添加该框架回空闲列表。
   * 另外，重置页面的内存和元数据。最后，您应该调用 DeallocatePage() 来
   * 模拟释放磁盘上的页面。
   *
   * @param page_id 要删除的页面的id
   * @return false 如果页面存在但无法删除, true 如果页面不存在或删除成功
   */
  bool DeletePageImpl(page_id_t page_id);

  /**
   * Flushes all the pages in the buffer pool to disk.
   */
  void FlushAllPagesImpl();

  /** Number of pages in the buffer pool. */
  size_t pool_size_;
  /** Array of buffer pool pages.
   * 
   * 缓冲池页面数组。
   */
  Page *pages_;

  /** Pointer to the disk manager. */
  DiskManager *disk_manager_ __attribute__((__unused__));
  /** Pointer to the log manager. */
  LogManager *log_manager_ __attribute__((__unused__));

  /** Page table for keeping track of buffer pool pages.
   * 
   *  用于跟踪缓冲池页面的页表。
   */
  std::unordered_map<page_id_t, frame_id_t> page_table_;

  /** Replacer to find unpinned pages for replacement. */
  Replacer *replacer_;
  /** List of free pages.
   * 
   * 空闲列表，上面的frame中没有任何的page
   */
  std::list<frame_id_t> free_list_;
  /** This latch protects shared data structures. We recommend updating this comment to describe what it protects. 
   * 
   * 该锁存器保护共享数据结构。我们建议更新此评论以描述其保护的内容。
  */
  std::mutex latch_;

  std::atomic<page_id_t> next_page_id_;

  auto GetPageFromDIsk(page_id_t page_id) -> Page*;

  // 检查是否要刷新Log
  void CheckPageLog(Page* page);
};
}  // namespace bustub
