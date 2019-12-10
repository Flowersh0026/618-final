#ifndef _FREE_LIST_ADAPTER_H_
#define _FREE_LIST_ADAPTER_H_

#include "free_list.h"
#include "queue.h"

template <typename T>
class FreeListAdapter : public Queue<T> {
 public:
  FreeListAdapter() = default;
  virtual ~FreeListAdapter() = default;

  virtual void Push(const T& value) override {
    Node* node = new Node();
    node->next = nullptr;
    node->value = value;
    free_list_.Push(node);
  }

  virtual void Push(T&& value) override {
    Node* node = new Node();
    node->next = nullptr;
    node->value = std::move(value);
    free_list_.Push(node);
  }

  virtual std::optional<T> Pop() override {
    std::optional<T> opt;
    Node* node = free_list_.Pop();
    if (node) {
      opt = std::move(node->value);
      delete node;
    }
    return opt;
  }

 private:
  struct Node {
    Node* next;  // free list stores its next pointer at offset 0
    T value;
  };

  FreeList<Node> free_list_;
};

#endif  // _FREE_LIST_ADAPTER_H_
