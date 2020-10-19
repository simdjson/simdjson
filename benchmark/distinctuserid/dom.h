#pragma once

#if SIMDJSON_EXCEPTIONS

#include "distinctuserid.h"

namespace distinct_user_id {

using namespace simdjson;


simdjson_really_inline void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::element element);
void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::array array) {
  for (auto child : array) {
    simdjson_recurse(v, child);
  }
}
void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::object object) {
  for (auto [key, value] : object) {
    if((key.size() == 4) && (memcmp(key.data(), "user", 4) == 0)) {
      // we are in an object under the key "user"
      simdjson::error_code error;
      simdjson::dom::object child_object;
      simdjson::dom::object child_array;
      if (not (error = value.get(child_object))) {
        for (auto [child_key, child_value] : child_object) {
          if((child_key.size() == 2) && (memcmp(child_key.data(), "id", 2) == 0)) {
            int64_t x;
            if (not (error = child_value.get(x))) {
              v.push_back(x);
            }
          }
          simdjson_recurse(v, child_value);
        }
      } else if (not (error = value.get(child_array))) {
        simdjson_recurse(v, child_array);
      }
      // end of: we are in an object under the key "user"
    } else {
      simdjson_recurse(v, value);
    }
  }
}
simdjson_really_inline void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::element element) {
  simdjson_unused simdjson::error_code error;
  simdjson::dom::array array;
  simdjson::dom::object object;
  if (not (error = element.get(array))) {
    simdjson_recurse(v, array);
  } else if (not (error = element.get(object))) {
    simdjson_recurse(v, object);
  }
}





class Dom {
public:
  simdjson_really_inline bool Run(const padded_string &json);
  simdjson_really_inline const std::vector<int64_t> &Result() { return ids; }
  simdjson_really_inline size_t ItemCount() { return ids.size(); }

private:
  dom::parser parser{};
  std::vector<int64_t> ids{};

};
void print_vec(const std::vector<int64_t> &v) {
  for (auto i : v) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

simdjson_really_inline bool Dom::Run(const padded_string &json) {
  ids.clear();
  dom::element doc = parser.parse(json);
  simdjson_recurse(ids, doc);
  remove_duplicates(ids);
  return true;
}

BENCHMARK_TEMPLATE(DistinctUserID, Dom);

} // namespace distinct_user_id

#endif // SIMDJSON_EXCEPTIONS