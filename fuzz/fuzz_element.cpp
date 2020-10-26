#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "FuzzUtils.h"
#include "NullBuffer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  FuzzData fd(Data, Size);
  const int action = fd.getInt<0, 31>();

  // there will be some templatized functions like is() which need to be tested
  // on a type. select one dynamically and create a function that will invoke
  // with that type
  const int selecttype=fd.getInt<0,7>();
  auto invoke_with_type=[selecttype](auto cb) {
    using constcharstar=const char*;
    switch(selecttype) {
    case 0: cb(bool{});break;
    case 1: cb(double{});break;
    case 2: cb(uint64_t{});break;
    case 3: cb(int64_t{});break;
    case 4: cb(std::string_view{});break;
    case 5: cb(constcharstar{});break;
    case 6: cb(simdjson::dom::array{});break;
    case 7: cb(simdjson::dom::object{});break;
    }
  };

  const auto index = fd.get<size_t>();

  // split the remainder of the document into strings
  auto strings = fd.splitIntoStrings();
  while (strings.size() < 2) {
    strings.emplace_back();
  }
  const auto str = strings[0];

  // exit if there was too little data
  if (!fd)
    return 0;

  simdjson::dom::parser parser;
  simdjson_unused simdjson::dom::element elem;
  simdjson_unused auto error = parser.parse(strings[1]).get(elem);

  if (error)
    return 0;

#define CASE(num, fun)                                                         \
  case num: {                                                                  \
    simdjson_unused auto v = elem.fun();                                                       \
    break;                                                                     \
  }
#define CASE2(num, fun)                                                        \
  case num: {                                                                  \
    simdjson_unused auto v = elem fun;                                                         \
    break;                                                                     \
  }
#if SIMDJSON_EXCEPTIONS
  try {
#endif

    switch (action) {
      CASE(0, type);
      CASE(1, get_array);
      CASE(2, get_object);
      CASE(3, get_c_str);
      CASE(4, get_string_length);
      CASE(5, get_string);
      CASE(6, get_int64);
      CASE(7, get_uint64);
      CASE(8, get_double);
      CASE(9, get_bool);
      CASE(10, is_array);
      CASE(11, is_object);
      CASE(12, is_string);
      CASE(13, is_int64);
      CASE(14, is_uint64);
      CASE(15, is_double);
      CASE(16, is_number);
      CASE(17, is_bool);
      CASE(18, is_null);
      // element.is<>() :
    case 19: {
        invoke_with_type([&elem](auto t){ simdjson_unused auto v = elem.is<decltype (t)>();  });
      } break;

      // CASE(xx,get);
      case 20: {
          invoke_with_type([&elem](auto t){ simdjson_unused auto v = elem.get<decltype (t)>();  });
        } break;

      // CASE(xx,tie);
      case 21: {
          invoke_with_type([&elem](auto t){
            simdjson::error_code ec;
            simdjson::dom::element{elem}.tie(t,ec);  });
        } break;

#if SIMDJSON_EXCEPTIONS
      // cast to type
      case 22: {
          invoke_with_type([&elem](auto t){
            using T=decltype(t);
            simdjson_unused auto v = static_cast<T>(elem);  });
        } break;

      CASE(23, begin);
      CASE(24, end);
#endif
      CASE2(25, [str]);
      CASE2(26, .at_pointer(str));
      // CASE2(xx,at(str)); deprecated
      CASE2(28, .at(index));
      CASE2(29, .at_key(str));
      CASE2(30, .at_key_case_insensitive(str));
      case 31: { NulOStream os;
        simdjson_unused auto dumpstatus = elem.dump_raw_tape(os);} ;break;
    default:
      return 0;
    }
#undef CASE
#undef CASE2

#if SIMDJSON_EXCEPTIONS
  } catch (std::exception &) {
    // do nothing
  }
#endif

  return 0;
}
