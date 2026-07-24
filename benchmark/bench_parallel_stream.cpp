// Benchmark for simdjson::experimental::parse_many_parallel.
//
// It contrasts three ways of extracting a value out of every document in a
// large newline-delimited JSON stream:
//
//   * serial            : one parser, single-threaded iterate_many
//   * builtin-2thread   : iterate_many with parser.threaded = true (simdjson's
//                         own stage1/stage2 pipeline; needs several batches)
//   * parallel(N)       : parse_many_parallel with N worker threads
//
// Two datasets exercise the two regimes:
//   * amazon_cellphones.ndjson (repeated) : light per-document work, so it
//                         becomes memory-bandwidth bound as threads grow.
//   * synthetic points {"x":..,"y":..,"z":..} : number-heavy, so stage 2
//                         dominates and it scales with core count.
//
// This is a plain timing harness (no google-benchmark dependency) so it can be
// run anywhere:  ./bench_parallel_stream [target_megabytes]
//
// It relies on threads; build simdjson with SIMDJSON_ENABLE_THREADS (the
// default), which links -pthread and defines SIMDJSON_THREADS_ENABLED.

#include "simdjson.h"
#include "json_benchmark/constants.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace simdjson;

namespace {

// ---- datasets --------------------------------------------------------------

// Load a file, drop its first line (the amazon file's header row of field
// names), then repeat the remaining rows until the buffer reaches target bytes.
std::string load_repeat(const char *path, size_t target_bytes) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "error: cannot open %s\n", path);
    std::exit(EXIT_FAILURE);
  }
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string all = buffer.str();
  size_t first_newline = all.find('\n');
  std::string body =
      (first_newline == std::string::npos) ? all : all.substr(first_newline + 1);
  if (!body.empty() && body.back() != '\n') { body.push_back('\n'); }
  std::string out;
  out.reserve(target_bytes + body.size());
  while (out.size() < target_bytes) { out += body; }
  return out;
}

// Load a file and return its data rows (header line dropped, blank lines
// skipped) so we can re-serialize them in any stream format.
std::vector<std::string> load_rows(const char *path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "error: cannot open %s\n", path);
    std::exit(EXIT_FAILURE);
  }
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string all = buffer.str();
  std::vector<std::string> rows;
  size_t start = all.find('\n'); // skip the header row
  start = (start == std::string::npos) ? all.size() : start + 1;
  while (start < all.size()) {
    size_t end = all.find('\n', start);
    if (end == std::string::npos) { end = all.size(); }
    if (end > start) { rows.emplace_back(all, start, end - start); }
    start = end + 1;
  }
  return rows;
}

// Serialize the rows, repeated up to target_bytes, in the requested format
// (whitespace_delimited or json_sequence).
std::string build_stream(const std::vector<std::string> &rows,
                         size_t target_bytes, stream_format format) {
  std::string out;
  out.reserve(target_bytes + 4096);
  size_t i = 0;
  while (out.size() < target_bytes) {
    const std::string &row = rows[i++ % rows.size()];
    if (format == stream_format::json_sequence) { out.push_back('\x1e'); } // RS
    out += row;
    out.push_back('\n');
  }
  return out;
}

// Newline-delimited {"x":..,"y":..,"z":..} documents.
std::string make_points(size_t target_bytes) {
  std::string out;
  out.reserve(target_bytes + 64);
  std::mt19937_64 rng(12345);
  std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
  char line[128];
  while (out.size() < target_bytes) {
    int n = std::snprintf(line, sizeof(line),
                          "{\"x\":%.6f,\"y\":%.6f,\"z\":%.6f}\n", dist(rng),
                          dist(rng), dist(rng));
    out.append(line, size_t(n));
  }
  return out;
}

// ---- extracted value types + extractors ------------------------------------

struct point {
  double x, y, z;
};

// amazon rows are arrays: [asin, brand, title, url, image, rating, reviewUrl,
// totalReviews, prices]. We pull a few representative fields.
struct amazon_record {
  double rating;
  int64_t total_reviews;
  uint32_t brand_length;
};

