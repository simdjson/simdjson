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
    assert(bench.Records().size() == reference.Records().size());
    for (size_t i=0; i<bench.Records().size(); i++) {
      if (bench.Records()[i] != reference.Records()[i]) {
        std::cerr << "Bench Record " << i << std::endl;
        std::cerr << "----------------------" << std::endl;
        std::cerr << bench.Records()[i] << std::endl;
        std::cerr << "Reference Record " << i << std::endl;
        std::cerr << "----------------------" << std::endl;
        std::cerr << reference.Records()[i] << std::endl;
        throw "Parse produced the wrong values!";
      }
    }
    reference.TearDown();
  }

  // Run the benchmark
  event_collector events(true);
  events.start();
  for (SIMDJSON_UNUSED auto _ : state) {
    if (!bench.Run(json)) { state.SkipWithError("tweet reading failed"); return; }
  }
  auto bytes = json.size() * state.iterations();
  state.SetBytesProcessed(bytes);
  state.SetItemsProcessed(bench.Records().size() * state.iterations());
  auto counts = events.end();
  if (events.has_events()) {
    state.counters["Ins./Byte"] = double(counts.instructions()) / double(bytes);
    state.counters["Ins./Cycle"] = double(counts.instructions()) / double(counts.cycles());
    state.counters["Cycles/Byte"] = double(counts.cycles()) / double(bytes);
    state.counters["BranchMiss"] = benchmark::Counter(counts.branch_misses(), benchmark::Counter::kAvgIterations);
    state.counters["CacheMiss"] = benchmark::Counter(counts.cache_misses(), benchmark::Counter::kAvgIterations);
    state.counters["CacheRef"] = benchmark::Counter(counts.cache_references(), benchmark::Counter::kAvgIterations);
  }
}
