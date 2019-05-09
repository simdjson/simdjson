#include "simdjson/jsonparser.h"
#include <unistd.h>

#include "benchmark.h"
// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

using namespace rapidjson;

struct stat_s {
  size_t number_count;
  size_t object_count;
  size_t array_count;
  size_t null_count;
  size_t true_count;
  size_t false_count;
  bool valid;
};

typedef struct stat_s stat_t;

bool stat_equal(const stat_t &s1, const stat_t &s2) {
  return (s1.valid == s2.valid) && (s1.number_count == s2.number_count) &&
         (s1.object_count == s2.object_count) &&
         (s1.array_count == s2.array_count) &&
         (s1.null_count == s2.null_count) && (s1.true_count == s2.true_count) &&
         (s1.false_count == s2.false_count);
}

void print_stat(const stat_t &s) {
  if (!s.valid) {
    printf("invalid\n");
    return;
  }
  printf("number: %zu object: %zu array: %zu null: %zu true: %zu false: %zu\n",
         s.number_count, s.object_count, s.array_count, s.null_count,
         s.true_count, s.false_count);
}

__attribute__ ((noinline))
stat_t simdjson_computestats(const padded_string &p) {
  stat_t answer;
  ParsedJson pj = build_parsed_json(p);
  answer.valid = pj.isValid();
  if (!answer.valid) {
    return answer;
  }
  answer.number_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
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
      answer.number_count++;
      tapeidx++; // skipping the integer
      break;
    case 'd': // we have a double
      answer.number_count++;
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
    default:
      break; // ignore
    }
  }
  return answer;
}

// see
// https://github.com/miloyip/nativejson-benchmark/blob/master/src/tests/sajsontest.cpp
void sajson_traverse(stat_t &stats, const sajson::value &node) {
  using namespace sajson;
  switch (node.get_type()) {
  case TYPE_NULL:
    stats.null_count++;
    break;
  case TYPE_FALSE:
    stats.false_count++;
    break;
  case TYPE_TRUE:
    stats.true_count++;
    break;
  case TYPE_ARRAY: {
    stats.array_count++;
    auto length = node.get_length();
    for (size_t i = 0; i < length; ++i) {
      sajson_traverse(stats, node.get_array_element(i));
    }
    break;
  }
  case TYPE_OBJECT: {
    stats.object_count++;
    auto length = node.get_length();
    for (auto i = 0u; i < length; ++i) {
      sajson_traverse(stats, node.get_object_value(i));
    }
    break;
  }
  case TYPE_STRING:
    // skip
    break;

  case TYPE_DOUBLE:
  case TYPE_INTEGER:
    stats.number_count++; // node.get_number_value();
    break;
  default:
    assert(false && "unknown node type");
  }
}

__attribute__ ((noinline))
stat_t sasjon_computestats(const padded_string &p) {
  stat_t answer;
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  auto d = sajson::parse(sajson::dynamic_allocation(),
                         sajson::mutable_string_view(p.size(), buffer));
  answer.valid = d.is_valid();
  if (!answer.valid) {
    return answer;
  }
  answer.number_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
  sajson_traverse(answer, d.get_root());
  free(buffer);
  return answer;
}

void rapid_traverse(stat_t &stats, const rapidjson::Value &v) {
  switch (v.GetType()) {
  case kNullType:
    stats.null_count++;
    break;
  case kFalseType:
    stats.false_count++;
    break;
  case kTrueType:
    stats.true_count++;
    break;

  case kObjectType:
    for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      rapid_traverse(stats, m->value);
    }
    stats.object_count++;
    break;
  case kArrayType:
    for (Value::ConstValueIterator i = v.Begin(); i != v.End();
         ++i) { // v.Size();
      rapid_traverse(stats, *i);
    }
    stats.array_count++;
    break;

  case kStringType:
    break;

  case kNumberType:
    stats.number_count++;
    break;
  }
}

__attribute__ ((noinline))
stat_t rapid_computestats(const padded_string &p) {
  stat_t answer;
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  answer.valid = !d.HasParseError();
  if (!answer.valid) {
    return answer;
  }
  answer.number_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
  rapid_traverse(answer, d);
  free(buffer);
  return answer;
}

int main(int argc, char *argv[]) {
  bool verbose = false;
  bool justdata = false;

  int c;
  while ((c = getopt(argc, argv, "vt")) != -1)
    switch (c) {
    case 't':
      justdata = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    std::cerr << "Using different parsers, we compute the content statistics of "
            "JSON documents." << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "Or " << argv[0] << " -v <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]  << std::endl;
  }
  padded_string p;
  try {
    get_corpus(filename).swap(p);
  } catch (const std::exception &e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }

  if (verbose) {
    std::cout << "Input has ";
    if (p.size() > 1024 * 1024)
      std::cout << p.size() / (1024 * 1024) << " MB ";
    else if (p.size() > 1024)
      std::cout << p.size() / 1024 << " KB ";
    else
      std::cout << p.size() << " B ";
    std::cout << std::endl;
  }
  stat_t s1 = simdjson_computestats(p);
  if (verbose) {
    printf("simdjson: ");
    print_stat(s1);
  }
  stat_t s2 = rapid_computestats(p);
  if (verbose) {
    printf("rapid:    ");
    print_stat(s2);
  }
  stat_t s3 = sasjon_computestats(p);
  if (verbose) {
    printf("sasjon:   ");
    print_stat(s3);
  }
  assert(stat_equal(s1, s2));
  assert(stat_equal(s1, s3));
  int repeat = 50;
  int volume = p.size();
  if(justdata) {
    printf("name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson  ", simdjson_computestats(p).valid, true, , repeat,
            volume, !justdata);
  BEST_TIME("RapidJSON  ", rapid_computestats(p).valid, true, , repeat, volume,
            !justdata);
  BEST_TIME("sasjon  ", sasjon_computestats(p).valid, true, , repeat, volume,
            !justdata);
}
