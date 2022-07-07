#pragma once

#if SIMDJSON_EXCEPTIONS

#include "largerandom.h"

namespace largerandom {

using namespace simdjson;
using namespace simdjson::builtin::stage2;

class Sax {
public:
  simdjson_inline bool Run(const padded_string &json) noexcept;

  simdjson_inline const std::vector<my_point> &Result() { return container; }
  simdjson_inline size_t ItemCount() { return container.size(); }

private:
  simdjson_inline error_code RunNoExcept(const padded_string &json) noexcept;
  error_code Allocate(size_t new_capacity);
  std::unique_ptr<uint8_t[]> string_buf{};
  size_t capacity{};
  dom_parser_implementation dom_parser{};
  std::vector<my_point> container{};
};

struct sax_point_reader_visitor {
public:
  std::vector<my_point> &points;
  enum {GOT_X=0, GOT_Y=1, GOT_Z=2, GOT_SOMETHING_ELSE=4};
  size_t idx{GOT_SOMETHING_ELSE};
  double buffer[3]={};

  explicit sax_point_reader_visitor(std::vector<my_point> &_points) : points(_points) {}

  simdjson_inline error_code visit_object_start(json_iterator &) {
    idx = 0;
    return SUCCESS;
  }
  simdjson_inline error_code visit_primitive(json_iterator &, const uint8_t *value) {
    if(idx == GOT_SOMETHING_ELSE) { return simdjson::SUCCESS; }
    return numberparsing::parse_double(value).get(buffer[idx]);
  }
  simdjson_inline error_code visit_object_end(json_iterator &)  {
    points.emplace_back(my_point{buffer[0], buffer[1], buffer[2]});
    return SUCCESS;
  }

  simdjson_inline error_code visit_document_start(json_iterator &) { return SUCCESS; }
  simdjson_inline error_code visit_key(json_iterator &, const uint8_t * key) {
    switch(key[1]) {
      // Technically, we should check the other characters
      // in the key, but we are cheating to go as fast
      // as possible.
      case 'x':
        idx = GOT_X;
        break;
      case 'y':
        idx = GOT_Y;
        break;
      case 'z':
        idx = GOT_Z;
        break;
      default:
        idx = GOT_SOMETHING_ELSE;
    }
    return SUCCESS;
  }
  simdjson_inline error_code visit_array_start(json_iterator &)  { return SUCCESS; }
  simdjson_inline error_code visit_array_end(json_iterator &) { return SUCCESS; }
  simdjson_inline error_code visit_document_end(json_iterator &)  { return SUCCESS; }
  simdjson_inline error_code visit_empty_array(json_iterator &)  { return SUCCESS; }
  simdjson_inline error_code visit_empty_object(json_iterator &)  { return SUCCESS; }
  simdjson_inline error_code visit_root_primitive(json_iterator &, const uint8_t *)  { return SUCCESS; }
  simdjson_inline error_code increment_count(json_iterator &) { return SUCCESS; }
};

// NOTE: this assumes the dom_parser is already allocated
bool Sax::Run(const padded_string &json) noexcept {
  auto error = RunNoExcept(json);
  if (error) { std::cerr << error << std::endl; return false; }
  return true;
}

error_code Sax::RunNoExcept(const padded_string &json) noexcept {
  container.clear();

  // Allocate capacity if needed
  if (capacity < json.size()) {
    SIMDJSON_TRY( Allocate(json.size()) );
  }

  // Run stage 1 first.
  SIMDJSON_TRY( dom_parser.stage1(json.u8data(), json.size(), false) );

  // Then walk the document, parsing the tweets as we go
  json_iterator iter(dom_parser, 0);
  sax_point_reader_visitor visitor(container);
  SIMDJSON_TRY( iter.walk_document<false>(visitor) );
  return SUCCESS;
}

error_code Sax::Allocate(size_t new_capacity) {
  // string_capacity copied from document::allocate
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * new_capacity / 3 + SIMDJSON_PADDING, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
  if (auto error = dom_parser.set_capacity(new_capacity)) { return error; }
  if (capacity == 0) { // set max depth the first time only
    if (auto error = dom_parser.set_max_depth(DEFAULT_MAX_DEPTH)) { return error; }
  }
  capacity = new_capacity;
  return SUCCESS;
}

BENCHMARK_TEMPLATE(LargeRandom, Sax);

} // namespace largerandom

#endif // SIMDJSON_EXCEPTIONS
