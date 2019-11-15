#ifndef _TM_QUEUE_H_
#define _TM_QUEUE_H_

#include "queue.h"

#include <immintrin.h>
#include <atomic>
#include <mutex>

template <typename T>
class TmQueue : public Queue<T> {
 public:
  TmQueue()
      : head_(new Node()),
        tail_(head_),
        head_locked_(false),
        tail_locked_(false) {
    head_->next = nullptr;
  }

  virtual ~TmQueue() {
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
    PushInternal(node);
  }

  virtual void Push(T&& value) override {
    Node* node = new Node();
    node->value = std::move(value);
    node->next = nullptr;
    PushInternal(node);
  }

  virtual std::optional<T> Pop() override {
    std::optional<T> optval;
    Node* node = nullptr;

    unsigned status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      if (head_locked_) {
        _xabort(_XABORT_EXPLICIT);  // will go to fallback path
      }
      Node* next = head_->next;
      if (next) {
        optval = std::move(next->value);
        node = head_;
        head_ = next;
      }
      _xend();
    } else {  // fallback path
      std::unique_lock<std::mutex> lock(head_mut_);
      head_locked_ = true;
      Node* next = head_->next;
      if (next) {
        optval = std::move(next->value);
        node = head_;
        head_ = next;
      }
      head_locked_ = false;
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

  std::atomic<bool> head_locked_;
  std::mutex head_mut_;

  std::atomic<bool> tail_locked_;
  std::mutex tail_mut_;

  void PushInternal(Node* node) {
    unsigned status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      if (tail_locked_) {
        _xabort(_XABORT_EXPLICIT);  // will go to fallback path
      }
      tail_->next = node;
      tail_ = node;
      _xend();
    } else {  // fallback path
      std::lock_guard<std::mutex> lock(tail_mut_);
      tail_locked_ = true;
      tail_->next = node;
      tail_ = node;
      tail_locked_ = false;
    }
  }
};

#endif  // _TM_QUEUE_H_
