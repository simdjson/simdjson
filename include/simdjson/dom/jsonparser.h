// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_JSONPARSER_H
#define SIMDJSON_DOM_JSONPARSER_H

#include "simdjson/dom/document.h"
#include "simdjson/dom/parsedjson.h"
#include "simdjson/jsonioutil.h"

namespace simdjson {

//
// C API (json_parse and build_parsed_json) declarations
//

[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const uint8_t *buf, size_t len, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const char *buf, size_t len, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const std::string &s, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(s.data(), s.length(), realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const padded_string &s, dom::parser &parser) noexcept {
  error_code code = parser.parse(s).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}

[[deprecated("Use parser.parse() instead")]]
SIMDJSON_WARN_UNUSED inline dom::parser build_parsed_json(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
SIMDJSON_WARN_UNUSED inline dom::parser build_parsed_json(const char *buf, size_t len, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
SIMDJSON_WARN_UNUSED inline dom::parser build_parsed_json(const std::string &s, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(s.data(), s.length(), realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
SIMDJSON_WARN_UNUSED inline dom::parser build_parsed_json(const padded_string &s) noexcept {
  dom::parser parser;
  error_code code = parser.parse(s).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}

/** @private We do not want to allow implicit conversion from C string to std::string. */
int json_parse(const char *buf, dom::parser &parser) noexcept = delete;
/** @private We do not want to allow implicit conversion from C string to std::string. */
dom::parser build_parsed_json(const char *buf) noexcept = delete;

} // namespace simdjson

#endif // SIMDJSON_DOM_JSONPARSER_H
