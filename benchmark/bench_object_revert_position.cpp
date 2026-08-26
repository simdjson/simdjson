#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "from/benchmark_helpers.h"
#include "simdjson.h"

using namespace simdjson;
using namespace counters;

namespace {

// A document with FIELD_COUNT required fields and no "optional" field: any
// probe for "optional" is a guaranteed miss, matching issue #1952's
// motivating case (an optional field that turns out not to be present).
constexpr size_t FIELD_COUNT = 20;
constexpr size_t PROBE_AFTER = FIELD_COUNT / 2;

padded_string build_dataset() {
  std::string out = "{";
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    if (i > 0) { out += ','; }
    out += "\"f" + std::to_string(i) + "\":" + std::to_string(i);
  }
  out += '}';
  return padded_string(out);
}

// Baseline required by the field-order find_field() API today: on a miss,
// the object must be reset() and every field walked again from the start.
size_t run_with_reset(ondemand::parser &parser, const padded_string &json) {
  ondemand::document doc;
  if (parser.iterate(json).get(doc)) { return 0; }
  ondemand::object obj;
  if (doc.get_object().get(obj)) { return 0; }

  int64_t sum = 0;
  int64_t v;
  for (size_t i = 0; i < PROBE_AFTER; i++) {
    if (obj.find_field("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  int64_t optional_value;
  if (!obj.find_field("optional").get(optional_value)) { sum += optional_value; }
  // NO_SUCH_FIELD: recover the only way find_field() allows today.
  if (obj.reset().error()) { return 0; }
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    if (obj.find_field("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  return size_t(sum);
}

// #1952: capture the position right before the optional probe, and on a
// miss, revert_position() instead of reset() so fields already consumed
// are not rescanned.
size_t run_with_revert_position(ondemand::parser &parser, const padded_string &json) {
  ondemand::document doc;
  if (parser.iterate(json).get(doc)) { return 0; }
  ondemand::object obj;
  if (doc.get_object().get(obj)) { return 0; }

  int64_t sum = 0;
  int64_t v;
  for (size_t i = 0; i < PROBE_AFTER; i++) {
    if (obj.find_field("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  auto position = obj.get_current_position();
  int64_t optional_value;
  if (!obj.find_field("optional").get(optional_value)) { sum += optional_value; }
  // NO_SUCH_FIELD: only the fields from here on need to be rescanned.
  if (obj.revert_position(position)) { return 0; }
  for (size_t i = PROBE_AFTER; i < FIELD_COUNT; i++) {
    if (obj.find_field("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  return size_t(sum);
}

// find_field_unordered() (operator[]'s default): remembers its location
// intrinsically, so a miss costs nothing extra by construction. Included
// for context, not as a fix for the ordered find_field() case: it walks
// the object with a different, non-positional API.
size_t run_with_find_field_unordered(ondemand::parser &parser, const padded_string &json) {
  ondemand::document doc;
  if (parser.iterate(json).get(doc)) { return 0; }
  ondemand::object obj;
  if (doc.get_object().get(obj)) { return 0; }

  int64_t sum = 0;
  int64_t v;
  for (size_t i = 0; i < PROBE_AFTER; i++) {
    if (obj.find_field_unordered("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  int64_t optional_value;
  if (!obj.find_field_unordered("optional").get(optional_value)) { sum += optional_value; }
  for (size_t i = PROBE_AFTER; i < FIELD_COUNT; i++) {
    if (obj.find_field_unordered("f" + std::to_string(i)).get(v)) { return 0; }
    sum += v;
  }
  return size_t(sum);
}

} // namespace

int main() {
  auto json = build_dataset();
  ondemand::parser parser;

  for (size_t trial = 0; trial < 3; ++trial) {
    printf("\nTrial %zu (missing optional field, probed after %zu/%zu fields):\n",
           trial + 1, PROBE_AFTER, FIELD_COUNT);
    pretty_print("reset() on miss", json.size(),
      bench([&]() { return run_with_reset(parser, json); }));
    pretty_print("revert_position() on miss", json.size(),
      bench([&]() { return run_with_revert_position(parser, json); }));
    pretty_print("find_field_unordered() (context)", json.size(),
      bench([&]() { return run_with_find_field_unordered(parser, json); }));
  }
  return EXIT_SUCCESS;
}
