#pragma once

#if SIMDJSON_EXCEPTIONS

//
// Interface
//

namespace kostya {
template<typename T> static void Kostya(benchmark::State &state);
namespace sum {
template<typename T> static void KostyaSum(benchmark::State &state);
}

using namespace simdjson;

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

static const padded_string &get_built_json_array() {
  static padded_string json = build_json_array(524288);
  return json;
}

struct my_point {
  double x;
  double y;
  double z;
  simdjson_really_inline bool operator==(const my_point &other) const {
    return x == other.x && y == other.y && z == other.z;
  }
  simdjson_really_inline bool operator!=(const my_point &other) const { return !(*this == other); }
};

simdjson_unused static std::ostream &operator<<(std::ostream &o, const my_point &p) {
  return o << p.x << "," << p.y << "," << p.z << std::endl;
}

} // namespace kostya

//
// Implementation
//
#include <vector>
#include "event_counter.h"
#include "dom.h"
#include "json_benchmark.h"

namespace kostya {

template<typename T> static void Kostya(benchmark::State &state) {
  JsonBenchmark<T, Dom>(state, get_built_json_array());
}

namespace sum {
template<typename T> static void KostyaSum(benchmark::State &state) {
  JsonBenchmark<T, Dom>(state, get_built_json_array());
}
}

} // namespace kostya

#endif // SIMDJSON_EXCEPTIONS
