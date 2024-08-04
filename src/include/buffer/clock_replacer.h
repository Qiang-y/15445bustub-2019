//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// clock_replacer.h
//
// Identification: src/include/buffer/clock_replacer.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <vector>

#include "buffer/replacer.h"
#include "common/config.h"

namespace bustub {

/**
 * ClockReplacer implements the clock replacement policy, which approximates the Least Recently Used policy.
 * ClockReplacer 实现时钟替换策略，该策略近似于最近最少使用策略。
 */
class ClockReplacer : public Replacer {
 public:
  /**
   * Create a new ClockReplacer.
   * @param num_pages the maximum number of pages the ClockReplacer will be required to store
   *
   * @param num_pages ClockReplacer 需要存储的最大页数
   */
  explicit ClockReplacer(size_t num_pages);

  /**
   * Destroys the ClockReplacer.
   */
  ~ClockReplacer() override;

  auto Victim(frame_id_t *frame_id) -> bool override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  auto Size() -> size_t override;

  size_t clock_point_;

 private:
  // TODO(student): implement me!
  std::mutex latch_;

  // size_t clock_point_;

  std::vector<std::pair<frame_id_t, bool>> clock_;

  // 最多可容下的 frame 数量
  std::size_t clock_max_size_;

  // 当前使用（可移除）的 frame 数量
  std::size_t clock_used_size_;


};

}  // namespace bustub
