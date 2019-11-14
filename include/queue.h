#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <optional>

template <typename T>
class Queue {
 public:
  Queue() = default;
  virtual ~Queue() = default;

  virtual void Push(const T& value) = 0;
  virtual void Push(T&& value) = 0;
  virtual std::optional<T> Pop() = 0;
};

#endif  // _QUEUE_H_
