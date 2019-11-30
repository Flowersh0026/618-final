#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include "queue.h"
#include <mutex>
#include <atomic>
#include <unordered_set>

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue() : head_(nullptr), tail_(nullptr) {
    // fprintf(stderr, "%s\n", "\n Initialize \n");
  }

  ~CasQueue() {
    // fprintf(stderr, "delete \n");
    // for (auto n : garbage_list_) {

      // delete n;
    // }
  }

  // BUG
  //
  // | Program state            | Thread 0               | Thread 1               |
  // |--------------------------|------------------------|------------------------|
  // | head = NULL, tail = NULL |                        |                        |
  // | head = NULL, tail = new1 | CAS(tail, curr1, new1) |                        |
  // | head = NULL, tail = new2 |                        | CAS(tail, curr2, new2) |
  // | head = new2, tail = new2 |                        | CAS(head, NULL, new2)  |
  // | head = new2, tail = new2 | CAS(head, NULL, new1)  |                        | <- CAS fail
  // |                          | Push ends, new1 lost   | Push ends              |
  virtual void Push(const T& value) {

    auto new_tail = new Node(value);
    Node* curr_tail;
    while (1) {
      curr_tail = tail_.load(std::memory_order_acquire);
      // fprintf(stderr, "push, curr_tail = %d\n", curr_tail);
      if (tail_.compare_exchange_weak(curr_tail, new_tail,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        // fprintf(stderr, "Pushed %d\n",value);
        Node* nullp = nullptr;
        head_.compare_exchange_strong(nullp, new_tail,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
        if (curr_tail != nullptr) {
          // fprintf(stderr, "push, set head to %d (%d)\n", new_tail, value);
          // head_.store(new_tail, std::memory_order_release);
        // } else {
          curr_tail->next_ = new_tail;
        }


        // atomic_fetch_add_explicit(&count_, 1, std::memory_order_release);
        count_.fetch_add(1, std::memory_order_release);
        // fprintf(stderr, "push, curr_tail = %d, Push %d at %d, Queue size %d \n", curr_tail, value, new_tail, count_.load(std::memory_order_acquire));
        break;
      }

    }
  }

  virtual void Push(T&& value) {
  }

  virtual std::optional<T> Pop() {
    Node* curr_head;
    Node* curr_tail;
    std::optional<T> optval;
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 500;

    while (1) {

      if (count_.load(std::memory_order_acquire) <= 0) {
        // fprintf(stderr, "%s\n", "Returned");
        return optval;
      }
      // fprintf(stderr, "Pop, Queue size %d \n", count_.load(std::memory_order_acquire));
      curr_head = head_.load(std::memory_order_acquire);
      curr_tail = tail_.load(std::memory_order_acquire);
      // fprintf(stderr, "curr_head = %d\n", curr_head);
      if (curr_head == nullptr) return optval;
      // fprintf(stderr, "pop, curr_head = %d, value = %d, next = %d, tail = %d\n", curr_head, curr_head->value_, curr_head->next_, tail_.load(std::memory_order_acquire));
      if (curr_head->next_ == nullptr && curr_tail != nullptr && curr_tail != curr_head) return optval;
      if (head_.compare_exchange_weak(curr_head, curr_head->next_,
                                      std::memory_order_release,
                                      std::memory_order_relaxed)) {
        // fprintf(stderr, "1 - curr_head = %d\n", curr_head);
        auto head_check = curr_head;
        tail_.compare_exchange_strong(head_check, curr_head->next_,
                                    std::memory_order_release,
                                    std::memory_order_relaxed);
        // fprintf(stderr, "2 - curr_head = %d\n", curr_head);
        optval = move(curr_head->value_);
        // fprintf(stderr, "deleting %d\n", curr_head);
        // delete curr_head;
        // garbage_list_.insert(curr_head);
        // curr_head = nullptr;

        // atomic_fetch_sub_explicit(&count_, 1, memory_order_release);
        count_.fetch_sub(1, std::memory_order_release);
        // fprintf(stderr, "pop, count down to %d new head = %d tail %d \n", count_.load(std::memory_order_relaxed), head_.load(std::memory_order_acquire), tail_.load(std::memory_order_acquire));
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

  // unordered_set<Node*> garbage_list_;

};

#endif  // _CAS_QUEUE_H_
