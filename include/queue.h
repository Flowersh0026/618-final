#ifndef _QUEUE_H_
#define _QUEUE_H_

template <typename T>
class Queue {
 public:
  Queue() = default;
  virtual ~Queue() = default;

  virtual void Push(T&& value) = 0;
  virtual T&& Pop() = 0;
};

#endif  // _QUEUE_H_
