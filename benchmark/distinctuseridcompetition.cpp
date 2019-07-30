#include "simdjson/jsonparser.h"
#include <algorithm>
#include <unistd.h>
#include <vector>

#include "benchmark.h"
// #define RAPIDJSON_SSE2 // bad for performance
// #define RAPIDJSON_SSE42 // bad for performance
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "sajson.h"

using namespace rapidjson;

bool equals(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

void remove_duplicates(std::vector<int64_t> &v) {
  std::sort(v.begin(), v.end());
  auto last = std::unique(v.begin(), v.end());
  v.erase(last, v.end());
}

void print_vec(const std::vector<int64_t> &v) {
  for (auto i : v) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

void simdjson_scan(std::vector<int64_t> &answer,
                   simdjson::ParsedJson::Iterator &i) {
  while (i.move_forward()) {
    if (i.get_scope_type() == '{') {
      bool found_user = (i.get_string_length() == 4) &&
                        (memcmp(i.get_string(), "user", 4) == 0);
      i.move_to_value();
      if (found_user) {
        if (i.is_object() && i.move_to_key("id", 2)) {
          if (i.is_integer()) {
            answer.push_back(i.get_integer());
          }
          i.up();
        }
      }
    }
  }
}

__attribute__((noinline)) std::vector<int64_t>
simdjson_just_dom(simdjson::ParsedJson &pj) {
  std::vector<int64_t> answer;
  simdjson::ParsedJson::Iterator i(pj);
  simdjson_scan(answer, i);
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) std::vector<int64_t>
simdjson_compute_stats(const simdjson::padded_string &p) {
  std::vector<int64_t> answer;
  simdjson::ParsedJson pj = simdjson::build_parsed_json(p);
  if (!pj.is_valid()) {
    return answer;
  }
  simdjson::ParsedJson::Iterator i(pj);
  simdjson_scan(answer, i);
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) bool
simdjson_just_parse(const simdjson::padded_string &p) {
  simdjson::ParsedJson pj = simdjson::build_parsed_json(p);
  bool answer = !pj.is_valid();
  return answer;
}

void sajson_traverse(std::vector<int64_t> &answer, const sajson::value &node) {
  using namespace sajson;
  switch (node.get_type()) {
  case TYPE_ARRAY: {
    auto length = node.get_length();
    for (size_t i = 0; i < length; ++i) {
      sajson_traverse(answer, node.get_array_element(i));
    }
    break;
  }
  case TYPE_OBJECT: {
    auto length = node.get_length();
    // sajson has O(log n) find_object_key, but we still visit each node anyhow
    // because we need to visit all values.
    for (auto i = 0u; i < length; ++i) {
      auto key = node.get_object_key(i); // expected: sajson::string
      bool found_user =
          (key.length() == 4) && (memcmp(key.data(), "user", 4) == 0);
      if (found_user) {                             // found a user!!!
        auto user_value = node.get_object_value(i); // get the value
        if (user_value.get_type() ==
            TYPE_OBJECT) { // the value should be an object
          // now we know that we only need one value
          auto user_value_length = user_value.get_length();
          auto right_index =
              user_value.find_object_key(sajson::string("id", 2));
          if (right_index < user_value_length) {
            auto v = user_value.get_object_value(right_index);
            if (v.get_type() == TYPE_INTEGER) { // check that it is an integer
              answer.push_back(v.get_integer_value()); // record it!
            } else if (v.get_type() == TYPE_DOUBLE) {
              answer.push_back((int64_t)v.get_double_value()); // record it!
            }
          }
        }
      }
      sajson_traverse(answer, node.get_object_value(i));
    }
    break;
  }
  case TYPE_NULL:
  case TYPE_FALSE:
  case TYPE_TRUE:
  case TYPE_STRING:
  case TYPE_DOUBLE:
  case TYPE_INTEGER:
    break;
  default:
    assert(false && "unknown node type");
  }
}

__attribute__((noinline)) std::vector<int64_t>
sasjon_just_dom(sajson::document &d) {
  std::vector<int64_t> answer;
  sajson_traverse(answer, d.get_root());
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) std::vector<int64_t>
sasjon_compute_stats(const simdjson::padded_string &p) {
  std::vector<int64_t> answer;
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  auto d = sajson::parse(sajson::dynamic_allocation(),
                         sajson::mutable_string_view(p.size(), buffer));
  if (!d.is_valid()) {
    free(buffer);
    return answer;
  }
  sajson_traverse(answer, d.get_root());
  free(buffer);
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) bool
sasjon_just_parse(const simdjson::padded_string &p) {
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  auto d = sajson::parse(sajson::dynamic_allocation(),
                         sajson::mutable_string_view(p.size(), buffer));
  bool answer = !d.is_valid();
  free(buffer);
  return answer;
}

void rapid_traverse(std::vector<int64_t> &answer, const rapidjson::Value &v) {
  switch (v.GetType()) {
  case kObjectType:
    for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      bool found_user = (m->name.GetStringLength() == 4) &&
                        (memcmp(m->name.GetString(), "user", 4) == 0);
      if (found_user) {
        const rapidjson::Value &child = m->value;
        if (child.GetType() == kObjectType) {
          for (Value::ConstMemberIterator k = child.MemberBegin();
               k != child.MemberEnd(); ++k) {
            if (equals(k->name.GetString(), "id")) {
              const rapidjson::Value &val = k->value;
              if (val.GetType() == kNumberType) {
                answer.push_back(val.GetInt64());
              }
            }
          }
        }
      }
      rapid_traverse(answer, m->value);
    }
    break;
  case kArrayType:
    for (Value::ConstValueIterator i = v.Begin(); i != v.End();
         ++i) { // v.Size();
      rapid_traverse(answer, *i);
    }
    break;
  case kNullType:
  case kFalseType:
  case kTrueType:
  case kStringType:
  case kNumberType:
  default:
    break;
  }
}

__attribute__((noinline)) std::vector<int64_t>
rapid_just_dom(rapidjson::Document &d) {
  std::vector<int64_t> answer;
  rapid_traverse(answer, d);
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) std::vector<int64_t>
rapid_compute_stats(const simdjson::padded_string &p) {
  std::vector<int64_t> answer;
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  if (d.HasParseError()) {
    free(buffer);
    return answer;
  }
  rapid_traverse(answer, d);
  free(buffer);
  remove_duplicates(answer);
  return answer;
}

__attribute__((noinline)) bool
rapid_just_parse(const simdjson::padded_string &p) {
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  bool answer = d.HasParseError();
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
  try {
    simdjson::get_corpus(filename).swap(p);
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
  std::vector<int64_t> s1 = simdjson_compute_stats(p);
  if (verbose) {
    printf("simdjson: ");
    print_vec(s1);
  }
  std::vector<int64_t> s2 = rapid_compute_stats(p);
  if (verbose) {
    printf("rapid:    ");
    print_vec(s2);
  }
  std::vector<int64_t> s3 = sasjon_compute_stats(p);
  if (verbose) {
    printf("sasjon:   ");
    print_vec(s3);
  }
  assert(s1 == s2);
  assert(s1 == s3);
  size_t size = s1.size();

  int repeat = 500;
  int volume = p.size();
  if (just_data) {
    printf(
        "name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson  ", simdjson_compute_stats(p).size(), size, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  ", rapid_compute_stats(p).size(), size, , repeat, volume,
            !just_data);
  BEST_TIME("sasjon  ", sasjon_compute_stats(p).size(), size, , repeat, volume,
            !just_data);
  BEST_TIME("simdjson (just parse)  ", simdjson_just_parse(p), false, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  (just parse) ", rapid_just_parse(p), false, , repeat,
            volume, !just_data);
  BEST_TIME("sasjon (just parse) ", sasjon_just_parse(p), false, , repeat,
            volume, !just_data);
  simdjson::ParsedJson dsimdjson = simdjson::build_parsed_json(p);
  BEST_TIME("simdjson (just dom)  ", simdjson_just_dom(dsimdjson).size(), size,
            , repeat, volume, !just_data);
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  rapidjson::Document drapid;
  drapid.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  BEST_TIME("rapid  (just dom) ", rapid_just_dom(drapid).size(), size, , repeat,
            volume, !just_data);
  memcpy(buffer, p.data(), p.size());
  auto dsasjon = sajson::parse(sajson::dynamic_allocation(),
                               sajson::mutable_string_view(p.size(), buffer));
  BEST_TIME("sasjon (just dom) ", sasjon_just_dom(dsasjon).size(), size, ,
            repeat, volume, !just_data);
  free(buffer);
}
