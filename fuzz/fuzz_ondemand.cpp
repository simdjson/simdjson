#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

std::vector<std::string_view> split(const char *Data, size_t Size) {

  std::vector<std::string_view> ret;

    using namespace std::literals;
    constexpr auto sep="\n~~\n"sv;

    std::string_view all(Data,Size);
    auto pos=all.find(sep);
    while(pos!=std::string_view::npos) {
      ret.push_back(all.substr(0,pos));
      all=all.substr(pos+sep.size());
      pos=all.find(sep);
    }
    ret.push_back(all);
    return ret;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  std::vector<std::string_view> strings=split(static_cast<const char*>(static_cast<const void*>(Data)),Size);

  if(strings.size()<2) {
    return 0;
  }
#if SIMDJSON_EXCEPTIONS
  try {
#endif
    simdjson::builtin::ondemand::parser parser;
    simdjson::padded_string padded(strings[0]);
    auto doc = parser.iterate(padded);
    if(doc.error()) {
      return 0;
    }
    for (auto item : doc/*[strings[1]]*/) {
      simdjson_unused auto str=item.get_string();
    }
#if SIMDJSON_EXCEPTIONS
  } catch (...) {
  }
#endif

  return 0;
}
