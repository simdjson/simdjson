#ifndef SIMDJSON_ONDEMAND_PARALLEL_H
#define SIMDJSON_ONDEMAND_PARALLEL_H

#include "simdjson/base.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/ondemand.h"

#include <utility>
#include <vector>

#ifdef SIMDJSON_THREADS_ENABLED
#include <atomic>
#include <cstring>
#include <thread>
#endif

namespace simdjson {
/**
 * @private Experimental API (subject to change).
 *
 * Scale the *parsing* step of a large JSON document stream across several
 * threads. simdjson's built-in `parser.iterate_many(...)` can already overlap
 * stage 1 (structural indexing) of one batch with stage 2 (value materializa-
 * tion) of the previous batch, but that tops out at a ~2x speedup and only
 * when the input is split into several batches. When the bottleneck is stage 2
 * (extracting values out of each document), it pays to run many parser threads
 * at once. That is what this header provides.
 *
 * ## Design
 *
 * Each worker claims a byte range from a shared atomic counter and snaps both
 * ends forward to the next delimiter, so the slices are contiguous, never split
 * a document, and are computed without any coordination beyond that counter.
 * Each worker owns its own `ondemand::parser` and its own output vector, so no
 * locking happens on the hot path. A user-supplied `extract` function turns each document into a value
 * of type `T`; the values are collected into one vector per worker ("shards").
 *
 * Because documents are distributed across workers, the resulting values are
 * interleaved: values keep their relative order *within* a shard, but the
 * shards themselves are interleaved with respect to the original input. Callers
 * that do not care about global order (the common case for bulk extraction)
 * simply iterate every shard.
 *
 * ## Input format
 *
 * Two stream formats are supported, each with a slicing rule that relies on a
 * delimiter byte that can never occur inside a JSON value (JSON forbids
 * unescaped control characters in strings), so a slice never splits a document
 * and boundaries are found with a single `memchr`:
 *
 *   - `whitespace_delimited` (ndjson/JSONL, default): cut just after a newline.
 *   - `json_sequence` (RFC 7464): cut just before a record separator (`0x1E`).
 *
 * The comma-delimited formats are intentionally not supported here: a comma can
 * appear nested or inside a string, so top-level commas can only be located by
 * a serial structural scan, which cannot be done from an arbitrary offset.
 * Whitespace-only-separated input with no newlines still parses correctly; it
 * just is not split (one slice).
 */
namespace experimental {

/** Options controlling parse_many_parallel. */
struct parallel_stream_options {
  /**
   * Number of worker (parser) threads, and the exact number of threads
   * spawned. When 0, we use `std::thread::hardware_concurrency()`, with a
   * floor of 1. Note that throughput normally peaks at or below the number of
   * *physical* cores, so on a machine with simultaneous multithreading the
   * default overshoots.
   */
  size_t threads = 0;
  /**
   * Target size, in bytes, of each slice handed to a worker. Each slice is
   * extended to the next document boundary so that documents are never split.
   * Larger slices amortize per-slice overhead; smaller slices improve load
   * balancing.
   */
  size_t slice_bytes = 1u << 20; // 1 MiB
  /**
   * The format of the input stream. Determines both how the input is sliced and
   * how each worker parses its slice. Only stream_format::whitespace_delimited
   * (the default, ndjson) and stream_format::json_sequence (RFC 7464) are
   * supported; any other value disables slicing (the input is parsed as a
   * single slice, i.e. serially).
   */
  stream_format format = stream_format::whitespace_delimited;
};

/**
 * The result of parse_many_parallel: one std::vector<T> per worker thread.
 *
 * Values keep their relative order within a shard; shards are interleaved with
 * respect to the original input.
 */
template <typename T>
class parallel_stream_result {
public:
  parallel_stream_result() noexcept = default;

  /** The per-worker vectors of extracted values. */
  const std::vector<std::vector<T>> &shards() const noexcept { return _shards; }

  /** Total number of values extracted across all shards. */
  size_t size() const noexcept {
    size_t total = 0;
    for (const auto &shard : _shards) { total += shard.size(); }
    return total;
  }

  /** Whether any value was extracted. */
  bool empty() const noexcept { return size() == 0; }

  /** Number of documents that failed to parse or that `extract` rejected. */
  size_t error_count() const noexcept { return _errors; }

  /** The first error reported by a slice or by `extract` (or SUCCESS). */
  error_code first_error() const noexcept { return _first_error; }

  /** Invoke `fn(const T&)` for every value, shard by shard. */
  template <typename Fn>
  void for_each(Fn &&fn) const {
    for (const auto &shard : _shards) {
      for (const auto &value : shard) { fn(value); }
    }
  }

private:
  std::vector<std::vector<T>> _shards{};
  size_t _errors = 0;
  error_code _first_error = SUCCESS;

