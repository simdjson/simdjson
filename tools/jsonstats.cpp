#include <iostream>
#include <set>

#include "simdjson.h"

SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
#ifndef __cpp_exceptions
#define CXXOPTS_NO_EXCEPTIONS
#endif
#include "cxxopts.hpp"
SIMDJSON_POP_DISABLE_WARNINGS

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
  size_t integer32_count;
  size_t unsigned_integer32_count;
  size_t unsigned_integer_count;
  size_t float_count;
  size_t string_count;
  size_t string_byte_count;
  size_t backslash_count;
  size_t non_ascii_byte_count;
  size_t object_count;
  size_t array_count;
  size_t null_count;
  size_t true_count;
  size_t false_count;
  size_t byte_count;
  size_t structural_indexes_count;
  size_t key_count;
  size_t key_maximum_length;
  size_t maximum_depth;
  size_t ascii_key_count;
  size_t ascii_string_count;
  size_t maximum_object_size;
  size_t maximum_array_size;
  size_t string_maximum_length;
  size_t repeated_key_byte_count;

  bool valid;
  std::set<std::string_view> all_keys;
  std::set<std::string_view> repeated_keys;
};

using stat_t = struct stat_s;

bool is_ascii(const std::string_view &v) {
  for (size_t i = 0; i < v.size(); i++) {
    if (static_cast<unsigned char>(v[i]) >= 128) {
      return false;
    }
  }
  return true;
}

void recurse(simdjson::dom::element element, stat_t &s, size_t depth) {
  if (depth > s.maximum_depth) {
    s.maximum_depth = depth;
  }
  simdjson::error_code error;
  if (element.is<simdjson::dom::array>()) {
    s.array_count++;
    simdjson::dom::array array;
    if (not (error = element.get(array))) {
      size_t counter = 0;
      for (auto child : array) {
        counter++;
        recurse(child, s, depth + 1);
      }
      if (counter > s.maximum_array_size) {
        s.maximum_array_size = counter;
      }
    }
  } else if (element.is<simdjson::dom::object>()) {
    s.object_count++;
    simdjson::dom::object object;
    if (not (error = element.get(object))) {
      size_t counter = 0;
      for (auto [key, value] : object) {
        counter++;
        if(s.all_keys.find(key) != s.all_keys.end()) {
          s.repeated_keys.insert(key);
          s.repeated_key_byte_count += key.size();
        } else {
          s.all_keys.insert(key);
        }
        if (is_ascii(key)) {
          s.ascii_key_count++;
          s.ascii_string_count++;
        }
        if (key.size() > s.key_maximum_length) {
          s.key_maximum_length = key.size();
        }
        if (key.size() > s.string_maximum_length) {
          s.string_maximum_length = key.size();
        }
        s.string_count++;
        s.string_byte_count+= key.size();
        s.key_count++;
        recurse(value, s, depth + 1);
      }
      if (counter > s.maximum_object_size) {
        s.maximum_object_size = counter;
      }
    }
  } else {
    if (element.is<int64_t>()) {
      s.integer_count++; // because an int can be sometimes represented as a double, we
      // to check whether it is an integer first!!!
      int64_t v{};
      error = element.get(v);
      SIMDJSON_ASSUME(!error);
      if((v >= std::numeric_limits<int32_t>::min()) and (v <= std::numeric_limits<int32_t>::max()) ) {
        s.integer32_count++;
      }
      if((v >= std::numeric_limits<uint32_t>::min()) and (v <= std::numeric_limits<uint32_t>::max()) ) {
        s.unsigned_integer32_count++;
      }
    }
    if(element.is<uint64_t>()) { // the else is intentionally missing
      s.unsigned_integer_count++;
    } else if (element.is<double>()) {
      s.float_count++;
    } else if (element.is<bool>()) {
      bool v{};
      error = element.get(v);
      SIMDJSON_ASSUME(!error);
      if (v) {
        s.true_count++;
      } else {
        s.false_count++;
      }
    } else if (element.is_null()) {
      s.null_count++;
    } else if (element.is<std::string_view>()) {
      s.string_count++;
      std::string_view v;
      error = element.get(v);
      SIMDJSON_ASSUME(!error);
      if (is_ascii(v)) {
        s.ascii_string_count++;
      }
      s.string_byte_count+= v.size();
      if (v.size() > s.string_maximum_length) {
        s.string_maximum_length = v.size();
      }
    } else {
      SIMDJSON_UNREACHABLE();
    }
  }
}

