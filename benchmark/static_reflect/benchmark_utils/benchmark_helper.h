#ifndef BENCHMARK_HELPER_HPP
#define BENCHMARK_HELPER_HPP
#include "event_counter.h"
#include <atomic>

inline event_collector &get_collector() {
  static event_collector collector;
  return collector;
}

template <class function_type>
event_aggregate bench(const function_type &function, size_t min_repeat = 10,
                      size_t min_time_ns = 1000000000,
                      size_t max_repeat = 100000) {
  event_collector &collector = get_collector();
  event_aggregate aggregate{};
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
    if ((i + 1 == N) && (aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  return aggregate;
}

// Source of the 2 functions below:
// https://github.com/simdutf/simdutf/blob/master/benchmarks/base64/benchmark_base64.cpp
inline void pretty_print(size_t strings, size_t bytes, std::string name,
                         event_aggregate agg) {
  event_collector &collector = get_collector();
  printf("%-60s : ", name.c_str());
  printf(" %5.2f MB/s ", bytes * 1000 / agg.elapsed_ns());
  printf(" %5.2f Ms/s ", strings * 1000 / agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %5.2f GHz ", agg.cycles() / agg.elapsed_ns());
    printf(" %5.2f c/b ", agg.cycles() / bytes);
    printf(" %5.2f i/b ", agg.instructions() / bytes);
    printf(" %5.2f i/c ", agg.instructions() / agg.cycles());
  }
  printf("\n");
}
#endif