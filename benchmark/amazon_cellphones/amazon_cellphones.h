#pragma once

#include "json_benchmark/file_runner.h"

namespace amazon_cellphones {

using namespace json_benchmark;

template<typename StringType>
struct brand {
    StringType brand_name;
    double total_rating;
    size_t count{1};
    template<typename OtherStringType>
    simdjson_really_inline bool operator==(const brand<OtherStringType> &other) const {
        return brand_name == other.brand_name &&
            total_rating == other.total_rating &&
            count == other.count;
    }
    template<typename OtherStringType>
    simdjson_really_inline bool operator!=(const brand<OtherStringType> &other) const { return !(*this == other); }
};

template<typename StringType>
simdjson_unused static std::ostream &operator<<(std::ostream &o, const brand<StringType> &b) {
  o << "brand_name: " << b.brand_name << std::endl;
  o << "total_rating: " << b.total_rating << std::endl;
  o << "count: " << b.count << std::endl;
  return o;
}

template<typename StringType>
simdjson_unused static std::ostream &operator<<(std::ostream &o, const std::map<std::string_view, brand<StringType>> &result) {
	for (auto p : result) {
		o << "Brand: "<< p.first << std::endl;
		o << p.second;
	}
  return o;
}

template<typename I>
struct runner : public file_runner<I> {
    std::map<typename I::StringType ,brand<typename I::StringType>> result{};

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

struct simdjson_dom;

template<typename I> simdjson_really_inline static void amazon_cellphones(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

}   // namespace amazon_cellphones