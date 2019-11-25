#ifndef _COARSE_LOCK_QUEUE_H_
#define _COARSE_LOCK_QUEUE_H_

#include "queue.h"

#include <mutex>

template <typename T>
class CoarseLockQueue : public Queue<T> {
 public:
  CoarseLockQueue() : head_(new Node()), tail_(head_) { head_->next = nullptr; }

  virtual ~CoarseLockQueue() {
    while (head_) {
      Node* node = head_;
      head_ = head_->next;
      delete node;
    }
  }

  virtual void Push(const T& value) override {
    std::lock_guard<std::mutex> lock(mut_);
    Node* node = new Node();
    node->value = value;
    node->next = nullptr;
    PushImpl(node);
  }

  virtual void Push(T&& value) override {
    std::lock_guard<std::mutex> lock(mut_);
    Node* node = new Node();
    node->value = std::move(value);
    node->next = nullptr;
    PushImpl(node);
  }

  virtual std::optional<T> Pop() override {
    std::lock_guard<std::mutex> lock(mut_);
    std::optional<T> optval;
    Node* node = nullptr;
    Node* next = head_->next;
    if (next) {
      optval = std::move(next->value);
      node = head_;
      head_ = next;
    }
    delete node;
    return optval;
  }

 private:
  struct Node {
    T value;
    Node* next;
  };

  Node* head_;
  Node* tail_;

  std::mutex mut_;

  void PushImpl(Node* node) {
    tail_->next = node;
    tail_ = node;
  }
};

#endif  // _COARSE_LOCK_QUEUE_H_
