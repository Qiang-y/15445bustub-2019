//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// clock_replacer.cpp
//
// Identification: src/buffer/clock_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/clock_replacer.h"

#include <algorithm>
#include <iostream>

namespace bustub {

ClockReplacer::ClockReplacer(size_t num_pages) : clock_point_(-1), clock_max_size_(num_pages), clock_used_size_(0) {
  std::lock_guard<std::mutex> lock(this->latch_);
  // this->clock_.resize(num_pages);
}

ClockReplacer::~ClockReplacer() = default;

auto ClockReplacer::Victim(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(this->latch_);

  if(clock_used_size_ == 0)
    return false;
  while(true) {
    clock_point_ = (clock_point_ + 1) % clock_used_size_;
    if(clock_[clock_point_].second == true) {
      clock_[clock_point_].second = false;
    }
    else if(clock_[clock_point_].second == false) {
      *frame_id = clock_[clock_point_].first;
      this->clock_.erase(clock_.begin() + clock_point_);
      clock_used_size_-=1;
      // std::cout << "clock_point_ before = " << clock_point_;
      clock_point_ = (clock_point_ - 1 );                     // 这里不能写成clock_point_ = (clock_point_ - 1 ) % clock_used_size_ ， 因为-1(size_t的max)%任何都是0
      // std::cout << "clock_point_ after = " << clock_point_;
      if(clock_point_ > clock_used_size_ || clock_used_size_ == 0)
        clock_point_ = clock_used_size_ - 1;
      // std::cout << "clock_point_ = " << clock_point_ << "   clock_used_size_ = " << clock_used_size_ << std::endl;
      return true;
    }
  }
}

void ClockReplacer::Pin(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);

  auto it = std::find_if(clock_.begin(), clock_.end(), [frame_id](const std::pair<frame_id_t, bool> &another) {
    return frame_id == another.first;
  });

  if(it != clock_.end()) {
    // clock_point_ = it - clock_.begin();
    clock_.erase(it);
    clock_used_size_-=1;
    if(clock_point_ == clock_used_size_)
      clock_point_ --;
  }
}

void ClockReplacer::Unpin(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(this->latch_);
  for(auto &it : this->clock_) {
    if(it.first == frame_id) {
      it.second = true;
      return;
    }
  }
  if(this->clock_used_size_ < this->clock_max_size_) {
    this->clock_.emplace_back(std::pair<frame_id_t, bool>(frame_id, true));
    this->clock_used_size_+=1;
    this->clock_point_ = (this->clock_point_ + 1) % this->clock_used_size_;
  }
  else{
    // 替换
    frame_id_t temp_can_frame_id;
    Victim(&temp_can_frame_id);
    this->clock_.insert(clock_.begin() + clock_point_, std::pair<frame_id_t, bool>(frame_id, true));
  }
}

auto ClockReplacer::Size() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);

  return clock_used_size_;
}

}  // namespace bustub
