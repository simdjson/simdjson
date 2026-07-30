#include <benchmark/benchmark.h>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include "simdjson.h"

using namespace simdjson;

namespace {

enum class stream_case {
  ndjson_small,
  ndjson_large,
  newline_small,
  newline_large,
  rfc7464_small,
  rfc7464_large,
  comma_delimited_small,
  comma_delimited_large
};

constexpr size_t TARGET_BYTES = 128 * 1000 * 1000;
constexpr size_t SMALL_PAYLOAD = 16;
constexpr size_t LARGE_PAYLOAD = 4096;
constexpr size_t BATCH_SIZE = 1 << 20;

struct stream_dataset {
  padded_string json;
  size_t count{};
};

std::string make_document(size_t id, size_t payload_size) {
  return std::string{"{\"id\":"} + std::to_string(id) +
    ",\"name\":\"aaaaaaaa\",\"payload\":\"" +
    std::string(payload_size, 'x') + "\",\"flag\":true}";
}

stream_dataset build_dataset(stream_case which) {
  const bool small = which == stream_case::ndjson_small ||
                     which == stream_case::newline_small ||
                     which == stream_case::rfc7464_small ||
                     which == stream_case::comma_delimited_small;
  const bool rfc = which == stream_case::rfc7464_small ||
                   which == stream_case::rfc7464_large;
  const bool comma = which == stream_case::comma_delimited_small ||
                     which == stream_case::comma_delimited_large;
  const size_t payload_size = small ? SMALL_PAYLOAD : LARGE_PAYLOAD;
  const size_t count = TARGET_BYTES / (payload_size + 48);
  std::string out;
  out.reserve(count * (payload_size + 64));
  for (size_t i = 0; i < count; i++) {
    if (rfc) {
      out += char(0x1E);
    }
    if (comma && i > 0) {
      out += ',';
    }
    out += make_document(i, payload_size);
    if (!comma) {
      out += '\n';
    }
  }
  return {padded_string(out), count};
}

const stream_dataset &get_dataset(stream_case which) {
  static const stream_dataset ndjson_small =
    build_dataset(stream_case::ndjson_small);
  static const stream_dataset ndjson_large =
    build_dataset(stream_case::ndjson_large);
  static const stream_dataset rfc_small =
    build_dataset(stream_case::rfc7464_small);
  static const stream_dataset rfc_large =
    build_dataset(stream_case::rfc7464_large);
  static const stream_dataset comma_small =
    build_dataset(stream_case::comma_delimited_small);
  static const stream_dataset comma_large =
    build_dataset(stream_case::comma_delimited_large);
  switch (which) {
    case stream_case::ndjson_small:
      return ndjson_small;
    case stream_case::ndjson_large:
      return ndjson_large;
    case stream_case::newline_small:
      return ndjson_small;
    case stream_case::newline_large:
      return ndjson_large;
    case stream_case::rfc7464_small:
      return rfc_small;
    case stream_case::rfc7464_large:
      return rfc_large;
    case stream_case::comma_delimited_small:
      return comma_small;
    case stream_case::comma_delimited_large:
      return comma_large;
  }
  return ndjson_small;
}

void set_counters(benchmark::State &state, const stream_dataset &dataset) {
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(dataset.json.size()));
  state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(dataset.count));
}

template <stream_case which, bool threaded = true>
static void bench_ondemand(benchmark::State &state) {
  const auto &dataset = get_dataset(which);
  ondemand::parser parser;
  parser.threaded = threaded;
  stream_format format = stream_format::whitespace_delimited;
  if constexpr (which == stream_case::newline_small ||
                which == stream_case::newline_large) {
    format = stream_format::newline_delimited;
  } else if constexpr (which == stream_case::rfc7464_small ||
                       which == stream_case::rfc7464_large) {
    format = stream_format::json_sequence;
  } else if constexpr (which == stream_case::comma_delimited_small ||
                       which == stream_case::comma_delimited_large) {
    format = stream_format::comma_delimited;
  }
  for (const auto _ : state) {
    ondemand::document_stream docs;
    auto error = parser.iterate_many(dataset.json, BATCH_SIZE, format).get(docs);
    if (error) {
      state.SkipWithError(error_message(error));
      return;
    }
    uint64_t sum = 0;
    for (auto doc : docs) {
      ondemand::object obj;
      if ((error = doc.get_object().get(obj))) {
        state.SkipWithError(error_message(error));
        return;
      }
      uint64_t id;
      if ((error = obj["id"].get_uint64().get(id))) {
        state.SkipWithError(error_message(error));
        return;
      }
      sum += id;
    }
    benchmark::DoNotOptimize(sum);
  }
  set_counters(state, dataset);
}