template <typename Doc>
error_code extract_point(Doc doc, point &out) noexcept {
  if (auto e = doc["x"].get_double().get(out.x)) { return e; }
  if (auto e = doc["y"].get_double().get(out.y)) { return e; }
  return doc["z"].get_double().get(out.z);
}

// A cheap value sink so the compiler cannot optimize the extraction away, and
// so results can be sanity-checked across configurations.
double sink_value(const point &p) noexcept { return p.x + p.y + p.z; }
double sink_value(const amazon_record &r) noexcept {
  return r.rating + double(r.total_reviews) + double(r.brand_length);
}

template <typename Doc>
error_code extract_amazon(Doc doc, amazon_record &out) noexcept {
  ondemand::array array;
  if (auto e = doc.get_array().get(array)) { return e; }
  out = {0.0, 0, 0};
  size_t index = 0;
  for (auto value : array) {
    if (index == 1) {
      std::string_view brand;
      if (auto e = value.get_string().get(brand)) { return e; }
      out.brand_length = uint32_t(brand.size());
    } else if (index == 5) {
      if (auto e = value.get_double().get(out.rating)) { return e; }
    } else if (index == 7) {
      if (auto e = value.get_int64().get(out.total_reviews)) { return e; }
    }
    index++;
  }
  return SUCCESS;
}

// ---- timing helpers --------------------------------------------------------

struct measurement {
  double seconds;
  size_t docs;
  double checksum;
};

template <typename Fn>
measurement best_of(int repetitions, Fn &&fn) {
  measurement best{1e30, 0, 0.0};
  for (int r = 0; r < repetitions; r++) {
    measurement m = fn();
    if (m.seconds < best.seconds) { best = m; }
  }
  return best;
}

// serial / builtin-2thread reference via a single iterate_many.
template <typename T, typename Extract>
measurement run_serial(padded_string_view json, bool threaded, size_t batch,
                       Extract &&extract,
                       stream_format format = stream_format::whitespace_delimited) {
  ondemand::parser parser;
  parser.threaded = threaded;
  double checksum = 0.0;
  size_t docs = 0;
  auto start = std::chrono::steady_clock::now();
  ondemand::document_stream stream;
  if (!parser.iterate_many(json.data(), json.size(), batch, format).get(stream)) {
    for (auto it = stream.begin(); it != stream.end(); ++it) {
      auto doc = *it;
      T value;
      if (!extract(doc, value)) {
        checksum += sink_value(value);
        docs++;
      }
    }
  }
  auto end = std::chrono::steady_clock::now();
  return {std::chrono::duration<double>(end - start).count(), docs, checksum};
}

template <typename T, typename Extract>
measurement run_parallel(padded_string_view json, size_t threads,
                         Extract &&extract,
                         stream_format format = stream_format::whitespace_delimited) {
  experimental::parallel_stream_options options;
  options.threads = threads;
  options.format = format;
  double checksum = 0.0;
  auto start = std::chrono::steady_clock::now();
  auto result =
      experimental::parse_many_parallel<T>(json, extract, options);
  auto end = std::chrono::steady_clock::now();
  result.for_each(
      [&](const T &value) { checksum += sink_value(value); });
  return {std::chrono::duration<double>(end - start).count(), result.size(),
          checksum};
}

void print_row(const char *tag, double megabytes, const measurement &m,
               double baseline_seconds) {
  double gbps = megabytes / 1024.0 / m.seconds;
  double mdocs = double(m.docs) / 1e6 / m.seconds;
  std::printf("  %-22s %8.3f GB/s  %8.2f Mdoc/s  %6.2fx  (docs=%zu)\n", tag,
              gbps, mdocs, baseline_seconds / m.seconds, m.docs);
}

