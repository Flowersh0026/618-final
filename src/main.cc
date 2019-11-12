#include <benchmark/benchmark.h>
#include <string.h>
#include <unistd.h>

using std::string;

template <typename... Args>
string StrFormat(const char* format, Args... args) {
  int size = snprintf(NULL, 0, format, args...);
  string buf(size + 1, '\0');
  snprintf(&buf[0], buf.size(), format, args...);
  return buf;
}

static constexpr const char* key = "e";
static constexpr double value = 2.718281828459;
static constexpr const char* format = "%s: %.6f\n";

static void StrFormatBenchmark(benchmark::State& state) {
  for (auto _ : state) {
    string s = StrFormat(format, key, value);
    assert(s.size() > 0);
    usleep(10);  // 10 us
  }
}
BENCHMARK(StrFormatBenchmark);

static void SnprintfBenchmark(benchmark::State& state) {
  for (auto _ : state) {
    char buf[4096];
    int ret = snprintf(buf, 4096, format, key, value);
    assert(ret > 0);
    usleep(10);  // 10 us
  }
}
BENCHMARK(SnprintfBenchmark);