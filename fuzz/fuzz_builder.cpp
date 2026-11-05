#include "FuzzUtils.h"
#include "simdjson.h"
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

// Fuzzer for simdjson::builder::string_builder, the low-level JSON writer
// behind the builder API (public via simdjson::builder; 0/233 covered lines
// on every fuzz target, 20261105 report). This file exercises the byte-level
// machinery: capacity checks and growth (including the 6x size estimate for
// escaping and the overflow checks in capacity_check), string escaping,
// comma/colon emission, move_to/clear state transitions and view().
//
// Threat model: an attacker controls a sequence of builder operations with
// fuzzed payloads (numbers of every width, doubles including NaN/inf
// bit patterns, strings that reach the escaping paths with quotes,
// backslashes, control bytes and arbitrary UTF-8, and raw byte appends).
//
// Two builders are driven per input:
//   Builder A ("structured") only ever receives WELL-FORMED operation
//   sequences: the harness tracks container state, so its output is
//   structurally valid JSON by construction. Property: if the builder
//   reports the output as valid UTF-8 and it is non-empty, the output must
//   parse with both the DOM and the On-Demand API. An abort is a genuine
//   builder or escaping bug, never fuzzer noise (raw bytes are never fed
//   to this builder, so they cannot invalidate the output).
//   Builder B ("raw") receives the byte-level operations (append_raw,
//   escape_and_append without quotes, single chars, commas, colons,
//   clear/move_to/size) with no validity property; it exists to exercise
//   the raw machinery under ASan/UBSan.
//
// Operation byte layout (op % 16), payload bytes are consumed from the same
// FuzzData stream (matching gen_builder_corpus.py):
//   0 start_object   1 end_object   2 start_array  3 end_array
//   4 raw: append_comma              5 raw: append_colon
//   6 null value
//   7 int8 value (1 payload byte)    8 uint64 value (8)
//   9 double value (8, bit pattern)  10 bool value (1)
//  11 raw: append(char) (1)          12 string value (1+n)
//  13 raw: escape_and_append (1+n)   14 raw: append_raw (1+n)
//  15 raw: move_to+clear+size cycle
// In object context, value ops (6-10, 12) first consume a fuzzed key
// (1+n bytes) and emit "key": value; containers as values emit "key": {/[.

namespace {

constexpr int kMaxOps = 64;
constexpr size_t kMaxDepth = 12;

struct Driver {
  simdjson::builder::string_builder sb;
  std::vector<char> stack;    // '{' or '['
  std::vector<size_t> counts; // emitted elements per open container
  bool root_used = false;

