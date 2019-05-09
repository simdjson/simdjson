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

void simdjson_traverse(std::vector<int64_t> &answer, ParsedJson::iterator &i) {
  switch (i.get_type()) {
  case '{':
    if (i.down()) {
      do {
        bool founduser = equals(i.get_string(), "user");
        i.next(); // move to value
        if (i.is_object()) {
          if (founduser && i.move_to_key("id")) {
            if (i.is_integer()) {
              answer.push_back(i.get_integer());
            }
            i.up();
          }
          simdjson_traverse(answer, i);
        } else if (i.is_array()) {
          simdjson_traverse(answer, i);
        }
      } while (i.next());
      i.up();
    }
    break;
  case '[':
    if (i.down()) {
      do {
        if (i.is_object_or_array()) {
          simdjson_traverse(answer, i);
        }
      } while (i.next());
      i.up();
    }
    break;
  case 'l':
  case 'd':
  case 'n':
  case 't':
  case 'f':
  default:
    break;
  }
}

std::vector<int64_t> simdjson_computestats(const padded_string &p) {
  std::vector<int64_t> answer;
  ParsedJson pj = build_parsed_json(p);
  if (!pj.isValid()) {
    return answer;
  }
  ParsedJson::iterator i(pj);

  simdjson_traverse(answer, i);
  remove_duplicates(answer);
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
    for (auto i = 0u; i < length; ++i) {
      if (equals(node.get_object_key(i).data(), "user")) { // found a user!!!
        auto uservalue = node.get_object_value(i);         // get the value
        if (uservalue.get_type() ==
            TYPE_OBJECT) { // the value should be an object
          auto uservaluelength = uservalue.get_length();
          for (auto j = 0u; j < uservaluelength;
               ++j) { // go through the children
            if (equals(uservalue.get_object_key(j).data(),
                       "id")) { // ah ah found id
              auto v = uservalue.get_object_value(j);
              if (v.get_type() == TYPE_INTEGER) { // check that it is an integer
                answer.push_back(v.get_integer_value()); // record it!
              } else if (v.get_type() == TYPE_DOUBLE) {
                answer.push_back((int64_t)v.get_double_value()); // record it!
              }
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

std::vector<int64_t> sasjon_computestats(const padded_string &p) {
  std::vector<int64_t> answer;
  char *buffer = (char *)malloc(p.size());
  memcpy(buffer, p.data(), p.size());
  auto d = sajson::parse(sajson::dynamic_allocation(),
                         sajson::mutable_string_view(p.size(), buffer));
  if (!d.is_valid()) {
    return answer;
  }
  sajson_traverse(answer, d.get_root());
  free(buffer);
  remove_duplicates(answer);
  return answer;
}

void rapid_traverse(std::vector<int64_t> &answer, const rapidjson::Value &v) {
  switch (v.GetType()) {
  case kObjectType:
    for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd();
         ++m) {
      if (equals(m->name.GetString(), "user")) {
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

std::vector<int64_t> rapid_computestats(const padded_string &p) {
  std::vector<int64_t> answer;
  char *buffer = (char *)malloc(p.size() + 1);
  memcpy(buffer, p.data(), p.size());
  buffer[p.size()] = '\0';
  rapidjson::Document d;
  d.ParseInsitu<kParseValidateEncodingFlag>(buffer);
  if (d.HasParseError()) {
    return answer;
  }
  rapid_traverse(answer, d);
  free(buffer);
  remove_duplicates(answer);
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
    std::cerr << "warning: ignoring everything after " << argv[optind + 1] << std::endl;
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
  std::vector<int64_t> s1 = simdjson_computestats(p);
  if (verbose) {
    printf("simdjson: ");
    print_vec(s1);
  }
  std::vector<int64_t> s2 = rapid_computestats(p);
  if (verbose) {
    printf("rapid:    ");
    print_vec(s2);
  }
  std::vector<int64_t> s3 = sasjon_computestats(p);
  if (verbose) {
    printf("sasjon:   ");
    print_vec(s3);
  }
  assert(s1 == s2);
  assert(s1 == s3);
  size_t size = s1.size();

  int repeat = 50;
  int volume = p.size();
  if(justdata) {
    printf("name cycles_per_byte cycles_per_byte_err  gb_per_s gb_per_s_err \n");
  }
  BEST_TIME("simdjson  ", simdjson_computestats(p).size(), size, , repeat,
            volume, !justdata);

  BEST_TIME("rapid  ", rapid_computestats(p).size(), size, , repeat, volume,
            !justdata);
  BEST_TIME("sasjon  ", sasjon_computestats(p).size(), size, , repeat, volume,
            !justdata);
}
