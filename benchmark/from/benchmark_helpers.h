#ifndef BENCHMARK_HELPERS_H
#define BENCHMARK_HELPERS_H

#include "event_counter.h"
#include <atomic>

event_collector collector;

template <class function_type>
std::pair<event_aggregate, size_t>
bench(const function_type &&function, size_t min_repeat = 10,
      size_t min_time_ns = 40'000'000, size_t max_repeat = 10000000) {
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  event_aggregate warm_aggregate{};
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    warm_aggregate << allocate_count;
    if ((i + 1 == N) && (warm_aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  event_aggregate aggregate{};
  for (size_t i = 0; i < 10; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    for (size_t i = 0; i < N; i++) {
      function();
    }
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
  }
  return {aggregate, N};
}

double pretty_print(const std::string &name, size_t num_chars,
                    std::pair<event_aggregate, size_t> result) {
  const auto &agg = result.first;
  size_t N = result.second;
  num_chars *= N;
  printf("%-40s : %8.2f ns %8.2f GB/s", name.c_str(),
         agg.elapsed_ns() / num_chars, num_chars / agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %8.2f GHz %8.2f cycles/char %8.2f ins./char %8.2f i/c",
           agg.cycles() / agg.elapsed_ns(), agg.cycles() / num_chars,
           agg.instructions() / num_chars, agg.instructions() / agg.cycles());
  }
  printf("\n");
  return num_chars / agg.elapsed_ns();
}

double pretty_print_array(const std::string &name, size_t num_chars, size_t num_elements,
                    std::pair<event_aggregate, size_t> result) {
  const auto &agg = result.first;
  size_t N = result.second;
  num_chars *= N;
  printf("%-40s : %8.2f ns/char %8.2f ns/value %8.2f GB/s", name.c_str(),
         agg.elapsed_ns() / num_chars, agg.elapsed_ns() / num_elements, num_chars / agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %8.2f GHz %8.2f cycles/char %8.2f ins./char %8.2f ins./value %8.2f i/c",
           agg.cycles() / agg.elapsed_ns(), agg.cycles() / num_chars,
           agg.instructions() / num_chars, agg.instructions() / num_elements, agg.instructions() / agg.cycles());
  }
  printf("\n");
  return num_chars / agg.elapsed_ns();
}


#endif