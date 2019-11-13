#ifndef _TM_QUEUE_H_
#define _TM_QUEUE_H_

#include <immintrin.h>

#include "queue.h"

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
    while (head_ != nullptr) {
      Node* node = head_;
      head_ = head_->next;
      delete node;
    }
  }

  virtual void Push(T&& value) override {
    Node* node = new Node();
    node->value = std::move(value);
    node->next = nullptr;

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

  virtual std::optional<T> Pop() override {
    unsigned status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      if (head_locked_) {
        _xabort(_XABORT_EXPLICIT);  // will go to fallback path
      }
      Node* new_head = head_->next;
      std::optional<T> opt;
      if (new_head) {
        opt = new_head->value;
        Node* node = head_;
        head_ = new_head;
        delete node;
      }
      _xend();
      return opt;
    } else {  // fallback path
      std::lock_guard<std::mutex> lock(head_mut_);
      head_locked_ = true;
      Node* new_head = head_->next;
      std::optional<T> opt;
      if (new_head) {
        opt = new_head->value;
        Node* node = head_;
        head_ = new_head;
        delete node;
      }
      head_locked_ = false;
      return opt;
    }
  }

 private:
  struct Node {
    T value;
    Node* next;
  };

  Node* head_;
  Node* tail_;

  bool head_locked_;
  bool tail_locked_;
  std::mutex head_mut_;
  std::mutex tail_mut_;
};

#endif  // _TM_QUEUE_H_
