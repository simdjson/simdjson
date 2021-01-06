#pragma once

#include "json_benchmark/string_runner.h"
#include <random>

namespace large_random {

static const simdjson::padded_string &get_built_json_array();

struct point {
  double x;
  double y;
  double z;
};

simdjson_unused static std::ostream &operator<<(std::ostream &o, const point &p) {
  return o << p.x << "," << p.y << "," << p.z << std::endl;
}

template<typename I>
struct runner : public json_benchmark::string_runner<I> {
  std::vector<point> result;

  runner() : json_benchmark::string_runner<I>(get_built_json_array()) {}

  bool before_run(benchmark::State &state) {
    if (!json_benchmark::string_runner<I>::before_run(state)) { return false; }
    result.clear();
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, result);
  }

  template<typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return json_benchmark::diff_results(state, result, reference.result);
  }

  size_t items_per_iteration() {
    return result.size();
  }
};

static std::string build_json_array(size_t N) {
  std::default_random_engine e;
  std::uniform_real_distribution<> dis(0, 1);
  std::stringstream myss;
  myss << "[" << std::endl;
  if(N > 0) {
    myss << "{ \"x\":" << dis(e) << ",  \"y\":" << dis(e) << ", \"z\":" << dis(e) << "}" << std::endl;
  }
  for(size_t i = 1; i < N; i++) {
    myss << "," << std::endl;
    myss << "{ \"x\":" << dis(e) << ",  \"y\":" << dis(e) << ", \"z\":" << dis(e) << "}";
  }
  myss << std::endl;
  myss << "]" << std::endl;
  std::string answer = myss.str();
  std::cout << "Creating a source file spanning " << (answer.size() + 512) / 1024 << " KB " << std::endl;
  return answer;
}

static const simdjson::padded_string &get_built_json_array() {
  static simdjson::padded_string json = build_json_array(1000000);
  return json;
}

struct simdjson_dom;

template<typename T> static void large_random(benchmark::State &state) {
  json_benchmark::run_json_benchmark<runner<T>, runner<simdjson_dom>>(state);
}

} // namespace large_random

namespace json_benchmark {
  template<>
  bool result_differ<large_random::point, large_random::point>::diff(benchmark::State &state, const large_random::point &result, const large_random::point &reference) {
    return diff_results(state, result.x, reference.x)
        && diff_results(state, result.y, reference.y)
        && diff_results(state, result.z, reference.z);
  }
}