  bool in_object() const { return !stack.empty() && stack.back() == '{'; }
  bool in_array() const { return !stack.empty() && stack.back() == '['; }
  bool can_open_root() const { return !root_used; }
};

// Consumes a fuzzed key (1+n bytes) when inside an object; returns the key.
std::string_view take_key(FuzzData &fd) { return fd.get_stringview<0, 40>(); }

void close_all(Driver &d) {
  while (!d.stack.empty()) {
    if (d.stack.back() == '{') {
      d.sb.end_object();
    } else {
      d.sb.end_array();
    }
    d.stack.pop_back();
  }
}

// ---- structured value emitters; return true when something was emitted ----

bool emit_string_value(Driver &d, FuzzData &fd, std::string_view s) {
  if (d.in_object()) {
    auto key = take_key(fd);
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.append_key_value(key, s);
    d.counts.back()++;
    return true;
  }
  if (d.in_array()) {
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.escape_and_append_with_quotes(s);
    d.counts.back()++;
    return true;
  }
  if (!d.root_used) {
    d.sb.escape_and_append_with_quotes(s);
    d.root_used = true;
    return true;
  }
  return false;
}

template <typename V>
bool emit_scalar_value(Driver &d, FuzzData &fd, V v) {
  if (d.in_object()) {
    auto key = take_key(fd);
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.append_key_value(key, v);
    d.counts.back()++;
    return true;
  }
  if (d.in_array()) {
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.append(v);
    d.counts.back()++;
    return true;
  }
  if (!d.root_used) {
    d.sb.append(v);
    d.root_used = true;
    return true;
  }
  return false;
}

bool emit_null_value(Driver &d, FuzzData &fd) {
  if (d.in_object()) {
    auto key = take_key(fd);
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.escape_and_append_with_quotes(key);
    d.sb.append_colon();
    d.sb.append_null();
    d.counts.back()++;
    return true;
  }
  if (d.in_array()) {
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.append_null();
    d.counts.back()++;
    return true;
  }
  if (!d.root_used) {
    d.sb.append_null();
    d.root_used = true;
    return true;
  }
  return false;
}

// Open a container; inside an object the container is a value, so a fuzzed
// key is consumed and emitted first.
bool open_container(Driver &d, FuzzData &fd, char which) {
  if (d.stack.size() >= kMaxDepth) {
    return false;
  }
  if (d.in_object()) {
    auto key = take_key(fd);
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
    d.sb.escape_and_append_with_quotes(key);
    d.sb.append_colon();
  } else if (d.in_array()) {
    if (d.counts.back() > 0) {
      d.sb.append_comma();
    }
  } else if (d.root_used) {
    return false; // root already taken
  }
  if (which == '{') {
    d.sb.start_object();
  } else {
    d.sb.start_array();
  }
  if (!d.in_object() && !d.in_array()) {
    d.root_used = true; // opened at the root
  }
  d.stack.push_back(which);
  d.counts.push_back(0);
  return true;
}

// ---- raw machinery (builder B); no validity property ----

void raw_op_comma(simdjson::builder::string_builder &b) { b.append_comma(); }
void raw_op_colon(simdjson::builder::string_builder &b) { b.append_colon(); }
void raw_op_char(simdjson::builder::string_builder &b, FuzzData &fd) {
  b.append(static_cast<char>(fd.get<uint8_t>()));
}
void raw_op_escape(simdjson::builder::string_builder &b, FuzzData &fd) {
  b.escape_and_append(fd.get_stringview<0, 64>());
}
void raw_op_append_raw(simdjson::builder::string_builder &b, FuzzData &fd) {
  b.append_raw(fd.get_stringview<0, 64>());
}
void raw_op_state_cycle(simdjson::builder::string_builder &b) {
  std::string out;
  auto moved = b.move_to(out); // moves the buffer out, leaving the builder invalid
  (void)moved; // result intentionally discarded; crash-freedom is the goal
  (void)b.size();
  b.clear(); // back to a usable empty state
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  Driver d;
  simdjson::builder::string_builder raw_sb;
  FuzzData fd(Data, Size);

  for (int ops = 0; ops < kMaxOps && fd; ops++) {
    const int op = fd.getInt<0, 255>() % 16;
    switch (op) {
    case 0:
      open_container(d, fd, '{');
      break;
    case 1:
      if (d.in_object()) {
        d.sb.end_object();
        d.stack.pop_back();
        d.counts.pop_back();
        if (d.stack.empty()) {
          d.root_used = true;
        } else {
          // the closed container was an element of the parent
          d.counts.back()++;
        }
      }
      break;
    case 2:
      open_container(d, fd, '[');
      break;
    case 3:
      if (d.in_array()) {
        d.sb.end_array();
        d.stack.pop_back();
        d.counts.pop_back();
        if (d.stack.empty()) {
          d.root_used = true;
        } else {
          d.counts.back()++;
        }
      }
      break;
    case 4:
      raw_op_comma(raw_sb);
      break;
    case 5:
      raw_op_colon(raw_sb);
      break;
    case 6:
      emit_null_value(d, fd);
      break;
    case 7:
      emit_scalar_value(d, fd,
                        static_cast<int64_t>(fd.getInt<0, 255>()) - 128);
      break;
    case 8: {
      const uint64_t v = fd.get<uint64_t>();
      emit_scalar_value(d, fd, v);
    } break;
    case 9: {
      const uint64_t bits = fd.get<uint64_t>();
      double dbl;
      static_assert(sizeof(dbl) == sizeof(bits), "size mismatch");
      std::memcpy(&dbl, &bits, sizeof(dbl));
      // Quarantine for finding simdjson-1: append(double) with a non-finite
      // value emits out-of-range text that no JSON parser accepts (the
      // float path bypasses write_double's non-finite handling when
      // SIMDJSON_ENABLE_NAN_INF is off). Routed around here so the rest of
      // the builder surface is exercised; see the pair report.
      if (std::isfinite(dbl)) {
        emit_scalar_value(d, fd, dbl);
      }
    } break;
    case 10:
      emit_scalar_value(d, fd, fd.getInt<0, 1>() == 1);
      break;
    case 11:
      raw_op_char(raw_sb, fd);
      break;
    case 12:
      emit_string_value(d, fd, fd.get_stringview<0, 80>());
      break;
    case 13:
      raw_op_escape(raw_sb, fd);
      break;
    case 14:
      raw_op_append_raw(raw_sb, fd);
      break;
    default:
      raw_op_state_cycle(raw_sb);
      break;
    }
  }

  close_all(d);

  // Property: well-formed builder output that the builder itself reports as
  // valid UTF-8 must parse as JSON with both APIs.
  auto maybe_view = d.sb.view();
  if (!maybe_view.error()) {
    std::string_view out = maybe_view.value();
    if (!out.empty() && d.sb.validate_unicode()) {
      simdjson::padded_string padded(out);
      simdjson::dom::parser dom_parser;
      simdjson::dom::element dom_elem;
      if (dom_parser.parse(padded).get(dom_elem)) {
        std::fprintf(stderr, "string_builder output is not valid JSON: %.*s\n",
                     static_cast<int>(out.size()), out.data());
        abort(); // genuine builder/escaping bug
      }
      simdjson::ondemand::parser od_parser;
      simdjson::ondemand::document od_doc;
      if (od_parser.iterate(padded).get(od_doc)) {
        std::fprintf(stderr,
                     "string_builder output rejected by ondemand: %.*s\n",
                     static_cast<int>(out.size()), out.data());
        abort();
      }
    }
  }

  // Builder B: machinery exercised under the sanitizers, no property.
  (void)raw_sb.size();
  (void)raw_sb.validate_unicode();
  auto raw_view = raw_sb.view();
  if (!raw_view.error()) {
    (void)raw_view.value();
  }
  return 0;
}

