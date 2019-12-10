#ifndef _CAS_QUEUE_H_
#define _CAS_QUEUE_H_

#include <atomic>
#include "config.h"
#include "lock_free_allocator.h"
#include "queue.h"

using namespace std;

template <typename T>
class CasQueue : public Queue<T> {
 public:
  CasQueue()
      : alloc_(),
        count_(0),
        head_(Pointer(alloc(), 0)),
        tail_(head_.load(memory_order_relaxed)) {}

  ~CasQueue() {
    while (true) {
      Pointer ptr = head_.load(std::memory_order_relaxed);
      if (ptr.node_) {
        head_.store(ptr.node_->next_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
        dealloc(ptr.node_);  // destruct itself before its member variables
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
    Node* new_tail = alloc(value);
    PushImpl(new_tail);
  }

  virtual void Push(T&& value) override {
    Node* new_tail = alloc(std::move(value));
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
            dealloc(curr_head.node_);
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
    Pointer(Node* node, int tag) : node_(node), tag_(tag) {
      // confirm that we can use double-word CAS for struct Pointer
      static_assert(sizeof(Pointer) == sizeof(__int128));
    }

    bool operator==(const Pointer& another) const {
      return this->node_ == another.node_ && this->tag_ == another.tag_;
    }
  };

  // have to be initialized before the pointers
  LockFreeAllocator<Node> alloc_;  // memory-safe lock-free allocator

  ALIGNED atomic<int> count_;

  ALIGNED atomic<Pointer> head_;
  ALIGNED atomic<Pointer> tail_;

  template <typename... Args>
  Node* alloc(Args&&... args) {
    using Tr = std::allocator_traits<LockFreeAllocator<Node>>;
    Node* node = Tr::allocate(alloc_, 1);
    Tr::construct(alloc_, node, std::forward<Args>(args)...);
    return node;
  }

  void dealloc(Node* node) {
    using Tr = std::allocator_traits<LockFreeAllocator<Node>>;
    Tr::deallocate(alloc_, node, 1);
  }
};

#endif  // _CAS_QUEUE_H_
