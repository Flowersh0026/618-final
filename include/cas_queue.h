#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include "queue.h"

#include <atomic>

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue()
      : head_(Pointer(new Node(), 0)),
        tail_(head_.load(memory_order_relaxed)) {}

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
  }

 private:
  struct Node;

  void PushImpl(Node* new_tail) {
    Pointer curr_tail;
    while (1) {
      curr_tail = tail_.load(std::memory_order_relaxed);
      Pointer next = curr_tail.node_->next_.load(std::memory_order_relaxed);
      if (curr_tail == tail_.load(std::memory_order_relaxed)) {
        if (next.node_ == nullptr) {
          if (curr_tail.node_->next_.compare_exchange_weak(
                  next, Pointer(new_tail, next.tag_ + 1),
                  std::memory_order_release,
                  std::memory_order_relaxed)) {  // commit point, append to tail
            break;
          }
        } else {
          tail_.compare_exchange_weak(curr_tail,
                                      Pointer(next.node_, curr_tail.tag_ + 1),
                                      std::memory_order_release,
                                      std::memory_order_relaxed);  // fix tail_
        }
      }
    }
    tail_.compare_exchange_weak(
        curr_tail, Pointer(new_tail, curr_tail.tag_ + 1),
        std::memory_order_release, std::memory_order_relaxed);
    count_.fetch_add(1, std::memory_order_release);
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
        return std::nullopt;
      }
      curr_head = head_.load(std::memory_order_relaxed);
      curr_tail = tail_.load(std::memory_order_relaxed);
      Pointer next = curr_head.node_->next_.load(std::memory_order_relaxed);
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
                  std::memory_order_release,
                  std::memory_order_relaxed)) {  // commit point
            // add node to free list
            count_.fetch_sub(1, std::memory_order_release);
            break;
          }
        }
      }
    }
    return optval;
  }

 private:
  struct Pointer;

  struct ALIGNED Node {
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

  ALIGNED atomic<int> count_{0};

  ALIGNED atomic<Pointer> head_;
  ALIGNED atomic<Pointer> tail_;
};

#endif  // _CAS_QUEUE_H_
