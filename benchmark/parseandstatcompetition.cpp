#include "simdjson.h"
#include <unistd.h>

#include "benchmark.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS

// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

SIMDJSON_POP_DISABLE_WARNINGS

using namespace rapidjson;
using namespace simdjson;
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

simdjson_really_inline void simdjson_process_atom(stat_t &s,
                                         simdjson::dom::element element) {
  if (element.is<double>()) {
    s.number_count++;
  } else if (element.is<bool>()) {
    simdjson::error_code error;
    bool v;
    if (not (error = element.get(v)) && v) {
      s.true_count++;
    } else {
      s.false_count++;
    }
  } else if (element.is_null()) {
    s.null_count++;
  }
}

void simdjson_recurse(stat_t &s, simdjson::dom::element element) {
  error_code error;
  if (element.is<simdjson::dom::array>()) {
    s.array_count++;
    dom::array array;
    if ((error = element.get(array))) {
      std::cerr << error << std::endl;
      abort();
    }
    for (auto child : array) {
      if (child.is<simdjson::dom::array>() ||
          child.is<simdjson::dom::object>()) {
        simdjson_recurse(s, child);
      } else {
        simdjson_process_atom(s, child);
      }
    }
  } else if (element.is<simdjson::dom::object>()) {
    s.object_count++;
    dom::object object;
    if ((error = element.get(object))) {
      std::cerr << error << std::endl;
      abort();
    }
    for (auto field : object) {
      if (field.value.is<simdjson::dom::array>() ||
          field.value.is<simdjson::dom::object>()) {
        simdjson_recurse(s, field.value);
      } else {
        simdjson_process_atom(s, field.value);
      }
    }
  } else {
    simdjson_process_atom(s, element);
  }
}

simdjson_never_inline stat_t simdjson_compute_stats(const simdjson::padded_string &p) {
  stat_t s{};
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(p).get(doc);
  if (error) {
    s.valid = false;
    return s;
  }
  s.valid = true;
  simdjson_recurse(s, doc);
  return s;
}

///
struct Stat {
  size_t objectCount;
  size_t arrayCount;
  size_t numberCount;
  size_t stringCount;
  size_t trueCount;
  size_t falseCount;
  size_t nullCount;

  size_t memberCount;  // Number of members in all objects
  size_t elementCount; // Number of elements in all arrays
  size_t stringLength; // Number of code units in all strings
};

static error_code GenStatPlus(Stat &stat, const dom::element &v);
static error_code GenStatPlus(Stat &stat, const simdjson_result<dom::element> &r) {
  dom::element v;
  SIMDJSON_TRY( r.get(v) );
  return GenStatPlus(stat, v);
}
static error_code GenStatPlus(Stat &stat, const dom::element &v) {
  switch (v.type()) {
  case dom::element_type::ARRAY: {
    dom::array a;
    SIMDJSON_TRY( v.get(a) )
    for (auto child : a) {
      GenStatPlus(stat, child);
      stat.elementCount++;
    }
    stat.arrayCount++;
  } break;
  case dom::element_type::OBJECT: {
    dom::object o;
    SIMDJSON_TRY( v.get(o) );
    for (dom::key_value_pair kv : o) {
      GenStatPlus(stat, kv.value);
      stat.stringLength += kv.key.size();
      stat.memberCount++;
      stat.stringCount++;
    }
    stat.objectCount++;
  } break;
  case dom::element_type::INT64:
  case dom::element_type::UINT64:
  case dom::element_type::DOUBLE:
    stat.numberCount++;
    break;
  case dom::element_type::STRING: {
    stat.stringCount++;
    std::string_view sv;
    SIMDJSON_TRY( v.get(sv) );
    stat.stringLength += sv.size();
  } break;
  case dom::element_type::BOOL: {
    bool b;
    SIMDJSON_TRY( v.get(b) );
    if (b) {
      stat.trueCount++;
    } else {
      stat.falseCount++;
    }
  } break;
  case dom::element_type::NULL_VALUE:
    ++stat.nullCount;
    break;
  }
  return SUCCESS;
}

static void RapidGenStat(Stat &stat, const rapidjson::Value &v) {
  switch (v.GetType()) {
  case kNullType:
    stat.nullCount++;
    break;
  case kFalseType:
    stat.falseCount++;
    break;
  case kTrueType:
    stat.trueCount++;
    break;

  case kObjectType:
    for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      stat.stringLength += m->name.GetStringLength();
      RapidGenStat(stat, m->value);
    }
    stat.objectCount++;
    stat.memberCount += (v.MemberEnd() - v.MemberBegin());
    stat.stringCount += (v.MemberEnd() - v.MemberBegin()); // Key
    break;

  case kArrayType:
    for (Value::ConstValueIterator i = v.Begin(); i != v.End(); ++i)
      RapidGenStat(stat, *i);
    stat.arrayCount++;
    stat.elementCount += v.Size();
    break;

  case kStringType:
    stat.stringCount++;
    stat.stringLength += v.GetStringLength();
    break;

  case kNumberType:
    stat.numberCount++;
    break;
  }
}
simdjson_never_inline Stat rapidjson_compute_stats_ref(const rapidjson::Value &doc) {
  Stat s{};
  RapidGenStat(s, doc);
  return s;
}

