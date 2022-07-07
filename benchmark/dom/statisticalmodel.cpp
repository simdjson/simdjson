#include <iostream>
#include <unistd.h>
#include "simdjson.h"
#ifdef __linux__
#include "linux-perf-events.h"
#endif

size_t count_nonasciibytes(const uint8_t *input, size_t length) {
  size_t count = 0;
  for (size_t i = 0; i < length; i++) {
    count += input[i] >> 7;
  }
  return count;
}

size_t count_backslash(const uint8_t *input, size_t length) {
  size_t count = 0;
  for (size_t i = 0; i < length; i++) {
    count += (input[i] == '\\') ? 1 : 0;
  }
  return count;
}

struct stat_s {
  size_t integer_count;
  size_t float_count;
  size_t string_count;
  size_t backslash_count;
  size_t non_ascii_byte_count;
  size_t object_count;
  size_t array_count;
  size_t null_count;
  size_t true_count;
  size_t false_count;
  size_t byte_count;
  size_t structural_indexes_count;
  bool valid;
};

using stat_t = struct stat_s;



simdjson_inline void simdjson_process_atom(stat_t &s,
                                         simdjson::dom::element element) {
  if (element.is<int64_t>()) {
    s.integer_count++;
  } else if(element.is<std::string_view>()) {
    s.string_count++;
  } else if(element.is<double>()) {
    s.float_count++;
  } else if (element.is<bool>()) {
    bool v;
    simdjson::error_code error;
    if ((error = element.get(v))) { std::cerr << error << std::endl; abort(); }
    if (v) {
      s.true_count++;
    } else {
      s.false_count++;
    }
  } else if (element.is_null()) {
    s.null_count++;
  }
}

void simdjson_recurse(stat_t &s, simdjson::dom::element element) {
  simdjson::error_code error;
  if (element.is<simdjson::dom::array>()) {
    s.array_count++;
    simdjson::dom::array array;
    if ((error = element.get(array))) { std::cerr << error << std::endl; abort(); }
    for (auto child : array) {
      if (child.is<simdjson::dom::array>() || child.is<simdjson::dom::object>()) {
        simdjson_recurse(s, child);
      } else {
        simdjson_process_atom(s, child);
      }
    }
  } else if (element.is<simdjson::dom::object>()) {
    s.object_count++;
    simdjson::dom::object object;
    if ((error = element.get(object))) { std::cerr << error << std::endl; abort(); }
    for (auto field : object) {
      s.string_count++; // for key
      if (field.value.is<simdjson::dom::array>() || field.value.is<simdjson::dom::object>()) {
        simdjson_recurse(s, field.value);
      } else {
        simdjson_process_atom(s, field.value);
      }
    }
  } else {
    simdjson_process_atom(s, element);
  }
}

stat_t simdjson_compute_stats(const simdjson::padded_string &p) {
  stat_t answer{};
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(p).get(doc);
  if (error) {
    answer.valid = false;
    return answer;
  }
  answer.valid = true;
  answer.backslash_count =
      count_backslash(reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.non_ascii_byte_count = count_nonasciibytes(
      reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.byte_count = p.size();
  answer.structural_indexes_count = parser.implementation->n_structural_indexes;
  simdjson_recurse(answer, doc);
  return answer;
}

int main(int argc, char *argv[]) {
#ifndef _MSC_VER
  int c;
  while ((c = getopt(argc, argv, "")) != -1) {
    switch (c) {

    default:
      abort();
    }
  }
#else
  int optind = 1;
#endif
  if (optind >= argc) {
    std::cerr << "Reads json, prints stats. " << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;

    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]
              << std::endl;
  }
  simdjson::padded_string p;
  auto error = simdjson::padded_string::load(filename).get(p);
  if (error) {
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  stat_t s = simdjson_compute_stats(p);
  if (!s.valid) {
    std::cerr << "not a valid JSON" << std::endl;
    return EXIT_FAILURE;
  }

  printf("# integer_count float_count string_count backslash_count "
         "non_ascii_byte_count object_count array_count null_count true_count "
         "false_count byte_count structural_indexes_count ");
#ifdef __linux__
  printf("  stage1_cycle_count stage1_instruction_count  stage2_cycle_count "
         " stage2_instruction_count  stage3_cycle_count "
         "stage3_instruction_count  ");
#else
  printf("(you are not under linux, so perf counters are disaabled)");
#endif
  printf("\n");
  printf("%zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu ", s.integer_count,
         s.float_count, s.string_count, s.backslash_count,
         s.non_ascii_byte_count, s.object_count, s.array_count, s.null_count,
         s.true_count, s.false_count, s.byte_count, s.structural_indexes_count);
#ifdef __linux__
  simdjson::dom::parser parser;
  simdjson::error_code alloc_error = parser.allocate(p.size());
  if (alloc_error) {
    std::cerr << alloc_error << std::endl;
    return EXIT_FAILURE;
  }
  const uint32_t iterations = p.size() < 1 * 1000 * 1000 ? 1000 : 50;
  std::vector<int> evts;
  evts.push_back(PERF_COUNT_HW_CPU_CYCLES);
  evts.push_back(PERF_COUNT_HW_INSTRUCTIONS);
  LinuxEvents<PERF_TYPE_HARDWARE> unified(evts);
  unsigned long cy1 = 0, cy2 = 0;
  unsigned long cl1 = 0, cl2 = 0;
  std::vector<unsigned long long> results;
  results.resize(evts.size());
  for (uint32_t i = 0; i < iterations; i++) {
    unified.start();
    // The default template is simdjson::architecture::NATIVE.
    bool isok = (parser.implementation->stage1((const uint8_t *)p.data(), p.size(), simdjson::stage1_mode::regular) == simdjson::SUCCESS);
    unified.end(results);

    cy1 += results[0];
    cl1 += results[1];

    unified.start();
    isok = isok && (parser.implementation->stage2(parser.doc) == simdjson::SUCCESS);
    unified.end(results);

    cy2 += results[0];
    cl2 += results[1];
    if (!isok) {
      std::cerr << "failure?" << std::endl;
    }
  }
  printf("%f %f %f %f ", static_cast<double>(cy1) / static_cast<double>(iterations), static_cast<double>(cl1) / static_cast<double>(iterations),
         static_cast<double>(cy2) / static_cast<double>(iterations), static_cast<double>(cl2) / static_cast<double>(iterations));
#endif // __linux__
  printf("\n");
  return EXIT_SUCCESS;
}
