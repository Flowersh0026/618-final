#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include "queue.h"
#include <mutex>

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue() : head_(nullptr), tail_(nullptr) {}

  virtual void Push(const T& value) {
    auto new_tail = new Node(value);
    Node* curr_tail;
    while (1) {
      curr_tail = tail_.load(std::memory_order_relaxed);
      if (tail_.compare_exchange_weak(curr_tail, new_tail,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        curr_tail->next_ = new_tail;
        atomic_fetch_add(&count_, 1);
        break;
      }
      
    }
  }

  virtual void Push(T&& value) {
  }

  virtual std::optional<T> Pop() {
    Node* curr_head;
    while (1) {
      if (count_.load(std::memory_order_relaxed) == 0) return;
      curr_head = head_.load(std::memory_order_relaxed);
      if (head_.compare_exchange_weak(curr_head, curr_head->next_,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        auto ret = curr_head->value_;
        delete curr_head;
        atomic_fetch_sub(&count_, 1);
        return ret;
      }
    }
  }

 private:
  struct Node {
    T value_;
    atomic<int> count_(0);
    atomic<Node*> next_(nullptr);

    Node(const T& value) : value_(value) {}
    Node(T&& value) : value_(value) {}
  };

  

  Node* head_;
  Node* tail_;

}

#endif  // _CAS_QUEUE_H_