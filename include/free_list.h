#ifndef _FREE_LIST_H_
#define _FREE_LIST_H_

#include <stdint.h>
#include <atomic>

// basically a node-based lock-free stack
template <class Node>
class FreeList {
 public:
  FreeList() : top_(StackPtr(nullptr, 0)) {
    // must be able to store the next pointer inside the node
    static_assert(sizeof(Node) >= sizeof(Node*));
  }

  ~FreeList() {
    StackPtr top = top_.load(std::memory_order_relaxed);
    assert(top.node == nullptr);  // deallocation is not free list's job
  }

  void Push(Node* node) {
    if (node == nullptr) {
      return;  // ignore nullptr
    }
    while (true) {
      StackPtr top = top_.load(std::memory_order_relaxed);
      SetNext(node, top.node);
      if (CAS(top_, top, StackPtr(node, top.tag + 1))) {
        return;
      }
    }
  }

  Node* Pop() {
    while (true) {
      StackPtr top = top_.load(std::memory_order_relaxed);
      if (top.node == nullptr) {
        return nullptr;  // allocation is not free list's job
      }
      Node* next = GetNext(top.node);
      if (CAS(top_, top, StackPtr(next, top.tag + 1))) {
        Node* node = top.node;
        return node;
      }
    }
  }

 private:
  struct StackPtr {
    Node* node;
    uintptr_t tag;

    StackPtr(Node* node, uintptr_t tag) : node(node), tag(tag) {
      // ensure lock-free double-word CAS is used
      static_assert(sizeof(StackPtr) == sizeof(__int128));
    }
  };

  ALIGNED std::atomic<StackPtr> top_;

  static Node* GetNext(Node* node) { return *reinterpret_cast<Node**>(node); }

  static void SetNext(Node* node, Node* next) {
    *reinterpret_cast<Node**>(node) = next;
  }

  template <typename T1, typename T2>
  static bool CAS(std::atomic<T1>& var, T1& expected, T2&& desired) {
    return var.compare_exchange_weak(expected, std::forward<T2>(desired),
                                     std::memory_order_release,
                                     std::memory_order_relaxed);
  }
};

#endif  // _FREE_LIST_H_