  template <typename U, typename F>
  friend parallel_stream_result<U>
  parse_many_parallel(padded_string_view, F &&, parallel_stream_options);
};

#ifdef SIMDJSON_THREADS_ENABLED
namespace internal {

/**
 * Snap `want` forward to the next slice boundary within [0, hi). The boundary
 * is a delimiter byte that cannot occur inside a JSON value, so a document is
 * never split:
 *  - whitespace_delimited: just after a newline ('\n');
 *  - json_sequence: just before a record separator (0x1E).
 * Any other format returns `hi` (no split).
 *
 * This depends only on `want`, which is what lets every worker compute its own
 * slice: worker i's end is snap(raw + slice_bytes) and worker i+1's start is
 * snap(raw + slice_bytes) for the same value, so the slices are contiguous and
 * never overlap.
 */
inline size_t snap_to_boundary(const char *data, size_t hi, size_t want,
                               stream_format format) {
  if (want >= hi) { return hi; }
  switch (format) {
  case stream_format::whitespace_delimited: {
    const void *newline = std::memchr(data + want, '\n', hi - want);
    return newline ? size_t(static_cast<const char *>(newline) - data) + 1 : hi;
  }
  case stream_format::json_sequence: {
    const void *rs = std::memchr(data + want, 0x1e, hi - want);
    return rs ? size_t(static_cast<const char *>(rs) - data) : hi;
  }
  default:
    return hi; // unsupported format: a single slice (parsed serially)
  }
}

} // namespace internal
#endif // SIMDJSON_THREADS_ENABLED

/**
 * @private Experimental API (subject to change).
 *
 * Parse a stream of JSON documents across multiple threads, extracting a value
 * of type `T` from each document.
 *
 * `extract` is invoked as `extract(doc, out)` where `doc` is the current
 * document (usable exactly as in an `iterate_many` loop) and `out` is a `T&` to
 * fill. It must return `SUCCESS` to keep the value or an error to skip it:
 *
 *     struct point { double x, y, z; };
 *     auto extract = [](auto doc, point &out) -> error_code {
 *       if (auto e = doc["x"].get_double().get(out.x)) { return e; }
 *       if (auto e = doc["y"].get_double().get(out.y)) { return e; }
 *       return doc["z"].get_double().get(out.z);
 *     };
 *     auto result = simdjson::experimental::parse_many_parallel<point>(json, extract);
 *
 * @tparam T       The extracted value type.
 * @tparam F       The extractor callable, `error_code(auto doc, T&)`.
 * @param json     The padded input buffer (see iterate_many for padding rules).
 * @param extract  The per-document extractor.
 * @param options  Threading / slicing options.
 * @return A parallel_stream_result<T> holding one vector per worker.
 *
 * When SIMDJSON_THREADS_ENABLED is not defined, this falls back to a
 * single-threaded implementation producing a single shard.
 */
template <typename T, typename F>
parallel_stream_result<T>
parse_many_parallel(padded_string_view json, F &&extract,
                    parallel_stream_options options = {}) {
  parallel_stream_result<T> result;
  const char *const data = json.data();
  const size_t length = json.size();

#ifdef SIMDJSON_THREADS_ENABLED
  size_t worker_count = options.threads;
  if (worker_count == 0) {
    unsigned hardware = std::thread::hardware_concurrency();
    worker_count = hardware > 1 ? size_t(hardware) : 1;
  }
  const size_t slice_bytes =
      options.slice_bytes ? options.slice_bytes : (1u << 20);

  const stream_format format = options.format;

  result._shards.resize(worker_count);
  std::vector<size_t> local_errors(worker_count, 0);
  std::vector<error_code> local_first(worker_count, SUCCESS);

  // Each worker claims its own byte range from this counter and snaps both ends
  // to a delimiter itself. Slicing needs no state from the preceding slice, so
  // there is nothing for a dedicated dispatcher thread to know that a worker
  // does not, and the pool spawns exactly `worker_count` threads.
  std::atomic<size_t> cursor{0};

  std::vector<std::thread> workers;
  workers.reserve(worker_count);
  for (size_t w = 0; w < worker_count; w++) {
    workers.emplace_back([&, w] {
      ondemand::parser parser;
      parser.threaded = false; // this worker's parser must stay single-threaded
      std::vector<T> &shard = result._shards[w];
      size_t errors = 0;
      error_code first = SUCCESS;

      for (;;) {
        const size_t raw =
            cursor.fetch_add(slice_bytes, std::memory_order_relaxed);
        if (raw >= length) { break; }
        const size_t begin =
            (raw == 0) ? 0
                       : internal::snap_to_boundary(data, length, raw, format);
        const size_t end = internal::snap_to_boundary(data, length,
                                                      raw + slice_bytes, format);
        // Two claims can land inside one long document; the earlier one covers
        // it and this slice is empty.
        if (begin >= end) { continue; }
        const size_t slice_length = end - begin;
        ondemand::document_stream stream;
        error_code slice_error =
            parser.iterate_many(data + begin, slice_length, slice_length, format)
                .get(stream);
        if (slice_error) {
          errors++;
          if (!first) { first = slice_error; }
          continue;
        }
        for (auto it = stream.begin(); it != stream.end(); ++it) {
          auto doc = *it;
          T out;
          error_code extract_error = extract(doc, out);
          if (extract_error) {
            errors++;
            if (!first) { first = extract_error; }
            continue;
          }
          shard.push_back(std::move(out));
        }
      }
      local_errors[w] = errors;
      local_first[w] = first;
    });
  }

  for (std::thread &worker : workers) { worker.join(); }

  for (size_t w = 0; w < worker_count; w++) {
    result._errors += local_errors[w];
    if (!result._first_error) { result._first_error = local_first[w]; }
  }
#else  // SIMDJSON_THREADS_ENABLED
  const size_t slice_bytes =
      options.slice_bytes ? options.slice_bytes : (1u << 20);
  result._shards.resize(1);
  ondemand::parser parser;
  ondemand::document_stream stream;
  error_code stream_error =
      parser.iterate_many(data, length, slice_bytes, options.format).get(stream);
  if (stream_error) {
    result._errors = 1;
    result._first_error = stream_error;
    return result;
  }
  for (auto it = stream.begin(); it != stream.end(); ++it) {
    auto doc = *it;
    T out;
    error_code extract_error = extract(doc, out);
    if (extract_error) {
      result._errors++;
      if (!result._first_error) { result._first_error = extract_error; }
      continue;
    }
    result._shards[0].push_back(std::move(out));
  }
#endif // SIMDJSON_THREADS_ENABLED

  return result;
}

} // namespace experimental
} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_PARALLEL_H
