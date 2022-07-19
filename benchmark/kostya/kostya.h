#pragma once

#include "json_benchmark/string_runner.h"
#include "json_benchmark/point.h"
#include <vector>
#include <random>

namespace kostya {

using namespace json_benchmark;

static const simdjson::padded_string &get_built_json_array();

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

static void append_coordinate(std::default_random_engine &e, std::uniform_real_distribution<> &dis, std::stringstream &myss) {
  using std::endl;
  myss << R"(        {)" << endl;
  myss << R"(            "x": )" << dis(e) << "," << endl;
  myss << R"(            "y": )" << dis(e) << "," << endl;
  myss << R"(            "z": )" << dis(e) << "," << endl;
  myss << R"(            "name": ")" << char('a'+dis(e)*25) << char('a'+dis(e)*25) << char('a'+dis(e)*25) << char('a'+dis(e)*25) << char('a'+dis(e)*25) << char('a'+dis(e)*25) << " " << int(dis(e)*10000) << "\"," << endl;
  myss << R"(            "opts": {)" << endl;
  myss << R"(                "1": [)" << endl;
  myss << R"(                    1,)" << endl;
  myss << R"(                    true)" << endl;
  myss << R"(                ])" << endl;
  myss << R"(            })" << endl;
  myss << R"(        })";
}

static std::string build_json_array(size_t N) {
  using namespace std;
  default_random_engine e;
  uniform_real_distribution<> dis(0, 1);
  stringstream myss;
  myss << R"({)" << endl;
  myss << R"(    "coordinates": [)" << endl;
  for (size_t i=1; i<N; i++) {
    append_coordinate(e, dis, myss); myss << "," << endl;
  }
  append_coordinate(e, dis, myss); myss << endl;
  myss << R"(    ],)" << endl;
  myss << R"(    "info": "some info")" << endl;
  myss << R"(})" << endl;
  string answer = myss.str();
  cout << "Creating a source file spanning " << (answer.size() + 512) / 1024 << " KB " << endl;
  return answer;
}

static const simdjson::padded_string &get_built_json_array() {
  static simdjson::padded_string json = build_json_array(524288);
  return json;
}

struct simdjson_dom;

template<typename I> simdjson_inline static void kostya(benchmark::State &state) {
  run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

} // namespace kostya
