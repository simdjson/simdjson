#include "simdjson.h"
#include <cstdio>

#ifdef _MSC_VER
#include <sysinfoapi.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "simdjson.h"

// Returns the default size of the page in bytes on this system.
long page_size() {
#ifdef _MSC_VER
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
  return ((reinterpret_cast<uintptr_t>(buf + len - 1) % page_size()) <
          simdjson::SIMDJSON_PADDING);
}

simdjson::padded_string_view
get_padded_string_view(const char *buf, size_t len,
                       simdjson::padded_string &jsonbuffer) {
  simdjson::padded_string_view jsonview;
  if (need_allocation(buf, len)) { // unlikely case
    jsonbuffer = simdjson::padded_string(buf, len);
    jsonview = jsonbuffer;
  } else { // no reallcation needed (very likely)
    jsonview = simdjson::padded_string_view(buf, len,
                                            len + simdjson::SIMDJSON_PADDING);
  }
  return jsonview;
}

int main() {
  printf("page_size: %ld\n", page_size());
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