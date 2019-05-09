#include <iostream>
#ifndef _MSC_VER 
#include <unistd.h>
#endif
#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"
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
  size_t nonasciibyte_count;
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

stat_t simdjson_computestats(const padded_string &p) {
  stat_t answer;
  ParsedJson pj = build_parsed_json(p);
  answer.valid = pj.isValid();
  if (!answer.valid) {
    return answer;
  }
  answer.backslash_count = count_backslash(reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.nonasciibyte_count =
      count_nonasciibytes(reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.byte_count = p.size();
  answer.integer_count = 0;
  answer.float_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
  answer.string_count = 0;
  answer.structural_indexes_count = pj.n_structural_indexes;
  size_t tapeidx = 0;
  uint64_t tape_val = pj.tape[tapeidx++];
  uint8_t type = (tape_val >> 56);
  size_t howmany = 0;
  assert(type == 'r');
  howmany = tape_val & JSONVALUEMASK;
  for (; tapeidx < howmany; tapeidx++) {
    tape_val = pj.tape[tapeidx];
    // uint64_t payload = tape_val & JSONVALUEMASK;
    type = (tape_val >> 56);
    switch (type) {
    case 'l': // we have a long int
      answer.integer_count++;
      tapeidx++; // skipping the integer
      break;
    case 'd': // we have a double
      answer.float_count++;
      tapeidx++; // skipping the double
      break;
    case 'n': // we have a null
      answer.null_count++;
      break;
    case 't': // we have a true
      answer.true_count++;
      break;
    case 'f': // we have a false
      answer.false_count++;
      break;
    case '{': // we have an object
      answer.object_count++;
      break;
    case '}': // we end an object
      break;
    case '[': // we start an array
      answer.array_count++;
      break;
    case ']': // we end an array
      break;
    case '"': // we have a string
      answer.string_count++;
      break;
    default:
      break; // ignore
    }
  }
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
  padded_string p;
  try {
    get_corpus(filename).swap(p);
  } catch (const std::exception &e) { // caught by reference to base
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  stat_t s = simdjson_computestats(p);
  if (!s.valid) {
    std::cerr << "not a valid JSON" << std::endl;
    return EXIT_FAILURE;
  }

  printf("# integer_count float_count string_count backslash_count "
         "nonasciibyte_count object_count array_count null_count true_count "
         "false_count byte_count structural_indexes_count ");
#ifdef __linux__
  printf(
      "  stage1_cycle_count stage1_instruction_count  stage2_cycle_count "
      " stage2_instruction_count  stage3_cycle_count stage3_instruction_count  ");
#else
  printf("(you are not under linux, so perf counters are disaabled)");
#endif
  printf("\n");
  printf("%zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu ", s.integer_count,
         s.float_count, s.string_count, s.backslash_count, s.nonasciibyte_count,
         s.object_count, s.array_count, s.null_count, s.true_count,
         s.false_count, s.byte_count, s.structural_indexes_count);
#ifdef __linux__
  ParsedJson pj;
  bool allocok = pj.allocateCapacity(p.size());
  if (!allocok) {
    std::cerr << "failed to allocate memory" << std::endl;
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
    bool isok = find_structural_bits(p.data(), p.size(), pj);
    unified.end(results);
    
    cy1 += results[0];
    cl1 += results[1];
    
    unified.start();
    isok = isok && unified_machine(p.data(), p.size(), pj);
    unified.end(results);
    
    cy2 += results[0];
    cl2 += results[1];
    if(!isok) {
      std::cerr << "failure?" << std::endl;
    }
  }
  printf("%f %f %f %f ", cy1 * 1.0 / iterations, cl1 * 1.0 / iterations,
         cy2 * 1.0 / iterations, cl2 * 1.0 / iterations);
#endif // __linux__
  printf("\n");
  return EXIT_SUCCESS;
}
