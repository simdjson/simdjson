#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

// Fuzzer for the FracturedJson DOM formatting engine
// (simdjson::fractured_json / simdjson::fractured_json_string).
//
// Threat model: an attacker controls (a) a JSON document, which is parsed
// with the DOM API, and (b) the formatting options. Both enter simdjson
// through the public formatting entry points, which recursively walk the
// parsed structure (attacker-shaped nesting) and build the output with
// inline / compact-multiline / table / expanded layout decisions, column
// padding arithmetic and comma placement. No other fuzz target reaches this
// engine: it shows 0/1194 covered lines on every fuzz target in the
// 20261105 coverage report.
//
// Property (the invariant upstream's own fractured_json_tests roundtrip
// test asserts): formatting a successfully parsed document must produce
// text that itself parses as JSON. A violation aborts, which surfaces as a
// libFuzzer crash and is a genuine formatter bug, never fuzzer noise.
//
// Options are fuzz-driven but bounded: unbounded sizes would only produce
// allocation failures, which are not bugs. Invalid option combinations
// (for example a tiny line length with a high row complexity) are accepted
// input and must not crash.
//
// Two lanes per input, so both plain-JSON corpora (the shared corpus.zip)
// and option-prefixed seeds are useful:
//   lane A: the whole input is the document, default options;
//   lane B: 15 option bytes (in make_options order) then the document.

namespace {

constexpr int kMaxLineLength = 4096;
constexpr int kMaxIndent = 32;
constexpr int kMaxComplexity = 8;

// Consumes 15 bytes from the front of fd (getInt<Min,Max> with range < 256
// takes exactly one byte each).
simdjson::fractured_json_options make_options(FuzzData &fd) {
  simdjson::fractured_json_options opts;
  opts.max_total_line_length =
      1 + static_cast<size_t>(fd.getInt<0, 255>()) * (kMaxLineLength / 256);
  opts.max_inline_complexity =
      static_cast<size_t>(fd.getInt<0, 255>()) % (kMaxComplexity + 1);
  opts.max_compact_array_complexity =
      static_cast<size_t>(fd.getInt<0, 255>()) % (kMaxComplexity + 1);
  opts.indent_spaces =
      static_cast<size_t>(fd.getInt<0, 255>()) % (kMaxIndent + 1);
  opts.always_expand_depth = fd.getInt<-1, 8>();
  opts.enable_table_format = fd.getInt<0, 1>() == 1;
  opts.max_table_row_complexity =
      static_cast<size_t>(fd.getInt<0, 255>()) % (kMaxComplexity + 1);
  opts.enable_compact_multiline = fd.getInt<0, 1>() == 1;
  opts.min_compact_array_row_items = 1 + static_cast<size_t>(fd.getInt<0, 63>());
  opts.simple_bracket_padding = fd.getInt<0, 1>() == 1;
  opts.nested_bracket_padding = fd.getInt<0, 1>() == 1;
  opts.colon_padding = fd.getInt<0, 1>() == 1;
  opts.comma_padding = fd.getInt<0, 1>() == 1;
  switch (fd.getInt<0, 2>()) {
  case 1:
    opts.comma_placement = simdjson::table_comma_placement::after_padding;
    break;
  case 2:
    opts.comma_placement =
        simdjson::table_comma_placement::before_padding_except_numbers;
    break;
  default:
    opts.comma_placement = simdjson::table_comma_placement::before_padding;
    break;
  }
  opts.number_alignment = fd.getInt<0, 1>() == 1
                              ? simdjson::number_list_alignment::right
                              : simdjson::number_list_alignment::left;
  return opts;
}

// Throws only on allocation failure or an internal error; neither is a
// finding, so both are swallowed. The re-parse property below is the one
// invariant worth aborting on.
void format_and_check(const simdjson::dom::element &elem,
                      const simdjson::fractured_json_options &opts) {
  std::string formatted;
#if SIMDJSON_EXCEPTIONS
  try {
    formatted = simdjson::fractured_json(elem, opts);
  } catch (...) {
    return;
  }
#else
  formatted = simdjson::fractured_json(elem, opts);
#endif
  simdjson::dom::parser reparser;
  simdjson::dom::element reparsed;
  if (reparser.parse(formatted).get(reparsed)) {
    std::fprintf(stderr,
                 "fractured_json produced output that fails to re-parse\n");
    abort(); // genuine formatter bug, classify as a real finding
  }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  simdjson::dom::parser parser;

  // Lane A: whole input as a document, default options.
  simdjson::dom::element elem;
  if (!parser.parse(std::string_view(as_chars(Data), Size)).get(elem)) {
    format_and_check(elem, simdjson::fractured_json_options{});

    // Same engine through the string entry point; parse errors from
    // arbitrary bytes are accepted behaviour, so swallow everything.
#if SIMDJSON_EXCEPTIONS
    try {
      (void)simdjson::fractured_json_string(
          std::string_view(as_chars(Data), Size));
    } catch (...) {
    }
#else
    (void)simdjson::fractured_json_string(
        std::string_view(as_chars(Data), Size));
#endif
  }

  // Lane B: fuzzed (bounded) options, then the document.
  FuzzData fd(Data, Size);
  const simdjson::fractured_json_options opts = make_options(fd);
  if (fd) {
    simdjson::dom::parser parser2;
    simdjson::dom::element elem2;
    if (!parser2.parse(fd.remainder_as_stringview()).get(elem2)) {
      format_and_check(elem2, opts);
    }
  }

  return 0;
}
