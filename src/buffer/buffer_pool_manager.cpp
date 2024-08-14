//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include <list>
#include <unordered_map>

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new ClockReplacer(pool_size);
  next_page_id_ = 0;
  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete replacer_;
}

Page *BufferPoolManager::FetchPageImpl(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  
  std::lock_guard<std::mutex> lock_guard(latch_);
  auto it = page_table_.find(page_id);
  // 找到该page
  if (it != page_table_.end()) {
    auto frame_id_temp = it->second;
    pages_[frame_id_temp].pin_count_ += 1;
    replacer_->Pin(frame_id_temp);
    return &pages_[frame_id_temp];
  }
  return GetPageFromDIsk(page_id);
}

bool BufferPoolManager::UnpinPageImpl(page_id_t page_id, bool is_dirty) {
  std::lock_guard<std::mutex> lock_guard(latch_);
  auto page_table_it = page_table_.find(page_id);
  if (page_table_it == page_table_.end())
    return false;                                   // 在缓冲池中未找到该page
  auto frame_id_temp = page_table_it->second;
  pages_[frame_id_temp].is_dirty_ = is_dirty;       // 设置脏页标志
  if(pages_[frame_id_temp].GetPinCount() == 0) {
    return false;                                   // 该page的引用计数为0，已加入raplacer
  }
  if(--pages_[frame_id_temp].pin_count_ == 0) {
    replacer_->Unpin(frame_id_temp);
  }
  return true;
}

bool BufferPoolManager::FlushPageImpl(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  if(page_id == INVALID_PAGE_ID) {
    throw std::invalid_argument("The argument 'page_id' can not be 'INVALID_PAGE_ID'.");
  }
  auto page_table_it = page_table_.find(page_id);
  // 没找到该page
  if(page_table_it == page_table_.end()) {
    return false;
  }
  auto frame_id_temp = page_table_it->second;

  // 用DiskManager::WritePage() 来写回，
  disk_manager_->WritePage(page_id, pages_[frame_id_temp].data_);
  // 清除脏页标志
  pages_[frame_id_temp].is_dirty_ = false;

  return true;
}

Page *BufferPoolManager::NewPageImpl(page_id_t *page_id) {
  // 0.   Make sure you call DiskManager::AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  std::lock_guard<std::mutex> lock_guard(latch_);
  frame_id_t frame_id_temp = -1;   // 记录可用的frame
  // 查找可用的fram_id
  if(!free_list_.empty()) {
    frame_id_temp = free_list_.front();
    free_list_.pop_front();
  }
  /* ??????????????????
   *  文档说要调用LRUK的etEvictable来固定frame，我不知道这是否等于clock中Pin还是Victim,Victim也会把frame从clock中删除
   */
  else if(replacer_->Victim(&frame_id_temp) == true) {
    // 若页面为脏则先写回
    if(pages_[frame_id_temp].IsDirty()) {
      FlushPage(pages_[frame_id_temp].page_id_);
      pages_[frame_id_temp].is_dirty_ = false;
    }
    page_table_.erase(pages_[frame_id_temp].page_id_);
  }

  // 综合处理 // 这里是新页似乎不用设置页中的内容
  if (frame_id_temp != -1) {
    auto new_page_id = next_page_id_++;
    pages_[frame_id_temp].page_id_ = new_page_id;
    pages_[frame_id_temp].pin_count_ = 1;
    *page_id = new_page_id;
    page_table_.emplace(new_page_id, frame_id_temp);
    return &pages_[frame_id_temp];
  }
  return nullptr;
}

bool BufferPoolManager::DeletePageImpl(page_id_t page_id) {
  // 0.   Make sure you call DiskManager::DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  std::lock_guard<std::mutex> lock_guard(latch_);
  auto page_table_it = page_table_.find(page_id);
  // 没找到该page
  if (page_table_it == page_table_.end()) return true;

  auto frame_id_temp = page_table_it->second;
  if (pages_[frame_id_temp].GetPinCount() > 0) return false;

  // 暂时不清楚这里是否要写回，认为应该是要
  if (pages_[frame_id_temp].is_dirty_ == true) FlushPage(page_id);

  // 清空所有数据和元数据
  pages_[frame_id_temp].ResetMemory();
  // pages_[frame_id_temp].is_dirty_ = false;
  pages_[frame_id_temp].page_id_ = INVALID_PAGE_ID;
  replacer_->Pin(frame_id_temp);
  page_table_.erase(page_id);

  // 将frame添加回空闲队列
  free_list_.emplace_back(frame_id_temp);

  // DeallocatePage(page_id);
  return false;
}

void BufferPoolManager::FlushAllPagesImpl() {
  // You can do it!

  // std::lock_guard<std::mutex> lock_guard(latch_);
  for (auto &page_table_it : page_table_) {
    auto frame_id_temp = page_table_it.second;

    disk_manager_->WritePage(page_table_it.first, pages_[frame_id_temp].data_);

    // 清除脏页标志
    pages_[frame_id_temp].is_dirty_ = false;
  }
}

auto BufferPoolManager::GetPageFromDIsk(page_id_t page_id)-> Page *{
  frame_id_t frame_id_temp = -1;   // 记录可用的frame
  // 查找可用的fram_id
  if(!free_list_.empty()) {
    frame_id_temp = free_list_.front();
    free_list_.pop_front();
  }
  /* ??????????????????
   *  文档说要调用LRUK的etEvictable来固定frame，我不知道这是否等于clock中Pin还是Victim
   */
  else if (replacer_->Victim(&frame_id_temp) == true) {
    page_table_.erase(pages_[frame_id_temp].page_id_);
    // 若页面为脏则先写回
    if (pages_[frame_id_temp].IsDirty()) {
      FlushPage(pages_[frame_id_temp].page_id_);
      pages_[frame_id_temp].is_dirty_ = false;
    }
  }

  // 综合处理
  if(frame_id_temp != -1) {
    pages_[frame_id_temp].page_id_ = page_id;
    pages_[frame_id_temp].pin_count_ += 1;      // 此处pin_count应为0->1
    page_table_.emplace(page_id, frame_id_temp);

    // 从磁盘获取页面
    disk_manager_->ReadPage(page_id, pages_[frame_id_temp].data_);
    return &pages_[frame_id_temp];
  }
  return nullptr;
}

}  // namespace bustub
