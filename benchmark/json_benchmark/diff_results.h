#pragma once

#include <vector>
#include <map>
#include <sstream>
#include <limits>

namespace json_benchmark {

enum class diff_flags {
  NONE = 0,
  IMPRECISE_FLOATS = 1
};

template<typename T, typename U>
static bool diff_results(benchmark::State &state, const T &result, const U &reference, diff_flags flags);

template<typename T, typename U>
struct result_differ {
  static bool diff(benchmark::State &state, const T &result, const U &reference, diff_flags flags) {
    if (result != reference) {
      std::stringstream str;
      str << "result incorrect: " << result << " ... reference: " << reference;
      state.SkipWithError(str.str().data());
      return false;
    }
    return true;
  }
};

template<typename T, typename U>
struct result_differ<std::vector<T>, std::vector<U>> {
  static bool diff(benchmark::State &state, const std::vector<T> &result, const std::vector<U> &reference, diff_flags flags) {
    auto result_iter = result.begin();
    auto reference_iter = reference.begin();
    while (result_iter != result.end() && reference_iter != reference.end()) {
      if (!diff_results(state, *result_iter, *reference_iter, flags)) { return false; }
      result_iter++;
      reference_iter++;
    }
    if (result_iter != result.end()) {
      std::stringstream str;
      str << "extra results (got " << result.size() << ", expected " << reference.size() << "): first extra element: " << *result_iter;
      state.SkipWithError(str.str().data());
      return false;
    } else if (reference_iter != reference.end()) {
      std::stringstream str;
      str << "missing results (got " << result.size() << ", expected " << reference.size() << "): first missing element: " << *reference_iter;
      state.SkipWithError(str.str().data());
      return false;
    }
    return true;
  }
};

template<typename T, typename U, typename StringType>
struct result_differ<std::map<StringType,T>, std::map<StringType,U>> {
  static bool diff(benchmark::State &state, const std::map<StringType,T> &result, const std::map<StringType,U> &reference, diff_flags flags) {
    auto result_iter = result.begin();
    auto reference_iter = reference.begin();
    while (result_iter != result.end() && reference_iter != reference.end()) {
      if (!diff_results(state, *result_iter, *reference_iter, flags)) { return false; }
      result_iter++;
      reference_iter++;
    }
    if (result_iter != result.end()) {
      std::stringstream str;
      str << "extra results (got " << result.size() << ", expected " << reference.size() << "): first extra element: " << *result_iter;
      state.SkipWithError(str.str().data());
      return false;
    } else if (reference_iter != reference.end()) {
      std::stringstream str;
      str << "missing results (got " << result.size() << ", expected " << reference.size() << "): first missing element: " << *reference_iter;
      state.SkipWithError(str.str().data());
      return false;
    }
    return true;
  }
};

template<typename T, typename U>
struct result_differ<std::map<std::string_view,T>, std::vector<std::string_view,U>> {
  static bool diff(benchmark::State &state, const std::map<std::string_view,T> &result, const std::map<std::string_view,U> &reference, diff_flags flags) {
    auto result_iter = result.begin();
    auto reference_iter = reference.begin();
    while (result_iter != result.end() && reference_iter != reference.end()) {
      if (!diff_results(state, *result_iter, *reference_iter, flags)) { return false; }
      result_iter++;
      reference_iter++;
    }
    if (result_iter != result.end()) {
      std::stringstream str;
      str << "extra results (got " << result.size() << ", expected " << reference.size() << "): first extra element: " << *result_iter;
      state.SkipWithError(str.str().data());
      return false;
    } else if (reference_iter != reference.end()) {
      std::stringstream str;
      str << "missing results (got " << result.size() << ", expected " << reference.size() << "): first missing element: " << *reference_iter;
      state.SkipWithError(str.str().data());
      return false;
    }
    return true;
  }
};

template<>
struct result_differ<double, double> {
  static bool diff(benchmark::State &state, const double &result, const double &reference, diff_flags flags) {
    bool different;
    if (int(flags) & int(diff_flags::IMPRECISE_FLOATS)) {
      different = f64_ulp_dist(result, reference) > 1;
    } else {
      different = result != reference;
    }
    if (different) {
      std::stringstream str;
      // We print it out using full precision.
      constexpr auto precision = std::numeric_limits<double>::max_digits10;
      str << std::setprecision(precision);
      str << "incorrect double result: " << std::endl;
      str << "   result: " << std::left << std::setw(precision+2) << result    << " (hexfloat " << std::hexfloat << result    << ")" << std::defaultfloat << std::endl;
      str << "reference: " << std::left << std::setw(precision+2) << reference << " (hexfloat " << std::hexfloat << reference << ")" << std::defaultfloat << std::endl;
      state.SkipWithError(str.str().data());
    }
    return true;
  }

  static uint64_t f64_ulp_dist(double a, double b) {
    uint64_t ua, ub;
    std::memcpy(&ua, &a, sizeof(ua));
    std::memcpy(&ub, &b, sizeof(ub));
    if ((int64_t)(ub ^ ua) >= 0)
      return (int64_t)(ua - ub) >= 0 ? (ua - ub) : (ub - ua);
    return ua + ub + 0x80000000;
  }
};

template<typename T, typename U>
static bool diff_results(benchmark::State &state, const T &result, const U &reference, diff_flags flags) {
  return result_differ<T, U>::diff(state, result, reference, flags);
}

} // namespace json_benchmark
