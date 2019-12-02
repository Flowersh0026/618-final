#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include "queue.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue()
      : head_(Pointer(new Node(), 0)), tail_(head_.load(memory_order_relaxed)) {
    // fprintf(stderr, "%s\n", "\n Initialize \n");
  }

  ~CasQueue() {
    while (true) {
      Pointer ptr = head_.load(std::memory_order_relaxed);
      if (ptr.node_) {
        head_.store(ptr.node_->next_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
        delete ptr.node_;
      } else {
        break;
      }
    }
    // fprintf(stderr, "delete \n");
    // for (auto n : garbage_list_) {
    //   delete n;
    // }
  }

 private:
  struct Node;

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
  void PushImpl(Node* new_tail) {
    Pointer curr_tail;
    while (1) {
      curr_tail = tail_.load(std::memory_order_relaxed);
      Pointer next = curr_tail.node_->next_.load(std::memory_order_relaxed);
      // fprintf(stderr, "push, curr_tail = %d\n", curr_tail);
      if (curr_tail == tail_.load(std::memory_order_relaxed)) {
        if (next.node_ == nullptr) {
          if (curr_tail.node_->next_.compare_exchange_weak(
                  next, Pointer(new_tail, next.tag_ + 1),
                  std::memory_order_release, std::memory_order_relaxed)) {  // commit point, append to the tail
            break;
          }
        } else {
          tail_.compare_exchange_weak(
              curr_tail, Pointer(next.node_, curr_tail.tag_ + 1),
              std::memory_order_release, std::memory_order_relaxed);  // fix tail_
        }
      }
    }
    tail_.compare_exchange_weak(
        curr_tail, Pointer(new_tail, curr_tail.tag_ + 1),
        std::memory_order_release, std::memory_order_relaxed);
    // fprintf(stderr, "Pushed %d\n",value);
    // atomic_fetch_add_explicit(&count_, 1, std::memory_order_release);
    count_.fetch_add(1, std::memory_order_release);
    // fprintf(stderr, "push, curr_tail = %d, Push %d at %d, Queue size %d
    // \n", curr_tail, value, new_tail,
    // count_.load(std::memory_order_acquire));
  }

 public:
  virtual void Push(const T& value) override {
    Node* new_tail = new Node(value);
    PushImpl(new_tail);
  }

  virtual void Push(T&& value) override {
    Node* new_tail = new Node(std::move(value));
    PushImpl(new_tail);
  }

  virtual std::optional<T> Pop() {
    Pointer curr_head;
    Pointer curr_tail;
    std::optional<T> optval;

    while (1) {
      if (count_.load(std::memory_order_acquire) <= 0) {
        // fprintf(stderr, "%s\n", "Returned");
        return std::nullopt;
      }
      // fprintf(stderr, "Pop, Queue size %d \n",
      // count_.load(std::memory_order_acquire));
      curr_head = head_.load(std::memory_order_relaxed);
      curr_tail = tail_.load(std::memory_order_relaxed);
      Pointer next = curr_head.node_->next_.load(std::memory_order_relaxed);
      // fprintf(stderr, "curr_head = %d\n", curr_head);
      // fprintf(stderr, "pop, curr_head = %d, value = %d, next = %d, tail =
      // %d\n", curr_head, curr_head->value_, curr_head->next_,
      // tail_.load(std::memory_order_acquire));
      if (curr_head == head_.load(std::memory_order_relaxed)) {
        if (curr_head.node_ == curr_tail.node_) {
          if (next.node_ == nullptr) {
            return std::nullopt;
          }
          tail_.compare_exchange_weak(
              curr_tail, Pointer(next.node_, curr_tail.tag_ + 1),
              std::memory_order_release, std::memory_order_relaxed);
        } else {
          optval = next.node_->value_;  // copy, because the CAS may fails
          if (head_.compare_exchange_weak(
                  curr_head, Pointer(next.node_, curr_head.tag_ + 1),
                  std::memory_order_release, std::memory_order_relaxed)) {  // commit point
            // atomic_fetch_sub_explicit(&count_, 1, memory_order_release);
            count_.fetch_sub(1, std::memory_order_release);
            // fprintf(stderr, "pop, count down to %d new head = %d tail %d \n",
            // count_.load(std::memory_order_relaxed),
            // head_.load(std::memory_order_acquire),
            // tail_.load(std::memory_order_acquire));
            break;
          }
        }
      }
    }
    return optval;
  }

 private:
  struct Pointer;

  struct Node {
    T value_;
    std::atomic<Pointer> next_;

    Node(const T& value) : value_(value), next_(Pointer()) {}
    Node(T&& value) : value_(value), next_(Pointer()) {}
    Node() : next_(Pointer()) {}
  };

  struct Pointer {
    Node* node_;
    int tag_;

    Pointer() : Pointer(nullptr, 0) {}
    Pointer(Node* node, int tag) : node_(node), tag_(tag) {}

    bool operator==(const Pointer& another) const {
      return this->node_ == another.node_ && this->tag_ == another.tag_;
    }
  };

  // confirm that we can use double-word CAS for struct Pointer
  static_assert(sizeof(Pointer) == sizeof(__int128));

  atomic<int> count_{0};

  atomic<Pointer> head_;
  atomic<Pointer> tail_;

  // unordered_set<Node*> garbage_list_;
};

#endif  // _CAS_QUEUE_H_