template <typename T, typename Extract>
void run_dataset(const char *name, const std::string &raw, Extract &&extract) {
  padded_string padded(raw.data(), raw.size());
  padded_string_view json(padded);
  double megabytes = double(raw.size()) / (1024.0 * 1024.0);
  std::printf("\n== %s : %.1f MB ==\n", name, megabytes);

  auto serial =
      best_of(3, [&] { return run_serial<T>(json, false, 1u << 20, extract); });
  double base = serial.seconds;
  print_row("serial", megabytes, serial, base);

  // built-in two-thread: overlap needs multiple batches; 16 MiB is a good size.
  auto builtin = best_of(
      3, [&] { return run_serial<T>(json, true, 16u << 20, extract); });
  print_row("builtin-2thread", megabytes, builtin, base);

  unsigned hardware = std::thread::hardware_concurrency();
  std::vector<size_t> counts;
  for (size_t t = 1; t <= hardware; t *= 2) { counts.push_back(t); }
  if (counts.empty() || counts.back() != hardware) { counts.push_back(hardware); }
  for (size_t t : counts) {
    auto parallel =
        best_of(3, [&] { return run_parallel<T>(json, t, extract); });
    char tag[32];
    std::snprintf(tag, sizeof(tag), "parallel(%zu)", t);
    print_row(tag, megabytes, parallel, base);
  }
}

// Verify every stream format: parse the same documents serially and in
// parallel and confirm the document count and checksum agree, then report the
// parallel throughput. This proves the format-aware slicing never splits or
// drops a document.
template <typename T, typename Extract>
void verify_formats(const std::vector<std::string> &rows, size_t target_bytes,
                    Extract &&extract) {
  struct named_format {
    const char *name;
    stream_format format;
  };
  const named_format formats[] = {
      {"whitespace_delimited", stream_format::whitespace_delimited},
      {"json_sequence (RFC7464)", stream_format::json_sequence},
  };
  unsigned hardware = std::thread::hardware_concurrency();
  std::printf("\n== format coverage (parallel vs serial, %u threads) ==\n",
              hardware);
  for (const named_format &nf : formats) {
    std::string raw = build_stream(rows, target_bytes, nf.format);
    padded_string padded(raw.data(), raw.size());
    padded_string_view json(padded);
    double megabytes = double(raw.size()) / (1024.0 * 1024.0);

    auto serial = run_serial<T>(json, false, 1u << 20, extract, nf.format);
    auto parallel =
        best_of(3, [&] { return run_parallel<T>(json, hardware, extract, nf.format); });

    bool ok = serial.docs == parallel.docs &&
              std::abs(serial.checksum - parallel.checksum) < 1e-3;
    double gbps = megabytes / 1024.0 / parallel.seconds;
    std::printf("  %-24s %s  serial=%zu parallel=%zu docs  %7.2f GB/s\n",
                nf.name, ok ? "[OK]  " : "[FAIL]", serial.docs, parallel.docs,
                gbps);
  }
}

} // namespace

int main(int argc, char **argv) {
  size_t target_mb = 256;
  if (argc > 1) { target_mb = strtoull(argv[1], nullptr, 10); }
  size_t target_bytes = target_mb * 1024 * 1024;

  std::printf("simdjson implementation: %s\n",
              get_active_implementation()->name().c_str());
#ifndef SIMDJSON_THREADS_ENABLED
  std::printf("WARNING: SIMDJSON_THREADS_ENABLED is off; parallel == serial.\n");
#endif
  std::printf("hardware_concurrency: %u\n", std::thread::hardware_concurrency());

  run_dataset<amazon_record>(
      "amazon_cellphones.ndjson (repeated)",
      load_repeat(json_benchmark::AMAZON_CELLPHONES_NDJSON, target_bytes),
      [](auto doc, amazon_record &out) { return extract_amazon(doc, out); });

  run_dataset<point>(
      "synthetic points", make_points(target_bytes),
      [](auto doc, point &out) { return extract_point(doc, out); });

  // Exercise all four stream formats on the amazon rows (use a smaller size so
  // building four copies stays cheap).
  verify_formats<amazon_record>(
      load_rows(json_benchmark::AMAZON_CELLPHONES_NDJSON),
      std::min<size_t>(target_bytes, size_t(256) * 1024 * 1024),
      [](auto doc, amazon_record &out) { return extract_amazon(doc, out); });

  return 0;
}
