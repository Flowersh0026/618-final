#ifndef _LOCK_QUEUE_H_
#define _LOCK_QUEUE_H_

#include "queue.h"
#include <mutex>
#include <atomic>

using namespace std;

template <typename T>
class LockQueue : public Queue<T> {
 public:
  LockQueue() : head_(nullptr), tail_(nullptr) {}

  virtual void Push(const T& value) {
    lock_guard<mutex> lock(mu_);
    tail_->next_ = new Node(value);
    tail_ = tail_->next_;
    count_.store(count_.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
  }

  virtual void Push(T&& value) {
    lock_guard<mutex> lock(mu_);
    tail_->next_ = new Node(value);
    tail_ = tail_->next_;
    count_.store(count_.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
  }

  virtual optional<T> Pop() {
    lock_guard<mutex> lock(mu_);
    if (count_.Load(MemoryOrder::RELAXED) == 0) return;
    auto ret = head_->value_;
    auto old_head = head_;
    head_ = head_->next_;
    delete old_head;
    count_.store(count_.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
    return ret;
  }

 private:
  struct Node {
    T value_;
    Node* next_;

    Node(const T& value) : value_(value), next_(nullptr) {}
    Node(T&& value) : value_(value), next_(nullptr) {}
  };

  Node* head_;
  Node* tail_;

  mutex mu_;
  atomic<int> count_;




}

#endif  // _LOCK_QUEUE_H_