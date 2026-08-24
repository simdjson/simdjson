#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace {

// Touches a dom::element across a few different types rather than just
// confirming it was reached: at_path()/at_path_with_wildcard() are noexcept,
// so this is safe to call unconditionally without a try/catch.
void probe_dom_element(const simdjson::dom::element &e) {
  simdjson_unused auto sv = e.get_string();
  simdjson_unused auto d = e.get_double();
  simdjson_unused auto b = e.get_bool();
  simdjson_unused auto a = e.get_array();
  simdjson_unused auto o = e.get_object();
}

void fuzz_dom(std::string_view json, std::string_view path) {
  simdjson::dom::parser parser;
  auto res = parser.parse(json);
  if (res.error())
    return;
  simdjson::dom::element root;
  if (res.get(root))
    return;

  // at_path() converts JSONPath to a JSON Pointer and delegates to
  // at_pointer(). Both this and at_path_with_wildcard() below are
  // noexcept: an exception escaping either would call std::terminate(),
  // which is exactly the kind of bug this fuzzer is looking for (see
  // https://github.com/simdjson/simdjson/pull/2801, which fixed one
  // such case in get_next_key_and_json_path()). Nothing here needs a
  // try/catch: a noexcept violation aborts before it could be caught.
  auto maybe_leaf = root.at_path(path);
  if (!maybe_leaf.error()) {
    simdjson::dom::element leaf;
    if (!maybe_leaf.get(leaf)) {
      probe_dom_element(leaf);
    }
  }

  // at_path_with_wildcard() walks the path itself (rather than converting
  // to a JSON Pointer first), so it exercises get_next_key_and_json_path()
  // directly -- the function with the actual crash history above.
  auto maybe_matches = root.at_path_with_wildcard(path);
  if (!maybe_matches.error()) {
    std::vector<simdjson::dom::element> matches;
    if (!std::move(maybe_matches).get(matches)) {
      for (const auto &match : matches) {
        probe_dom_element(match);
      }
    }
  }
}

void fuzz_ondemand(std::string_view json, std::string_view path) {
  // On-Demand requires a padded buffer; a fresh one is built per call below
  // since a document is single-pass and consumed by each of the two calls.
  simdjson::padded_string padded(json);

  {
    simdjson::ondemand::parser parser;
    auto doc_result = parser.iterate(padded);
    if (!doc_result.error()) {
      simdjson::ondemand::document doc;
      if (!std::move(doc_result).get(doc)) {
        auto maybe_value = doc.at_path(path);
        if (!maybe_value.error()) {
          simdjson::ondemand::value val;
          if (!maybe_value.get(val)) {
            simdjson_unused auto sv = val.get_string();
          }
        }
      }
    }
  }

  {
    simdjson::ondemand::parser parser;
    auto doc_result = parser.iterate(padded);
    if (!doc_result.error()) {
      simdjson::ondemand::document doc;
      if (!std::move(doc_result).get(doc)) {
        // for_each_at_path_with_wildcard() rewinds internally and walks the
        // path itself, exercising the same JSONPath parsing logic as the
        // DOM side, but through the On-Demand surface.
        (void)doc.for_each_at_path_with_wildcard(
            path, [](simdjson::ondemand::value v) {
              simdjson_unused auto sv = v.get_string();
            });
      }
    }
  }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

  // Split data into two strings, JSONPath and the document string.
  // Might end up with none, either or both being empty, important for
  // covering edge cases. Inputs missing the separator get an empty
  // JSONPath but all the input put in the document string, meaning
  // test data from other fuzzers that take json input works for this
  // fuzzer as well. Uses the same split convention as fuzz_atpointer,
  // so corpora between the two are interchangeable.
  FuzzData fd(Data, Size);
  auto strings = fd.splitIntoStrings();
  while (strings.size() < 2) {
    strings.emplace_back();
  }
  assert(strings.size() >= 2);

  fuzz_dom(strings[0], strings[1]);
  fuzz_ondemand(strings[0], strings[1]);

  return 0;
}
