#include "simdjson/parsedjson.h"

namespace simdjson {

WARN_UNUSED
bool ParsedJson::allocate_capacity(size_t len, size_t max_depth) {
  return doc.allocate_capacity(len, max_depth) && parser.allocate_capacity(len, max_depth);
}

bool ParsedJson::is_valid() const { return doc.is_valid(); }

int ParsedJson::get_error_code() const { return doc.get_error_code(); }

std::string ParsedJson::get_error_message() const {
  return doc.get_error_message();
}

void ParsedJson::deallocate() {
  doc.deallocate();
  parser.deallocate();
}

void ParsedJson::init() {
  parser.init(doc);
}

WARN_UNUSED
bool ParsedJson::print_json(std::ostream &os) const {
  return doc.print_json(os);
}

WARN_UNUSED
bool ParsedJson::dump_raw_tape(std::ostream &os) const {
  return doc.dump_raw_tape(os);
}

} // namespace simdjson
