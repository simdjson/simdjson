#pragma once

#include "json_benchmark/file_runner.h"
#include <map>
#include <string>


namespace amazon_cellphones {

const bool UNTHREADED = false;
const bool THREADED = true;

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
struct runner : public file_runner<I> {
    std::map<typename I::StringType, brand> result{};

    bool setup(benchmark::State &state) {
        return this->load_json(state, AMAZON_CELLPHONES_NDJSON);
    }

    bool before_run(benchmark::State &state) {
        if (!file_runner<I>::before_run(state)) { return false; }
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

template<bool threaded>
struct simdjson_dom;

template<typename I> simdjson_inline static void amazon_cellphones(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_dom<UNTHREADED>>>(state);
}

}   // namespace amazon_cellphones
