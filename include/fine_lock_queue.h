#ifndef _FINE_LOCK_QUEUE_H_
#define _FINE_LOCK_QUEUE_H_

#include "queue.h"

#include <mutex>

template <typename T>
class FineLockQueue : public Queue<T> {
 public:
  FineLockQueue() : head_(new Node()), tail_(head_) { head_->next = nullptr; }

  virtual ~FineLockQueue() {
    while (head_) {
      Node* node = head_;
      head_ = head_->next;
      delete node;
    }
  }

  virtual void Push(const T& value) override {
    Node* node = new Node();
    node->value = value;
    node->next = nullptr;
    PushImpl(node);
  }

  virtual void Push(T&& value) override {
    Node* node = new Node();
    node->value = std::move(value);
    node->next = nullptr;
    PushImpl(node);
  }

  virtual std::optional<T> Pop() override {
    std::lock_guard<std::mutex> lock(head_mut_);
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

  // TODO: align to cache line
  Node* head_;
  Node* tail_;

  std::mutex head_mut_;
  std::mutex tail_mut_;

  void PushImpl(Node* node) {
    std::lock_guard<std::mutex> lock(tail_mut_);
    tail_->next = node;
    tail_ = node;
  }
};

#endif  // _FINE_LOCK_QUEUE_H_
