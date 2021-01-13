#pragma once

#include "json_benchmark/string_runner.h"
#include "json_benchmark/point.h"
#include <random>

namespace large_random {

static const simdjson::padded_string &get_built_json_array();

using namespace json_benchmark;

simdjson_unused static std::ostream &operator<<(std::ostream &o, const point &p) {
  return o << p.x << "," << p.y << "," << p.z << std::endl;
}

template<typename I>
struct runner : public string_runner<I> {
  std::vector<point> result;

  runner() : string_runner<I>(get_built_json_array()) {}

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
    return diff_results(state, result, reference.result, I::DiffFlags);
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
  run_json_benchmark<runner<T>, runner<simdjson_dom>>(state);
}

} // namespace large_random
