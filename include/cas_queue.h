#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include "queue.h"
#include <mutex>
#include <atomic>

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue() : head_(nullptr), tail_(nullptr) {
    fprintf(stderr, "%s\n", "\n Initialize \n");
  }

  virtual void Push(const T& value) {
    
    auto new_tail = new Node(value);
    Node* curr_tail;
    while (1) {
      curr_tail = tail_.load(std::memory_order_acquire);
      if (tail_.compare_exchange_weak(curr_tail, new_tail,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        if (curr_tail == nullptr) {
          fprintf(stderr, "set head\n");
          head_.store(new_tail, std::memory_order_release);
        } else {
          curr_tail->next_ = new_tail;
        }
        Node* nullp = nullptr;
        head_.compare_exchange_strong(nullp, new_tail,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
        fprintf(stderr, "curr_tail = %d, Push %d at %d, Queue size %d \n", curr_tail, value, new_tail, count_.load(std::memory_order_acquire));
        // atomic_fetch_add_explicit(&count_, 1, std::memory_order_release);
        count_.fetch_add(1, std::memory_order_release);
        break;
      }
      
    }
  }

  virtual void Push(T&& value) {
  }

  virtual std::optional<T> Pop() {
    Node* curr_head;
    std::optional<T> optval;

    while (1) {
      
      if (count_.load(std::memory_order_acquire) == 0) {
        // fprintf(stderr, "%s\n", "Returned");
        return optval;
      }
      fprintf(stderr, "Pop, Queue size %d \n", count_.load(std::memory_order_acquire));
      curr_head = head_.load(std::memory_order_acquire);
      fprintf(stderr, "curr_head = %d, value = %d\n", curr_head, curr_head->value_);
      if (head_.compare_exchange_weak(curr_head, curr_head->next_,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        // fprintf(stderr, "1 - curr_head = %d\n", curr_head);
        auto head_check = curr_head;
        tail_.compare_exchange_strong(head_check, nullptr,
                                    std::memory_order_release,
                                    std::memory_order_relaxed);
        // fprintf(stderr, "2 - curr_head = %d\n", curr_head);
        optval = move(curr_head->value_);
        // fprintf(stderr, "deleting %d\n", curr_head);
        delete curr_head;
        fprintf(stderr, "new head = %d\n", head_.load(std::memory_order_acquire));
        // atomic_fetch_sub_explicit(&count_, 1, memory_order_release);
        count_.fetch_sub(1, std::memory_order_release);
        return optval;
      }
    }
  }

 private:
  struct Node {
    T value_;
    Node* next_;

    Node(const T& value) : value_(value), next_(nullptr) {}
    Node(T&& value) : value_(value), next_(nullptr) {}
  };

  atomic<int> count_{0};

  atomic<Node*> head_;
  atomic<Node*> tail_;

};

#endif  // _CAS_QUEUE_H_