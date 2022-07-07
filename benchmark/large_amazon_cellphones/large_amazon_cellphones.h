#pragma once

#include "json_benchmark/string_runner.h"
#include <map>
#include <string>

namespace large_amazon_cellphones {

const bool UNTHREADED = false;
const bool THREADED = true;

static const simdjson::padded_string &get_built_json();

using namespace json_benchmark;

struct brand {
    double cumulative_rating;
    uint64_t reviews_count;
    simdjson_inline bool operator==(const brand &other) const {
        return cumulative_rating == other.cumulative_rating &&
            reviews_count == other.reviews_count;
    }
    simdjson_inline bool operator!=(const brand &other) const { return !(*this == other); }
};

simdjson_unused static std::ostream &operator<<(std::ostream &o, const brand &b) {
  o << "cumulative_rating: " << b.cumulative_rating << std::endl;
  o << "reviews_count: " << b.reviews_count << std::endl;
  return o;
}

template<typename StringType>
simdjson_unused static std::ostream &operator<<(std::ostream &o, const std::pair<const StringType, brand> &p) {
  o << "brand: " << p.first << std::endl;
	o << p.second;
  return o;
}

template<typename I>
struct runner : public string_runner<I> {
    std::map<typename I::StringType, brand> result{};

    runner() : string_runner<I>(get_built_json()) {}

    bool before_run(benchmark::State &state) {
    if (!string_runner<I>::before_run(state)) { return false; }
    result.clear();
    return true;
  }

    bool run(benchmark::State &) {
        return this->implementation.run(this->json, result);
    }

    template<typename R>
    bool diff(benchmark::State &state, runner<R> &reference) {
        return diff_results(state, result, reference.result, diff_flags::NONE);
    }

    size_t items_per_iteration() {
        return result.size();
    }
};

static std::string build_json(size_t N) {
  std::ifstream in(AMAZON_CELLPHONES_NDJSON);
  std::string answer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  // Find position of first line to exclude it in further copies
  size_t first_line = answer.find('\n');
  std::string copy(answer,first_line + 1);
  size_t count{1};

  while (answer.size() < N) {
    answer.append(copy);
    count++;
  }

  std::cout << "Creating a source file spanning " << (answer.size() + 512) / (1024*1024) << " MB (" << count << " copies of original file)" << std::endl;
  return answer;
}

static const simdjson::padded_string &get_built_json() {
  static simdjson::padded_string json = build_json(10*1024*1024);
  return json;
}

template<bool threaded>
struct simdjson_dom;

template<typename I> simdjson_inline static void large_amazon_cellphones(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_dom<UNTHREADED>>>(state);
}

}   // namespace large_amazon_cellphones
