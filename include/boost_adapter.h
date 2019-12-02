#ifndef _BOOST_ADAPTER_H_
#define _BOOST_ADAPTER_H_

#include "queue.h"

#include <assert.h>
#include <boost/lockfree/queue.hpp>

template <typename T>
class BoostAdapter : public Queue<T> {
 public:
  BoostAdapter() : queue_(0) {}
  virtual ~BoostAdapter() = default;

  virtual void Push(const T& value) override {
    bool ok = queue_.push(value);
    assert(ok);
  }

  virtual void Push(T&& value) override {
    bool ok = queue_.push(value);
    assert(ok);
  }

  virtual std::optional<T> Pop() override {
    std::optional<T> opt;
    T value;
    bool ok = queue_.pop(value);
    if (ok) {
      opt = value;
    }
    return opt;
  }

 private:
  boost::lockfree::queue<T> queue_;
};

#endif  // _BOOST_ADAPTER_H_
