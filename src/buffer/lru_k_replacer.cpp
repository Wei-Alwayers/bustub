//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  current_timestamp_++;
  size_t max_k_distance = 0;
  size_t max_last_access_time = 0; // 距离最近一次访问的时间差
  auto target_it = node_store_.begin(); // 用于保存指定位置的迭代器
  for(auto it = node_store_.begin(); it != node_store_.end(); it++){
    if(it->second.isEvictable() == false){
      continue;
    }
//    size_t cur_frame_id = it->first;
    size_t cur_k_distance;
    size_t cur_last_access_time;
    if(it->second.getHistory().size() < k_){
      cur_k_distance = INT_MAX;
      cur_last_access_time = current_timestamp_ - it->second.getHistory().front();
    }
    else{
      cur_k_distance = current_timestamp_ - it->second.getHistory().back();
      cur_last_access_time = current_timestamp_ - it->second.getHistory().front();
    }
    if(cur_k_distance > max_k_distance){
      max_k_distance = cur_k_distance;
      max_last_access_time = cur_last_access_time;
      target_it = it;
    }
    else if(cur_k_distance == max_k_distance && cur_last_access_time > max_last_access_time){
      max_k_distance = cur_k_distance;
      max_last_access_time = cur_last_access_time;
      target_it = it;
    }
  }
  if(target_it == node_store_.end()){
    frame_id = nullptr;
    return false;
  }
  *frame_id = target_it->first;
  node_store_.erase(target_it->first);
  evictable_size_--;
  curr_size_--;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, AccessType access_type){
  std::lock_guard<std::mutex> lock(latch_);
  current_timestamp_++;
  if(node_store_.find(frame_id) != node_store_.end()){
    // 之前存在，更新一下accessTime
    node_store_[frame_id].addHistory(current_timestamp_);
  }
  else{
    if(curr_size_ >= replacer_size_){
      throw Exception("larger than replacer_size_");
    }
    else{
      // 之前不存在，插入新的record
      LRUKNode lrukNode = LRUKNode(k_, frame_id);
      lrukNode.addHistory(current_timestamp_);
      node_store_.insert(std::make_pair(frame_id, lrukNode));
      curr_size_++;
    }
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lock(latch_);
  if(node_store_.find(frame_id) == node_store_.end()){
    throw Exception("frame id is invalid");
  }
  else{
    bool cur_is_evictable = node_store_[frame_id].isEvictable();
    if(cur_is_evictable == true && set_evictable == false){
      node_store_[frame_id].setIsEvictable(set_evictable);
      evictable_size_--;
    }
    if(cur_is_evictable == false && set_evictable == true){
      node_store_[frame_id].setIsEvictable(set_evictable);
      evictable_size_++;
    }
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id){
  std::lock_guard<std::mutex> lock(latch_);
  if(node_store_.find(frame_id) == node_store_.end()){
    return;
  }
  bool is_evictable = node_store_[frame_id].isEvictable();
  if(is_evictable == false){
    throw Exception("non-evictable frame");
  }
  else{
    node_store_.erase(frame_id);
    evictable_size_--;
    curr_size_--;
  }
}

auto LRUKReplacer::Size() -> size_t { return evictable_size_ ;}

}  // namespace bustub
