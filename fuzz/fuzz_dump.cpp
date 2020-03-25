#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "NullBuffer.h"

// from the README on the front page
void compute_dump(simdjson::ParsedJson::Iterator &pjh) {
  NulOStream os;

  if (pjh.is_object()) {
    os << "{";
    if (pjh.down()) {
      pjh.print(os); // must be a string
      os << ":";
      pjh.next();
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        os << ",";
        pjh.print(os);
        os << ":";
        pjh.next();
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    os << "}";
  } else if (pjh.is_array()) {
    os << "[";
    if (pjh.down()) {
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        os << ",";
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    os << "]";
  } else {
    pjh.print(os); // just print the lone value
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  try {
    auto pj = simdjson::build_parsed_json(Data, Size);
    if (!pj.is_valid()) {
      throw 1;
    }
    simdjson::ParsedJson::Iterator pjh(pj);
    if (pjh.is_ok()) {
      compute_dump(pjh);
    }
  } catch (...) {
  }
  return 0;
}
