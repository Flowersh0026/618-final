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

// Cache line size is obtained from `$ getconf LEVEL1_DCACHE_LINESIZE`
#define CACHELINE_SIZE 64

#ifdef ENABLE_CACHELINE_ALIGNMENT
#define ALIGNED alignas(CACHELINE_SIZE)
#else
#define ALIGNED
#endif

#endif  // _QUEUE_H_
