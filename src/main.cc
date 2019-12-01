#include <benchmark/benchmark.h>

#include <boost/lockfree/queue.hpp>
#include <thread>

#include "boost_adapter.h"
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
// BENCHMARK_TEMPLATE(QueuePushBenchmark, RtmQueue<int>)->ThreadRange(1, 256);
// BENCHMARK_TEMPLATE(QueuePushBenchmark, CasQueue<int>)->ThreadRange(1, 256);

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

// BENCHMARK_TEMPLATE(QueuePopBenchmark, RtmQueue<int>)->ThreadRange(1,
// 256)->Arg(256000); BENCHMARK_TEMPLATE(QueuePopBenchmark,
// CasQueue<int>)->ThreadRange(1, 256)->Arg(256000);

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

// BENCHMARK_TEMPLATE(MpmcBenchmark, FineLockQueue<int>)
//     ->DenseThreadRange(2, 32, 2);
// BENCHMARK_TEMPLATE(MpmcBenchmark, RtmQueue<int>)->DenseThreadRange(2, 32, 2);
// BENCHMARK_TEMPLATE(MpmcBenchmark, BoostAdapter<int>)->DenseThreadRange(2, 32,
// 2);

template <class T>
void MpmcPushBenchmark(benchmark::State& state) {
  static Queue<int>* queue = nullptr;
  static bool done = false;
  static std::vector<std::thread>* consumers = nullptr;

  // set up
  if (state.thread_index == 0) {
    queue = new T();
    consumers = new std::vector<std::thread>();
    for (int i = 0; i < state.threads; ++i) {
      consumers->emplace_back(
          [](bool* done, Queue<int>* queue) {
            while (!(*done)) {
              std::optional<int> opt = queue->Pop();
              benchmark::DoNotOptimize(opt);
            }
          },
          &done, queue);
    }
  }

  // main benchmark for producer
  for (auto _ : state) {
    int data = 1;
    benchmark::DoNotOptimize(data);
    queue->Push(data);
  }

  // tear down
  if (state.thread_index == 0) {
    done = true;
    for (std::thread& consumer : *consumers) {
      consumer.join();
    }
    delete consumers;
    delete queue;
  }
}

BENCHMARK_TEMPLATE(MpmcPushBenchmark, FineLockQueue<int>)
    ->DenseThreadRange(1, 16);
BENCHMARK_TEMPLATE(MpmcPushBenchmark, RtmQueue<int>)->DenseThreadRange(1, 16);
BENCHMARK_TEMPLATE(MpmcPushBenchmark, BoostAdapter<int>)
    ->DenseThreadRange(1, 16);

template <class T>
void MpmcPopBenchmark(benchmark::State& state) {
  static Queue<int>* queue = nullptr;
  static bool done = false;
  static std::vector<std::thread>* producers = nullptr;

  // set up
  if (state.thread_index == 0) {
    queue = new T();
    producers = new std::vector<std::thread>();
    for (int i = 0; i < state.threads; ++i) {
      producers->emplace_back(
          [](bool* done, Queue<int>* queue) {
            while (!(*done)) {
              int data = 1;
              benchmark::DoNotOptimize(data);
              queue->Push(data);
            }
          },
          &done, queue);
    }
  }

  // main benchmark for consumers
  for (auto _ : state) {
    std::optional<int> opt = queue->Pop();
    benchmark::DoNotOptimize(opt);
  }

  // tear down
  if (state.thread_index == 0) {
    done = true;
    for (std::thread& producer : *producers) {
      producer.join();
    }
    delete producers;
    delete queue;
  }
}

BENCHMARK_TEMPLATE(MpmcPopBenchmark, FineLockQueue<int>)
    ->DenseThreadRange(1, 16);
BENCHMARK_TEMPLATE(MpmcPopBenchmark, RtmQueue<int>)->DenseThreadRange(1, 16);
BENCHMARK_TEMPLATE(MpmcPopBenchmark, BoostAdapter<int>)
    ->DenseThreadRange(1, 16);
