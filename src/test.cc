#include "queue.h"
#include "tm_queue.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <random>
#include <thread>

class ConcurrentQueueTest
    : public ::testing::TestWithParam<
          std::tuple<size_t /* data size */, size_t /* thread num */>> {
 public:
  ConcurrentQueueTest()
      : size_(std::get<0>(GetParam())), thread_num_(std::get<1>(GetParam())),
        index_(0), input_(size_), output_(size_) {}

  // prepare random input data
  void SetUp() override {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, std::numeric_limits<int>::max());
    for (size_t i = 0; i < size_; ++i) {
      input_[i] = dist(gen);
    }
  }

  static void RunThread(Queue<int>* queue, const std::vector<int>* input,
                        std::atomic<size_t>* index, size_t size,
                        std::vector<int>* output) {
    for (size_t i = (*index)++; i < size; i = (*index)++) {
      int data = (*input)[i];
      queue->Push(std::move(data));
      while (true) {
        std::optional<int> opt = queue->Pop();
        if (opt) {
          (*output)[i] = opt.value();
          break;
        }
      }
    }
  }

  void RunTest(Queue<int>* queue) {
    if (thread_num_ == 0) {
      RunThread(queue, &input_, &index_, size_, &output_);
    } else {
      for (size_t i = 0; i < thread_num_; ++i) {
        threads_.emplace_back(ConcurrentQueueTest::RunThread, queue, &input_,
                              &index_, size_, &output_);
      }
      for (size_t i = 0; i < thread_num_; ++i) {
        threads_[i].join();
      }
      threads_.clear();
    }
    std::sort(input_.begin(), input_.end());
    std::sort(output_.begin(), output_.end());
    EXPECT_THAT(output_, ::testing::ElementsAreArray(input_));
  }

 private:
  const size_t size_;
  const size_t thread_num_;

  std::atomic<size_t> index_;
  std::vector<int> input_;
  std::vector<int> output_;

  std::vector<std::thread> threads_;
};

TEST_P(ConcurrentQueueTest, TmQueueTest) {
  TmQueue<int> queue;
  RunTest(&queue);
}

INSTANTIATE_TEST_SUITE_P(ConcurrentQueueTest, ConcurrentQueueTest,
                         ::testing::Combine(::testing::Values(100),
                                            ::testing::Values(0)));