stat_t simdjson_compute_stats(const simdjson::padded_string &p) {
  stat_t s{};
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(p).get(doc);
  if (error) {
    s.valid = false;
    std::cerr << error << std::endl;
    return s;
  }
  s.valid = true;
  s.backslash_count =
      count_backslash(reinterpret_cast<const uint8_t *>(p.data()), p.size());
  s.non_ascii_byte_count = count_nonasciibytes(
      reinterpret_cast<const uint8_t *>(p.data()), p.size());
  s.byte_count = p.size();
  s.structural_indexes_count = parser.implementation->n_structural_indexes;

  //  simdjson::document::iterator iter(doc);
  recurse(doc, s, 0);
  return s;
}
#if CXXOPTS__VERSION_MAJOR < 3
int main(int argc, char *argv[]) {
#else
int main(int argc, const char *argv[]) {
#endif
#ifdef __cpp_exceptions
  try {
#endif
  std::string progName = "jsonstat";
  std::string progUsage = "Reads json, prints stats.\n";
  progUsage += argv[0];
  progUsage += " <jsonfile>";

  cxxopts::Options options(progName, progUsage);

  options.add_options()
    ("h,help", "Print usage.")
    ("f,file", "File name.", cxxopts::value<std::string>())
  ;

  options.parse_positional({"file"});
  auto result = options.parse(argc, argv);

  if(result.count("help")) {
    std::cerr << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if(!result.count("file")) {
    std::cerr << "No filename specified." << std::endl;
    std::cerr << options.help() << std::endl;
    return EXIT_FAILURE;
  }

  const char *filename = result["file"].as<std::string>().c_str();

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
  // Future work: the proper way to do the what follows would be to create
  // a JSON object and then to serialize it.

  printf(R"({
      "integer_count"              = %10zu,
      "integer32_count"            = %10zu,
      "unsigned_integer32_count"   = %10zu,
      "unsigned_integer_count"     = %10zu,
      "float_count"                = %10zu,
      "string_count"               = %10zu,
      "string_byte_count"          = %10zu,
      "ascii_string_count"         = %10zu,
      "string_maximum_length"      = %10zu,
      "backslash_count"            = %10zu,
      "non_ascii_byte_count"       = %10zu,
      "object_count"               = %10zu,
      "maximum_object_size"        = %10zu,
      "array_count"                = %10zu,
      "maximum_array_size"         = %10zu,
      "null_count"                 = %10zu,
      "true_count"                 = %10zu,
      "false_count"                = %10zu,
      "byte_count"                 = %10zu,
      "structural_indexes_count"   = %10zu,
      "key_count"                  = %10zu,
      "ascii_key_count"            = %10zu,
      "key_maximum_length"         = %10zu,
      "key_distinct_count"         = %10zu,
      "repeated_key_distinct_count"= %10zu,
      "repeated_key_byte_count"    = %10zu;
      "maximum_depth"              = %10zu
}
)",
         s.integer_count,s.integer32_count,s.unsigned_integer32_count,s.unsigned_integer_count,
         s.float_count, s.string_count, s.string_byte_count, s.ascii_string_count,
         s.string_maximum_length, s.backslash_count, s.non_ascii_byte_count,
         s.object_count, s.maximum_object_size, s.array_count,
         s.maximum_array_size, s.null_count, s.true_count, s.false_count,
         s.byte_count, s.structural_indexes_count, s.key_count,
         s.ascii_key_count, s.key_maximum_length, s.all_keys.size(), s.repeated_keys.size(),
         s.repeated_key_byte_count, s.maximum_depth);
  return EXIT_SUCCESS;
#ifdef __cpp_exceptions
  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif
}
