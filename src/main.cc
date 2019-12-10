#include <benchmark/benchmark.h>

#include "boost_adapter.h"
#include "cas_queue.h"
#include "fine_lock_queue.h"
#include "lock_queue.h"
#include "queue.h"
#include "rtm_queue.h"
#include "distributed_queue.h"

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

// BENCHMARK_TEMPLATE(QueuePushBenchmark, RtmQueue<int>)->ThreadRange(1, 16);
// BENCHMARK_TEMPLATE(QueuePushBenchmark, CasQueue<int>)->ThreadRange(1, 16);
// BENCHMARK_TEMPLATE(QueuePushBenchmark, BoostAdapter<int>)->ThreadRange(1, 16);

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

// BENCHMARK_TEMPLATE(QueuePopBenchmark, RtmQueue<int>)
//     ->ThreadRange(1, 256)
//     ->Arg(256000);
// BENCHMARK_TEMPLATE(QueuePopBenchmark, CasQueue<int>)
//     ->ThreadRange(1, 256)
//     ->Arg(256000);
// BENCHMARK_TEMPLATE(QueuePopBenchmark, BoostAdapter<int>)
//     ->ThreadRange(1, 256)
//     ->Arg(256000);

template <class T>
void MpmcBenchmark(benchmark::State& state) {
  // set up
  static Queue<int>* queue = nullptr;
  if (state.thread_index == 0) {
    queue = new T();
  }

  // main benchmark
  if (state.thread_index % 2 == 0) {  // producer
    int64_t num_push = 0;
    for (auto _ : state) {
      int data = 1;
      benchmark::DoNotOptimize(data);
      queue->Push(data);
      ++num_push;
    }
    state.counters["push_count"] =
        benchmark::Counter(static_cast<double>(num_push));
  } else {  // consumer
    int64_t num_pop = 0;
    for (auto _ : state) {
      std::optional<int> opt = queue->Pop();
      if (opt) {
        benchmark::DoNotOptimize(opt.value());
        ++num_pop;
      }
    }
    state.counters["pop_count"] =
        benchmark::Counter(static_cast<double>(num_pop));
  }

  // tear down
  if (state.thread_index == 0) {
    delete queue;
  }
}

// BENCHMARK_TEMPLATE(MpmcBenchmark, BoostAdapter<int>)
//     ->DenseThreadRange(2, 32, 2);
// BENCHMARK_TEMPLATE(MpmcBenchmark, CasQueue<int>)->DenseThreadRange(2, 32, 2);
// BENCHMARK_TEMPLATE(MpmcBenchmark, FineLockQueue<int>)
//     ->DenseThreadRange(2, 32, 2);
// BENCHMARK_TEMPLATE(MpmcBenchmark, LockQueue<int>)->DenseThreadRange(2, 32, 2);
BENCHMARK_TEMPLATE(MpmcBenchmark, RtmQueue<int>)->DenseThreadRange(2, 32, 2);
BENCHMARK_TEMPLATE(MpmcBenchmark, DistributedQueue<int>)->DenseThreadRange(2, 32, 2);
