#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#else
#include <unistd.h>
#endif
#include "simdjson.h"
#include <cstdio>
// Returns the default size of the page in bytes on this system.
long page_size() {
#ifdef _WIN32
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  long pagesize = sysInfo.dwPageSize;
#else
  long pagesize = sysconf(_SC_PAGESIZE);
#endif
  return pagesize;
}

// Returns true if the buffer + len + simdjson::SIMDJSON_PADDING crosses the
// page boundary.
bool need_allocation(const char *buf, size_t len) {
  return ((reinterpret_cast<uintptr_t>(buf + len - 1) % page_size())
    + simdjson::SIMDJSON_PADDING > static_cast<uintptr_t>(page_size()));
}

bool check_need_allocation() {
  long ps = page_size();
  std::vector<std::tuple<uintptr_t,size_t,bool>> test_cases = {
    {2*ps, 5, false}, {2*ps, ps, true}, {2*ps, 100, false},
  };
  for(auto t : test_cases) {
    uintptr_t buf = std::get<0>(t);
    size_t len = std::get<1>(t);
    bool expected = std::get<2>(t);
    if(need_allocation(reinterpret_cast<const char *>(buf), len) != expected) {
      return false;
    }
  }
  return true;
}

simdjson::padded_string_view
get_padded_string_view(const char *buf, size_t len,
                       simdjson::padded_string &jsonbuffer) {
  if (need_allocation(buf, len)) { // unlikely case
    jsonbuffer = simdjson::padded_string(buf, len);
    return jsonbuffer;
  } else { // no reallcation needed (very likely)
    return simdjson::padded_string_view(buf, len,
                                            len + simdjson::SIMDJSON_PADDING);
  }
}

int main() {
  printf("page_size: %ld\n", page_size());
  if(!check_need_allocation()) {
    printf("Bug in the need_allocation function.\n");
    return EXIT_FAILURE;
  }
  const char *jsonpoiner = R"(
        {
            "key": "value"
        }
    )";
  size_t len = strlen(jsonpoiner);
  simdjson::padded_string jsonbuffer; // only allocate if needed
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  simdjson::error_code error =
      parser.iterate(get_padded_string_view(jsonpoiner, len, jsonbuffer))
          .get(doc);
  if (error) {
    printf("error: %s\n", simdjson::error_message(error));
    return EXIT_FAILURE;
  }
  std::string_view value;
  error = doc["key"].get_string().get(value);
  if (error) {
    return EXIT_FAILURE;
  }
  printf("Value: \"%.*s\"\n", (int)value.size(), value.data());
  if (value != "value") {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}