#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include "queue.h"
#include "tm_queue.h"

template <typename T>
class Wrapper : public Queue<T> {
 public:
  Wrapper() {}
  virtual ~Wrapper() {}

  virtual void Push(T&& value) override {
    std::unique_lock<std::mutex> lock(mut_);
    queue_.push(value);
    cond_.notify_one();
  }

  virtual std::optional<T> Pop() override {
    std::unique_lock<std::mutex> lock(mut_);
    while (queue_.empty()) {
      cond_.wait(lock);
    }
    std::optional<T> res = std::make_optional(queue_.front());
    queue_.pop();
    return res;
  }

 private:
  mutable std::mutex mut_;
  std::condition_variable cond_;
  std::queue<T> queue_;
};

struct DummyValue {
  int val;

  DummyValue() {}

  DummyValue(int val) : val(val) {
    std::cout << "constructor" << std::endl;
  }

  ~DummyValue() {
    std::cout << "destructor" << std::endl;
  }

  DummyValue(const DummyValue& dummy) : val(dummy.val) {
    std::cout << "copy constructor" << std::endl;
  }

  DummyValue(DummyValue&& dummy) : val(std::move(dummy.val)) {
    std::cout << "move constructor" << std::endl;
  }

  DummyValue& operator=(const DummyValue& dummy) {
    val = dummy.val;
    std::cout << "copy assignment" << std::endl;
    return *this;
  }

  DummyValue& operator=(DummyValue&& dummy) {
    val = std::move(dummy.val);
    std::cout << "move assignment" << std::endl;
    return *this;
  }
};

TEST(WrapperTest, CheckCompile) {
  TmQueue<DummyValue> another;
  Wrapper<DummyValue> wrapper;
  std::cout << "start" << std::endl;
  DummyValue input(1);
  std::cout << "get input" << std::endl;
  wrapper.Push(std::move(input));
  std::cout << "finish push" << std::endl;
  DummyValue output = wrapper.Pop().value();
  std::cout << "finish pop" << std::endl;
  ASSERT_EQ(output.val, 1);
}

// correctness test, benchmark should be placed into another file

TEST(TmQueue, SimpleTest) {
  TmQueue<DummyValue> another;
  DummyValue input(1);
  another.Push(std::move(input));
  DummyValue output = another.Pop().value();
  ASSERT_EQ(output.val, 1);
}


