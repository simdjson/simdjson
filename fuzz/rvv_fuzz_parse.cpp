// C:\MyCode\Lemire\simdjson\fuzz\rvv_fuzz_parse.cpp
//
// -----------------------------------------------------------------------------
// RVV Backend Fuzzer Harness: Parse (DOM + On-Demand)
// -----------------------------------------------------------------------------
//
// Purpose
// - Provide a libFuzzer-compatible harness that forces the RVV backend (when
//   available) and exercises both DOM and On-Demand parsing paths.
// - Useful for catching RVV-specific stage1 issues (structure indexing) as well
//   as integration issues in higher layers.
//
// Notes
// - This harness is intentionally small and deterministic.
// - It avoids throwing exceptions and avoids large allocations when possible.
// - It uses padded_string (copy) to ensure required padding for simdjson.
// - The harness is safe to compile even on non-RVV targets; it will try to
//   select rvv but will run on the active backend if rvv is unavailable.
//
// Build hints (example, clang + libFuzzer):
//   clang++ -fsanitize=fuzzer,address,undefined -O1 -g \
//     -Iinclude -Isingleheader -DSIMDJSON_ENABLE_RV64=ON \
//     fuzz/rvv_fuzz_parse.cpp -o rvv_fuzz_parse
//
// Run (QEMU example):
//   export QEMU_CPU="rv64,v=true,vext_spec=v1.0,vlen=128"
//   export SIMDJSON_FORCE_IMPLEMENTATION=rvv
//   qemu-riscv64 ./rvv_fuzz_parse -runs=100000
//
// -----------------------------------------------------------------------------

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>

#include "simdjson.h"

namespace {

static inline void force_rvv_if_present() {
  // Best-effort: set active impl to rvv if it's registered.
  // If the build doesn't include rvv, do nothing.
  for (auto impl : simdjson::get_available_implementations()) {
    if (impl && impl->name() == "rvv") {
      simdjson::get_active_implementation() = impl;
      break;
    }
  }
}

// Small helper: cheaply reject absurd sizes (keeps fuzzer fast).
static inline bool size_ok(size_t len) {
  // 1 MiB cap by default. Adjust if you want deeper stress.
  return len <= (1u << 20);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (data == nullptr) return 0;
  if (!size_ok(size)) return 0;

  force_rvv_if_present();

  // Make a padded copy. This is important for simdjson parsers.
  simdjson::padded_string p;
  // padded_string::copy returns an error_code in newer simdjson versions.
  // We support both patterns by using .value() only if needed is risky;
  // instead, use the stable helper: allocate a vector with padding and wrap.
  //
  // But padded_string::copy is commonly available; use it and bail on error.
  if (auto err = simdjson::padded_string::copy(reinterpret_cast<const char *>(data), size).get(p)) {
    return 0;
  }

  // -------------------------------
  // DOM parsing path
  // -------------------------------
  {
    // Use a local parser to avoid cross-iteration state issues.
    simdjson::dom::parser dom_parser;
    simdjson::dom::element doc;

    auto res = dom_parser.parse(p);
    // If valid, touch a little bit of the document to exercise navigation.
    if (!res.get(doc)) {
      // A tiny traversal to keep it cheap but non-trivial.
      // - If array: iterate a few items.
      // - If object: iterate a few fields.
      // - If scalar: read its type.
      switch (doc.type()) {
        case simdjson::dom::element_type::ARRAY: {
          simdjson::dom::array a;
          if (!doc.get(a)) {
            size_t i = 0;
            for (auto v : a) {
              (void)v.type();
              if (++i >= 4) break;
            }
          }
          break;
        }
        case simdjson::dom::element_type::OBJECT: {
          simdjson::dom::object o;
          if (!doc.get(o)) {
            size_t i = 0;
            for (auto f : o) {
              (void)f.key;
              (void)f.value.type();
              if (++i >= 4) break;
            }
          }
          break;
        }
        default:
          (void)doc.type();
          break;
      }
    }
  }

  // -------------------------------
  // On-Demand parsing path
  // -------------------------------
  {
    // On-demand parser can be reused, but keeping it local is simplest.
    simdjson::ondemand::parser od_parser;
    auto doc_res = od_parser.iterate(p);

    simdjson::ondemand::document doc;
    if (!doc_res.get(doc)) {
      // Touch a bit of the doc:
      // - If it's an array/object, iterate a few elements/fields.
      // - Otherwise, try to read as raw json string (cheap slice).
      //
      // We must be careful: on-demand errors are returned via simdjson_result.
      simdjson::ondemand::json_type t;
      if (!doc.type().get(t)) {
        if (t == simdjson::ondemand::json_type::array) {
          simdjson::ondemand::array a;
          if (!doc.get_array().get(a)) {
            size_t i = 0;
            for (auto v : a) {
              simdjson::ondemand::value vv = v.value();
              simdjson::ondemand::json_type vt;
              (void)vv.type().get(vt);
              if (++i >= 4) break;
            }
          }
        } else if (t == simdjson::ondemand::json_type::object) {
          simdjson::ondemand::object o;
          if (!doc.get_object().get(o)) {
            size_t i = 0;
            for (auto f : o) {
              simdjson::ondemand::field ff = f;
              (void)ff.unescaped_key(); // triggers key decoding path
              simdjson::ondemand::value vv = ff.value();
              simdjson::ondemand::json_type vt;
              (void)vv.type().get(vt);
              if (++i >= 4) break;
            }
          }
        } else {
          std::string_view raw;
          (void)doc.get_raw_json_string().get(raw);
        }
      }
    }
  }

  return 0;
}