simdjson_never_inline Stat
simdjson_compute_stats_refplus(const simdjson::dom::element &doc) {
  Stat s{};
  auto error = GenStatPlus(s, doc);
  if (error) { std::cerr << error << std::endl; abort(); }
  return s;
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

simdjson_never_inline stat_t sasjon_compute_stats(const simdjson::padded_string &p) {
  stat_t answer{};
  char *buffer = (char *)malloc(p.size());
  if (buffer == nullptr) {
    return answer;
  }
  memcpy(buffer, p.data(), p.size());
  auto d = sajson::parse(sajson::dynamic_allocation(),
                         sajson::mutable_string_view(p.size(), buffer));
  answer.valid = d.is_valid();
  if (!answer.valid) {
    free(buffer);
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

simdjson_never_inline stat_t rapid_compute_stats(const simdjson::padded_string &p) {
  stat_t answer{};
  char *buffer = (char *)malloc(p.size() + 1);
  if (buffer == nullptr) {
    return answer;
  }
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  answer.valid = !d.HasParseError();
  if (!answer.valid) {
    free(buffer);
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

simdjson_never_inline stat_t
rapid_accurate_compute_stats(const simdjson::padded_string &p) {
  stat_t answer{};
  char *buffer = (char *)malloc(p.size() + 1);
  if (buffer == nullptr) {
    return answer;
  }
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(buffer);
  answer.valid = !d.HasParseError();
  if (!answer.valid) {
    free(buffer);
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
  bool just_data = false;

  int c;
  while ((c = getopt(argc, argv, "vt")) != -1)
    switch (c) {
    case 't':
      just_data = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      abort();
    }
  if (optind >= argc) {
    std::cerr
        << "Using different parsers, we compute the content statistics of "
           "JSON documents."
        << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "Or " << argv[0] << " -v <jsonfile>" << std::endl;
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
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  if (verbose) {
    std::cout << "Input has ";
    if (p.size() > 1000 * 1000)
      std::cout << p.size() / (1000 * 1000) << " MB ";
    else if (p.size() > 1000)
      std::cout << p.size() / 1000 << " KB ";
    else
      std::cout << p.size() << " B ";
    std::cout << std::endl;
  }
  stat_t s1 = simdjson_compute_stats(p);
  if (verbose) {
    printf("simdjson: ");
    print_stat(s1);
  }
  stat_t s2 = rapid_compute_stats(p);
  if (verbose) {
    printf("rapid:    ");
    print_stat(s2);
  }
  stat_t s2a = rapid_accurate_compute_stats(p);
  if (verbose) {
    printf("rapid full:    ");
    print_stat(s2a);
  }
  stat_t s3 = sasjon_compute_stats(p);
  if (verbose) {
    printf("sasjon:   ");
    print_stat(s3);
  }
  assert(stat_equal(s1, s2));
  assert(stat_equal(s1, s3));
  int repeat = 50;
  size_t volume = p.size();
  if (just_data) {
    printf("name cycles_per_byte cycles_per_byte_err gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson            ", simdjson_compute_stats(p).valid, true, ,
            repeat, volume, !just_data);
  BEST_TIME("RapidJSON           ", rapid_compute_stats(p).valid, true, ,
            repeat, volume, !just_data);
  BEST_TIME("RapidJSON (precise) ", rapid_accurate_compute_stats(p).valid, true,
            , repeat, volume, !just_data);
  BEST_TIME("sasjon              ", sasjon_compute_stats(p).valid, true, ,
            repeat, volume, !just_data);
  if (!just_data) {
    printf("API traversal tests\n");
    printf("Based on https://github.com/miloyip/nativejson-benchmark\n");
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(p).get(doc);
    if (error) { std::cerr << error << std::endl; abort(); }
    size_t refval = simdjson_compute_stats_refplus(doc).objectCount;

    BEST_TIME("simdjson            ",
              simdjson_compute_stats_refplus(doc).objectCount, refval, , repeat,
              volume, !just_data);
    char *buffer = (char *)malloc(p.size() + 1);
    memcpy(buffer, p.data(), p.size());
    buffer[p.size()] = '\0';
    rapidjson::Document d;
    d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
    BEST_TIME("rapid              ", rapidjson_compute_stats_ref(d).objectCount,
              refval, , repeat, volume, !just_data);
    free(buffer);
  }
}
