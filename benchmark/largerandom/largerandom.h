#pragma once

#if SIMDJSON_EXCEPTIONS

//
// Interface
//

namespace largerandom {
template<typename T> static void LargeRandom(benchmark::State &state);
namespace sum {
template<typename T> static void LargeRandomSum(benchmark::State &state);
}

using namespace simdjson;

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

static const padded_string &get_built_json_array() {
  static padded_string json = build_json_array(1000000);
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

} // namespace largerandom

//
// Implementation
//
#include <vector>
#include "event_counter.h"
#include "dom.h"
#include "json_benchmark.h"

namespace largerandom {

template<typename T> static void LargeRandom(benchmark::State &state) {
  JsonBenchmark<T, Dom>(state, get_built_json_array());
}

namespace sum {

template<typename T> static void LargeRandomSum(benchmark::State &state) {
  JsonBenchmark<T, Dom>(state, get_built_json_array());
}

}
} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
