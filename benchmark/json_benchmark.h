#pragma once

template<typename B, typename R> static void JsonBenchmark(benchmark::State &state, const simdjson::padded_string &json) {
  event_collector collector(true);
  event_aggregate events;

  // Warmup and equality check (make sure the data is right!)
  B bench;
  if (!bench.Run(json)) { state.SkipWithError("warmup tweet reading failed"); return; }
  {
    R reference;
    if (!reference.Run(json)) { state.SkipWithError("reference tweet reading failed"); return; }
    if (bench.Result() != reference.Result()) { state.SkipWithError("results are not the same"); return; }
  }

  // Run the benchmark
  for (simdjson_unused auto _ : state) {
    collector.start();

    if (!bench.Run(json)) { state.SkipWithError("tweet reading failed"); return; }

    events << collector.end();
  }

  state.SetBytesProcessed(json.size() * state.iterations());
  state.SetItemsProcessed(bench.ItemCount() * state.iterations());
  state.counters["best_bytes_per_sec"] = benchmark::Counter(double(json.size()) / events.best.elapsed_sec());
  state.counters["best_items_per_sec"] = benchmark::Counter(double(bench.ItemCount()) / events.best.elapsed_sec());

  state.counters["docs_per_sec"] = benchmark::Counter(1.0, benchmark::Counter::kIsIterationInvariantRate);
  state.counters["best_docs_per_sec"] = benchmark::Counter(1.0 / events.best.elapsed_sec());

  if (collector.has_events()) {
    state.counters["instructions"] = events.instructions();
    state.counters["cycles"]       = events.cycles();
    state.counters["branch_miss"]  = events.branch_misses();
    state.counters["cache_miss"]   = events.cache_misses();
    state.counters["cache_ref"]    = events.cache_references();

    state.counters["instructions_per_byte"]  = events.instructions() / double(json.size());
    state.counters["instructions_per_cycle"] = events.instructions() / events.cycles();
    state.counters["cycles_per_byte"]        = events.cycles()       / double(json.size());
    state.counters["frequency"]              = benchmark::Counter(events.cycles(), benchmark::Counter::kIsIterationInvariantRate);

    state.counters["best_instructions"] = events.best.instructions();
    state.counters["best_cycles"]       = events.best.cycles();
    state.counters["best_branch_miss"]  = events.best.branch_misses();
    state.counters["best_cache_miss"]   = events.best.cache_misses();
    state.counters["best_cache_ref"]    = events.best.cache_references();

    state.counters["best_instructions_per_byte"]  = events.best.instructions() / double(json.size());
    state.counters["best_instructions_per_cycle"] = events.best.instructions() / events.best.cycles();
    state.counters["best_cycles_per_byte"]        = events.best.cycles()       / double(json.size());
    state.counters["best_frequency"]              = events.best.cycles()       / events.best.elapsed_sec();
  }
  state.counters["bytes"] = benchmark::Counter(double(json.size()));
  state.counters["items"] = benchmark::Counter(double(bench.ItemCount()));

  // Build the label
  using namespace std;
  stringstream label;
  label << fixed << setprecision(2);
  label << "[best:";
  label << " throughput="     << setw(6)  << (double(json.size()) / 1000000000.0     / events.best.elapsed_sec()) << " GB/s";
  label << " doc_throughput=" << setw(6)  << uint64_t(1.0                            / events.best.elapsed_sec()) << " docs/s";

  if (collector.has_events()) {
    label << " instructions=" << setw(12) << uint64_t(events.best.instructions())     << setw(0);
    label << " cycles="       << setw(12) << uint64_t(events.best.cycles())           << setw(0);
    label << " branch_miss="  << setw(8)  << uint64_t(events.best.branch_misses())    << setw(0);
    label << " cache_miss="   << setw(8)  << uint64_t(events.best.cache_misses())     << setw(0);
    label << " cache_ref="    << setw(10) << uint64_t(events.best.cache_references()) << setw(0);
  }

  label << " items=" << setw(10) << bench.ItemCount() << setw(0);
  label << " avg_time=" << setw(10) << uint64_t(events.elapsed_ns()) << setw(0) << " ns";
  label << "]";

  state.SetLabel(label.str());
}
