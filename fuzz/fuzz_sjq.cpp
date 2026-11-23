// Fuzzes sjq (tools/sjq): the first byte gives the length of a filter, the
// rest is JSON. The filter runs on every document through the DOM evaluator
// and, when it compiles to one, through the On-Demand plan.
#include "ast.h"
#include "ondemand.h"
#include "writer.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  using namespace sjq;
  stack_guard_init(size_t(2) << 20);
  if (size < 1) { return 0; }
  size_t filter_len = data[0];
  if (filter_len > size - 1) { filter_len = size - 1; }
  std::string filter(reinterpret_cast<const char *>(data + 1), filter_len);
  simdjson::padded_string input(reinterpret_cast<const char *>(data + 1 + filter_len), size - 1 - filter_len);

  node_ptr prog;
  try {
    prog = compile(filter);
  } catch (std::exception &) {
    return 0;
  }
  od_ptr plan = od_compile(prog.get());

  writer w;
  w.to_memory = true;
  env_ptr root_env;
  auto emit = [&](const value &v) {
    w.write_value(v, 0);
    w.put('\n');
    if (w.buf.size() > (size_t(1) << 20)) { w.buf.clear(); }
  };
  auto reset_budget = []() {
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    g_fuzz_budget = 20000;
#endif
  };

  simdjson::dom::parser dom_parser;
  simdjson::dom::document_stream dom_stream;
  if (!dom_parser.parse_many(input).get(dom_stream)) {
    for (auto result : dom_stream) {
      simdjson::dom::element doc;
      if (result.get(doc)) { break; }
      reset_budget();
      try {
        prog->eval(value::of(doc), root_env, emit);
      } catch (std::exception &) {
      } catch (break_signal &) {
      }
    }
  }

  if (plan) {
    simdjson::ondemand::parser od_parser;
    simdjson::ondemand::document_stream od_stream;
    if (!od_parser.iterate_many(input).get(od_stream)) {
      for (auto it = od_stream.begin(); it != od_stream.end(); ++it) {
        simdjson::ondemand::document_reference doc;
        if ((*it).get(doc)) { break; }
        reset_budget();
        try {
          od_run_doc(*plan, doc, w, false, false, false);
        } catch (std::exception &) {
        } catch (break_signal &) {
        }
        if (w.buf.size() > (size_t(1) << 20)) { w.buf.clear(); }
      }
    }
  }
  return 0;
}
