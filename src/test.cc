#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <random>
#include <thread>

#include "boost_adapter.h"
#include "cas_queue.h"
#include "fine_lock_queue.h"
#include "free_list_adapter.h"
#include "lock_free_allocator.h"
#include "lock_queue.h"
#include "queue.h"
#include "rtm_queue.h"

class ConcurrentQueueTest
    : public ::testing::TestWithParam<
          std::tuple<size_t /* data size */, size_t /* producer num */,
                     size_t /* consumer num */>> {
 public:
  ConcurrentQueueTest()
      : size_(std::get<0>(GetParam())),
        producer_num_(std::get<1>(GetParam())),
        consumer_num_(std::get<2>(GetParam())),
        input_index_(0),
        input_(size_),
        output_index_(0),
        output_(size_) {}

  // prepare random input data
  void SetUp() override {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, std::numeric_limits<int>::max());
    for (size_t i = 0; i < size_; ++i) {
      input_[i] = dist(gen);
    }
  }

  static void RunProducer(Queue<int>* queue, const std::vector<int>* input,
                          std::atomic<size_t>* input_index, size_t size) {
    for (size_t i = (*input_index)++; i < size; i = (*input_index)++) {
      int data = (*input)[i];
      queue->Push(data);
    }
  }

  static void RunConsumer(Queue<int>* queue, std::vector<int>* output,
                          std::atomic<size_t>* output_index, size_t size) {
    for (size_t i = (*output_index)++; i < size; i = (*output_index)++) {
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
    ASSERT_GT(producer_num_, 0);
    ASSERT_GT(consumer_num_, 0);
    for (size_t i = 0; i < consumer_num_; ++i) {
      consumers_.emplace_back(ConcurrentQueueTest::RunConsumer, queue, &output_,
                              &output_index_, size_);
    }
    for (size_t i = 0; i < producer_num_; ++i) {
      producers_.emplace_back(ConcurrentQueueTest::RunProducer, queue, &input_,
                              &input_index_, size_);
    }
    for (size_t i = 0; i < consumer_num_; ++i) {
      consumers_[i].join();
    }
    for (size_t i = 0; i < producer_num_; ++i) {
      producers_[i].join();
    }
    producers_.clear();
    consumers_.clear();
    std::sort(input_.begin(), input_.end());
    std::sort(output_.begin(), output_.end());
    EXPECT_THAT(output_, ::testing::ElementsAreArray(input_));
  }

 private:
  const size_t size_;
  const size_t producer_num_;
  const size_t consumer_num_;

  std::atomic<size_t> input_index_;
  std::vector<int> input_;

  std::atomic<size_t> output_index_;
  std::vector<int> output_;

  std::vector<std::thread> producers_;
  std::vector<std::thread> consumers_;
};

TEST_P(ConcurrentQueueTest, RtmQueueTest) {
  RtmQueue<int> queue;
  RunTest(&queue);
}

TEST_P(ConcurrentQueueTest, CasQueueTest) {
  CasQueue<int> queue;
  RunTest(&queue);
}

TEST_P(ConcurrentQueueTest, FineQueueTest) {
  FineLockQueue<int> queue;
  RunTest(&queue);
}

TEST_P(ConcurrentQueueTest, BoostAdapterTest) {
  BoostAdapter<int> queue;
  RunTest(&queue);
}

TEST_P(ConcurrentQueueTest, LockQueueTest) {
  LockQueue<int> queue;
  RunTest(&queue);
}

TEST_P(ConcurrentQueueTest, FreeListAdapterTest) {
  FreeListAdapter<int> queue;
  RunTest(&queue);
}

INSTANTIATE_TEST_SUITE_P(ConcurrentQueueTest, ConcurrentQueueTest,
                         ::testing::Combine(::testing::Values(1, 1000, 1000000),
                                            ::testing::Values(1, 32),
                                            ::testing::Values(1, 32)));

class LockFreeAllocatorTest : public ::testing::Test {
 public:
  struct Node {
    char a;
    int b;
    float c;
    double d;
    Node* ptr;

    Node() : a('a'), b(1), c(3.14), d(4.0), ptr(nullptr) {}
  };
};

TEST_F(LockFreeAllocatorTest, AllocatorTest) {
  LockFreeAllocator<Node> alloc;
  Node* p = alloc.allocate(1);
  ASSERT_NE(p, nullptr);
  alloc.deallocate(p, 1);
  Node* q = alloc.allocate(1);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(q, p);
  alloc.deallocate(q, 1);
}

TEST_F(LockFreeAllocatorTest, AllocatorTraitsTest) {
  using alloc_tr = std::allocator_traits<LockFreeAllocator<Node>>;
  LockFreeAllocator<Node> alloc;

  Node* p = alloc_tr::allocate(alloc, 1);
  ASSERT_NE(p, nullptr);

  alloc_tr::construct(alloc, p);
  EXPECT_EQ(p->a, 'a');
  EXPECT_EQ(p->b, 1);
  EXPECT_FLOAT_EQ(p->c, 3.14);
  EXPECT_DOUBLE_EQ(p->d, 4.0);
  EXPECT_EQ(p->ptr, nullptr);

  // change it
  p->a = 'b';
  p->b = -1;
  p->c = -4.0;
  p->d = -100.1;
  p->ptr = p;

  EXPECT_EQ(p->a, 'b');
  EXPECT_EQ(p->b, -1);
  EXPECT_FLOAT_EQ(p->c, -4.0);
  EXPECT_DOUBLE_EQ(p->d, -100.1);
  EXPECT_EQ(p->ptr, p);

  alloc_tr::deallocate(alloc, p, 1);

  Node* q = alloc_tr::allocate(alloc, 1);
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(q, p);

  // should not be the default values
  EXPECT_NE(q->a, 'a');
  EXPECT_NE(q->b, 1);
  EXPECT_NE(q->c, 3.14);
  EXPECT_NE(q->d, 4.0);
  EXPECT_NE(q->ptr, nullptr);

  alloc_tr::construct(alloc, q);
  EXPECT_EQ(q->a, 'a');
  EXPECT_EQ(q->b, 1);
  EXPECT_FLOAT_EQ(q->c, 3.14);
  EXPECT_DOUBLE_EQ(q->d, 4.0);
  EXPECT_EQ(q->ptr, nullptr);

  alloc_tr::deallocate(alloc, q, 1);
}