// Documents are independent, so a caller can cut the input with slice_at and
// parse the pieces on as many threads as it likes.
template <stream_case which>
static void bench_sliced(benchmark::State &state) {
  const auto &dataset = get_dataset(which);
  const size_t threads = size_t(state.range(0));
  const stream_format format =
      (which == stream_case::newline_small || which == stream_case::newline_large)
          ? stream_format::newline_delimited
          : stream_format::whitespace_delimited;
  auto view = padded_string_view(dataset.json);

  for (const auto _ : state) {
    std::atomic<size_t> next{0};
    std::vector<uint64_t> sums(threads, 0);
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (size_t t = 0; t < threads; t++) {
      pool.emplace_back([&, t] {
        ondemand::parser parser;
        parser.threaded = false;
        uint64_t local = 0;
        for (;;) {
          const size_t i = next.fetch_add(1, std::memory_order_relaxed);
          if (i * BATCH_SIZE >= view.size()) { break; }
          auto piece = slice_at(view, '\n', BATCH_SIZE, i);
          if (piece.empty()) { continue; }
          ondemand::document_stream docs;
          if (parser.iterate_many(piece, piece.size(), format).get(docs)) { continue; }
          for (auto doc : docs) {
            uint64_t id;
            if (!doc["id"].get_uint64().get(id)) { local += id; }
          }
        }
        sums[t] = local;
      });
    }
    for (auto &th : pool) { th.join(); }
    uint64_t sum = 0;
    for (uint64_t v : sums) { sum += v; }
    benchmark::DoNotOptimize(sum);
  }
  set_counters(state, dataset);
}

template <stream_case which>
static void bench_dom(benchmark::State &state) {
  const auto &dataset = get_dataset(which);
  dom::parser parser;
  parser.threaded = true;
  stream_format format = stream_format::whitespace_delimited;
  if constexpr (which == stream_case::rfc7464_small ||
                which == stream_case::rfc7464_large) {
    format = stream_format::json_sequence;
  } else if constexpr (which == stream_case::comma_delimited_small ||
                       which == stream_case::comma_delimited_large) {
    format = stream_format::comma_delimited;
  }
  for (const auto _ : state) {
    dom::document_stream docs;
    auto error = parser.parse_many(dataset.json, BATCH_SIZE, format).get(docs);
    if (error) {
      state.SkipWithError(error_message(error));
      return;
    }
    uint64_t sum = 0;
    for (auto doc : docs) {
      uint64_t id;
      if ((error = doc["id"].get(id))) {
        state.SkipWithError(error_message(error));
        return;
      }
      sum += id;
    }
    benchmark::DoNotOptimize(sum);
  }
  set_counters(state, dataset);
}

} // namespace

BENCHMARK(bench_ondemand<stream_case::ndjson_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::ndjson_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::newline_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::newline_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
// The stage1/stage2 overlap thread caps out near 2x; slicing does not.
BENCHMARK(bench_ondemand<stream_case::ndjson_small, false>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::newline_small, false>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_sliced<stream_case::ndjson_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8);
BENCHMARK(bench_sliced<stream_case::newline_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8);
BENCHMARK(bench_ondemand<stream_case::rfc7464_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::rfc7464_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::comma_delimited_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::comma_delimited_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
// Non-threaded comma_delimited for comparison
BENCHMARK(bench_ondemand<stream_case::comma_delimited_small, false>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_ondemand<stream_case::comma_delimited_large, false>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);

BENCHMARK(bench_dom<stream_case::ndjson_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_dom<stream_case::ndjson_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_dom<stream_case::rfc7464_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_dom<stream_case::rfc7464_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_dom<stream_case::comma_delimited_small>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);
BENCHMARK(bench_dom<stream_case::comma_delimited_large>)
    ->UseRealTime()
    ->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();
