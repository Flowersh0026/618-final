#include <benchmark/benchmark.h>

#include <boost/lockfree/queue.hpp>

#include "cas_queue.h"
#include "fine_lock_queue.h"
#include "queue.h"
#include "rtm_queue.h"

using namespace std;

template <class T>
static void QueuePushBenchmark(benchmark::State& state) {
  static Queue<int>* q = nullptr;
  // Setup for each run of test.
  if (state.thread_index == 0) {
    q = new T();
  }

  for (auto _ : state) {
    q->Push(0);
  }
  state.SetItemsProcessed(state.iterations());
  // Teardown at the end of each test run.
  if (state.thread_index == 0) {
    delete q;
  }
}
BENCHMARK_TEMPLATE(QueuePushBenchmark, RtmQueue<int>)->ThreadRange(1, 256);
BENCHMARK_TEMPLATE(QueuePushBenchmark, CasQueue<int>)->ThreadRange(1, 256);

template <class T>
static void QueuePopBenchmark(benchmark::State& state) {
  static Queue<int>* q = nullptr;

  if (state.thread_index == 0) {
    const int num_items = state.range(0);
    q = new T();
    for (int i = 0; i < num_items; i++) {
      q->Push(0);
    }
  }

  const int num_iterations = state.range(0) / state.threads;
  while (state.KeepRunningBatch(num_iterations)) {
    auto ret = q->Pop();
  }
  state.SetItemsProcessed(num_iterations);
  // Teardown at the end of each test run.
  if (state.thread_index == 0) {
    delete q;
  }
}

BENCHMARK_TEMPLATE(QueuePopBenchmark, RtmQueue<int>)->ThreadRange(1, 256)->Arg(256000);
BENCHMARK_TEMPLATE(QueuePopBenchmark, CasQueue<int>)->ThreadRange(1, 256)->Arg(256000);

// multi-producer multi-consumer benchmark
template <class T>
void MpmcBenchmark(benchmark::State& state) {
  // set up
  static Queue<int>* queue = nullptr;
  if (state.thread_index == 0) {
    queue = new T();
  }

  // main benchmark
  if (state.thread_index % 2 == 0) {  // producer
    for (auto _ : state) {
      int data = 1;
      benchmark::DoNotOptimize(data);
      queue->Push(data);
    }
  } else {  // consumer
    for (auto _ : state) {
      std::optional<int> opt = queue->Pop();
      benchmark::DoNotOptimize(opt);
    }
  }

  // tear down
  if (state.thread_index == 0) {
    delete queue;
  }
}

BENCHMARK_TEMPLATE(MpmcBenchmark, FineLockQueue<int>)
    ->DenseThreadRange(2, 32, 2);
BENCHMARK_TEMPLATE(MpmcBenchmark, RtmQueue<int>)->DenseThreadRange(2, 32, 2);

// boost adapter
void BoostMpmcBenchmark(benchmark::State& state) {
  // set up
  static boost::lockfree::queue<int>* queue = nullptr;
  if (state.thread_index == 0) {
    queue = new boost::lockfree::queue<int>(0);
  }

  // main benchmark
  if (state.thread_index % 2 == 0) {  // producer
    for (auto _ : state) {
      int data = 1;
      benchmark::DoNotOptimize(data);
      queue->push(data);
    }
  } else {  // consumer
    for (auto _ : state) {
      int data;
      bool ok = queue->pop(data);
      benchmark::DoNotOptimize(ok);
    }
  }

  // tear down
  if (state.thread_index == 0) {
    delete queue;
  }
}

BENCHMARK(BoostMpmcBenchmark)->ThreadRange(2, 256);
