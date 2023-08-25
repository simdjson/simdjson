#pragma once

#include "simdjson.h"
#include "event_counter.h"
#include <iostream>

namespace json_benchmark {

void maybe_display_implementation() {
  static bool displayed_implementation = false;
  if(!displayed_implementation) {
    displayed_implementation = true;
    std::cout << "simdjson::dom implementation:      " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "simdjson::ondemand implementation (stage 1): " << simdjson::get_active_implementation()->name() << std::endl;
    std::cout << "simdjson::ondemand implementation (stage 2): " << simdjson::builtin_implementation()->name() << std::endl;
  }
}

template<typename B, typename R> static void run_json_benchmark(benchmark::State &state) {
  maybe_display_implementation();

  event_collector collector;
  event_aggregate events;

  // Warmup and equality check (make sure the data is right!)
  B bench;
  if (!bench.setup(state)) { return; }
  if (!bench.before_run(state)) { state.SkipWithError("warmup document before_run failed"); return; }
  if (!bench.run(state)) { state.SkipWithError("warmup document reading failed"); return; }
  if (!bench.after_run(state)) { state.SkipWithError("warmup document after_run failed"); return; }
  {
    R reference;
    if (!reference.setup(state)) { return; }
    if (!reference.before_run(state)) { state.SkipWithError("reference before_run failed"); };
    if (!reference.run(state)) { state.SkipWithError("reference document reading failed"); return; }
    if (!reference.after_run(state)) { state.SkipWithError("reference before_run failed"); };
    if (!bench.diff(state, reference)) { return; }
  }

  // Run the benchmark
  for (simdjson_unused auto _ : state) {
    if (!bench.before_run(state)) { state.SkipWithError("before_run failed"); };
    collector.start();
    if (!bench.run(state)) { state.SkipWithError("run failed"); return; }
    auto event = collector.end();
    events << event;
    state.SetIterationTime(event.elapsed_sec());
    if (!bench.after_run(state)) { state.SkipWithError("after_run failed"); return; };
  }

  state.SetBytesProcessed(bench.bytes_per_iteration() * state.iterations());
  state.SetItemsProcessed(bench.items_per_iteration() * state.iterations());
  state.counters["best_docs_per_sec"] = benchmark::Counter(double(bench.documents_per_iteration()) / events.best.elapsed_sec());
  state.counters["best_bytes_per_sec"] = benchmark::Counter(double(bench.bytes_per_iteration()) / events.best.elapsed_sec());
  state.counters["best_items_per_sec"] = benchmark::Counter(double(bench.items_per_iteration()) / events.best.elapsed_sec());
  state.counters["docs_per_sec"] = benchmark::Counter(double(bench.documents_per_iteration()), benchmark::Counter::kIsIterationInvariantRate);

  if (collector.has_events()) {
    state.counters["instructions"] = events.instructions();
    state.counters["cycles"]       = events.cycles();
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    state.counters["branch_miss"]  = events.branch_misses();
    state.counters["cache_miss"]   = events.cache_misses();
    state.counters["cache_ref"]    = events.cache_references();
#endif
    state.counters["instructions_per_byte"]  = events.instructions() / double(bench.bytes_per_iteration());
    state.counters["instructions_per_cycle"] = events.instructions() / events.cycles();
    state.counters["cycles_per_byte"]        = events.cycles()       / double(bench.bytes_per_iteration());
    state.counters["frequency"]              = benchmark::Counter(events.cycles(), benchmark::Counter::kIsIterationInvariantRate);

    state.counters["best_instructions"] = events.best.instructions();
    state.counters["best_cycles"]       = events.best.cycles();
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    state.counters["best_branch_miss"]  = events.best.branch_misses();
    state.counters["best_cache_miss"]   = events.best.cache_misses();
    state.counters["best_cache_ref"]    = events.best.cache_references();
#endif

    state.counters["best_instructions_per_byte"]  = events.best.instructions() / double(bench.bytes_per_iteration());
    state.counters["best_instructions_per_cycle"] = events.best.instructions() / events.best.cycles();
    state.counters["best_cycles_per_byte"]        = events.best.cycles()       / double(bench.bytes_per_iteration());
    state.counters["best_frequency"]              = events.best.cycles()       / events.best.elapsed_sec();
  }
  state.counters["bytes"] = benchmark::Counter(double(bench.bytes_per_iteration()));
  state.counters["items"] = benchmark::Counter(double(bench.items_per_iteration()));

  // Build the label
  using namespace std;
  stringstream label;
  label << fixed << setprecision(2);
  label << "[BEST:";
  label << " throughput="     << setw(6)  << (double(bench.bytes_per_iteration()) / 1000000000.0     / events.best.elapsed_sec()) << " GB/s";
  label << " doc_throughput=" << setw(6)  << uint64_t(bench.documents_per_iteration()                / events.best.elapsed_sec()) << " docs/s";

  if (collector.has_events()) {
    label << " instructions=" << setw(12) << uint64_t(events.best.instructions())     << setw(0);
    label << " cycles="       << setw(12) << uint64_t(events.best.cycles())           << setw(0);
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    label << " branch_miss="  << setw(8)  << uint64_t(events.best.branch_misses())    << setw(0);
    label << " cache_miss="   << setw(8)  << uint64_t(events.best.cache_misses())     << setw(0);
    label << " cache_ref="    << setw(10) << uint64_t(events.best.cache_references()) << setw(0);
#endif
  }

  label << " items=" << setw(10) << bench.items_per_iteration() << setw(0);
  label << " avg_time=" << setw(10) << uint64_t(events.elapsed_ns()) << setw(0) << " ns";
  label << "]";

  state.SetLabel(label.str());
}

} // namespace json_benchmark
