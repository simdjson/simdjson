#include "simdjson.h"
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <vector>

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

// clang-format off

// simdjson_recurse below can be implemented like so but it is slow:
/*void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::element element) {
  error_code error;
  if (element.is_array()) {
    dom::array array;
    error = element.get(array);
    for (auto child : array) {
      if (child.is<simdjson::dom::array>() || child.is<simdjson::dom::object>()) {
        simdjson_recurse(v, child);
      }
    }
  } else if (element.is_object()) {
    int64_t id;
    error = element["user"]["id"].get(id);
    if(!error) {
      v.push_back(id);
    }
    for (auto [key, value] : object) {
      if (value.is<simdjson::dom::array>() || value.is<simdjson::dom::object>()) {
        simdjson_recurse(v, value);
      }
    }
  }
}*/
// clang-format on


simdjson_really_inline void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::element element);
void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::array array) {
  for (auto child : array) {
    simdjson_recurse(v, child);
  }
}
void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::object object) {
  for (auto [key, value] : object) {
    if((key.size() == 4) && (memcmp(key.data(), "user", 4) == 0)) {
      // we are in an object under the key "user"
      simdjson::error_code error;
      simdjson::dom::object child_object;
      simdjson::dom::object child_array;
      if (not (error = value.get(child_object))) {
        for (auto [child_key, child_value] : child_object) {
          if((child_key.size() == 2) && (memcmp(child_key.data(), "id", 2) == 0)) {
            int64_t x;
            if (not (error = child_value.get(x))) {
              v.push_back(x);
            }
          }
          simdjson_recurse(v, child_value);
        }
      } else if (not (error = value.get(child_array))) {
        simdjson_recurse(v, child_array);
      }
      // end of: we are in an object under the key "user"
    } else {
      simdjson_recurse(v, value);
    }
  }
}
simdjson_really_inline void simdjson_recurse(std::vector<int64_t> & v, simdjson::dom::element element) {
  simdjson_unused simdjson::error_code error;
  simdjson::dom::array array;
  simdjson::dom::object object;
  if (not (error = element.get(array))) {
    simdjson_recurse(v, array);
  } else if (not (error = element.get(object))) {
    simdjson_recurse(v, object);
  }
}

simdjson_really_inline std::vector<int64_t>
simdjson_just_dom(simdjson::dom::element doc) {
  std::vector<int64_t> answer;
  simdjson_recurse(answer, doc);
  remove_duplicates(answer);
  return answer;
}

simdjson_really_inline std::vector<int64_t>
simdjson_compute_stats(const simdjson::padded_string &p) {
  std::vector<int64_t> answer;
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(p).get(doc);
  if (!error) {
    simdjson_recurse(answer, doc);
    remove_duplicates(answer);
  }
  return answer;
}

simdjson_really_inline simdjson::error_code
simdjson_just_parse(const simdjson::padded_string &p) {
  simdjson::dom::parser parser;
  return parser.parse(p).error();
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

simdjson_really_inline std::vector<int64_t>
sasjon_just_dom(sajson::document &d) {
  std::vector<int64_t> answer;
  sajson_traverse(answer, d.get_root());
  remove_duplicates(answer);
  return answer;
}

simdjson_really_inline std::vector<int64_t>
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

simdjson_really_inline bool
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

simdjson_really_inline std::vector<int64_t>
rapid_just_dom(rapidjson::Document &d) {
  std::vector<int64_t> answer;
  rapid_traverse(answer, d);
  remove_duplicates(answer);
  return answer;
}

simdjson_really_inline std::vector<int64_t>
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

simdjson_really_inline bool
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
  size_t volume = p.size();
  if (just_data) {
    printf(
        "name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson ", simdjson_compute_stats(p).size(), size, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  ", rapid_compute_stats(p).size(), size, , repeat, volume,
            !just_data);
  BEST_TIME("sasjon  ", sasjon_compute_stats(p).size(), size, , repeat, volume,
            !just_data);
  BEST_TIME("simdjson (just parse)  ", simdjson_just_parse(p), simdjson::error_code::SUCCESS, , repeat,
            volume, !just_data);
  BEST_TIME("rapid  (just parse) ", rapid_just_parse(p), false, , repeat,
            volume, !just_data);
  BEST_TIME("sasjon (just parse) ", sasjon_just_parse(p), false, , repeat,
            volume, !just_data);
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  error = parser.parse(p).get(doc);
  BEST_TIME("simdjson (just dom)  ", simdjson_just_dom(doc).size(), size,
            , repeat, volume, !just_data);
  char *buffer = (char *)malloc(p.size() + 1);
  buffer[p.size()] = '\0';
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
