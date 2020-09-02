#pragma once

template<typename B, typename R> static void ParseRecordsBenchmark(benchmark::State &state, const simdjson::padded_string &json) {
  // Warmup and equality check (make sure the data is right!)
  B bench;
  bench.SetUp();
  if (!bench.Run(json)) { state.SkipWithError("warmup tweet reading failed"); return; }
  {
    R reference;
    reference.SetUp();
    if (!reference.Run(json)) { state.SkipWithError("reference tweet reading failed"); return; }
    // assert(bench.Records() == reference.Records());
    reference.TearDown();
  }

  // Run the benchmark
  event_collector<true> events;
  events.start();
  for (SIMDJSON_UNUSED auto _ : state) {
    if (!bench.Run(json)) { state.SkipWithError("tweet reading failed"); return; }
  }
  state.SetBytesProcessed(json.size() * state.iterations());
  state.SetItemsProcessed(bench.Records().size() * state.iterations());
  auto counts = events.end();
  if (events.has_events()) {
    state.counters["Instructions"] = counts.instructions();
    state.counters["Cycles"] = counts.cycles();
    state.counters["Branch Misses"] = counts.branch_misses();
    state.counters["Cache References"] = counts.cache_references();
    state.counters["Cache Misses"] = counts.cache_misses();
  }
}
