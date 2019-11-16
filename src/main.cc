#include <benchmark/benchmark.h>
#include <string.h>
#include <unistd.h>

#include "cas_queue.h"

using namespace std;

// template <typename... Args>
// string StrFormat(const char* format, Args... args) {
//   int size = snprintf(NULL, 0, format, args...);
//   string buf(size + 1, '\0');
//   snprintf(&buf[0], buf.size(), format, args...);
//   return buf;
// }

// static constexpr const char* key = "e";
// static constexpr double value = 2.718281828459;
// static constexpr const char* format = "%s: %.6f\n";

// static void StrFormatBenchmark(benchmark::State& state) {
//   for (auto _ : state) {
//     string s = StrFormat(format, key, value);
//     assert(s.size() > 0);
//     usleep(10);  // 10 us
//   }
// }
// BENCHMARK(StrFormatBenchmark);

// static void SnprintfBenchmark(benchmark::State& state) {
//   for (auto _ : state) {
//     char buf[4096];
//     int ret = snprintf(buf, 4096, format, key, value);
//     assert(ret > 0);
//     usleep(10);  // 10 us
//   }
// }
// BENCHMARK(SnprintfBenchmark);

static void CASQueuePushBenchmark(benchmark::State& state) {
  static CasQueue<int>* casq = nullptr;
  // Setup for each run of test.
  if (state.thread_index == 0) {
    casq = new CasQueue<int>();
  }
  
  for (auto _ : state) {
    casq->Push(0);
  }

  // Teardown at the end of each test run.
  if (state.thread_index == 0) {
    delete casq;
  }
}
BENCHMARK(CASQueuePushBenchmark)->ThreadRange(1, 256);

static void CASQueuePopBenchmark(benchmark::State& state) {
  static CasQueue<int>* casq = nullptr;
  
  if (state.thread_index == 0) {
    const int num_items = state.range(0);
    casq = new CasQueue<int>();
    for (int i = 0; i < num_items; i++) {
      casq->Push(0);
    }
  }

  const int num_iterations = state.range(0) / state.threads;
  while (state.KeepRunningBatch(num_iterations)) {
    auto ret = casq->Pop();
  }

  // Teardown at the end of each test run.
  if (state.thread_index == 0) {
    delete casq;
  }
}

BENCHMARK(CASQueuePushBenchmark)->ThreadRange(1, 256)->Arg(256000);