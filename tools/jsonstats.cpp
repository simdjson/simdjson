#include <iostream>

#include "simdjson.h"

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
  size_t key_count;
  size_t key_maximum_length;
  size_t maximum_depth;
  size_t ascii_key_count;
  size_t ascii_string_count;
  size_t maximum_object_size;
  size_t maximum_array_size;
  size_t string_maximum_length;
  bool valid;
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
  if (element.is<simdjson::dom::array>()) {
    s.array_count++;
    auto [array, array_error] = element.get<simdjson::dom::array>();
    if (!array_error) {
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
    auto [object, object_error] = element.get<simdjson::dom::object>();
    if (!object_error) {
      size_t counter = 0;
      for (auto [key, value] : object) {
        counter++;
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
        s.key_count++;
        recurse(value, s, depth + 1);
      }
      if (counter > s.maximum_object_size) {
        s.maximum_object_size = counter;
      }
    }
  } else {
    if (element.is<double>()) {
      s.float_count++;
    } else if (element.is<int64_t>()) {
      s.integer_count++;
    } else if (element.is<bool>()) {
      if (element.get<bool>()) {
        s.true_count++;
      } else {
        s.false_count++;
      }
    } else if (element.is_null()) {
      s.null_count++;
    } else if (element.is<std::string_view>()) {
      s.string_count++;
      if (is_ascii(element.get<std::string_view>())) {
        s.ascii_string_count++;
      }
      const std::string_view strval = element.get<std::string_view>();
      if (strval.size() > s.string_maximum_length) {
        s.string_maximum_length = strval.size();
      }
    } else {
      throw std::runtime_error("unrecognized node.");
    }
  }
}

stat_t simdjson_compute_stats(const simdjson::padded_string &p) {
  stat_t s{};
  simdjson::dom::parser parser;
  auto [doc, error] = parser.parse(p);
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
  s.structural_indexes_count = parser.n_structural_indexes;

  //  simdjson::document::iterator iter(doc);
  recurse(doc, s, 0);
  return s;
}

int main(int argc, char *argv[]) {
  int myoptind = 1;
  if (myoptind >= argc) {
    std::cerr << "Reads json, prints stats. " << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    exit(1);
  }
  const char *filename = argv[myoptind];
  if (myoptind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[myoptind + 1]
              << std::endl;
  }

  auto [p, error] = simdjson::padded_string::load(filename);
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
      "integer_count"            = %10zu,
      "float_count"              = %10zu,
      "string_count"             = %10zu,
      "ascii_string_count"       = %10zu,
      "string_maximum_length"    = %10zu,
      "backslash_count"          = %10zu,
      "non_ascii_byte_count"     = %10zu,
      "object_count"             = %10zu,
      "maximum_object_size"      = %10zu,
      "array_count"              = %10zu,
      "maximum_array_size"       = %10zu,
      "null_count"               = %10zu,
      "true_count"               = %10zu,
      "false_count"              = %10zu,
      "byte_count"               = %10zu,
      "structural_indexes_count" = %10zu,
      "key_count"                = %10zu,
      "ascii_key_count"          = %10zu,
      "key_maximum_length"       = %10zu,
      "maximum_depth"            = %10zu
}
)",
         s.integer_count, s.float_count, s.string_count, s.ascii_string_count,
         s.string_maximum_length, s.backslash_count, s.non_ascii_byte_count,
         s.object_count, s.maximum_object_size, s.array_count,
         s.maximum_array_size, s.null_count, s.true_count, s.false_count,
         s.byte_count, s.structural_indexes_count, s.key_count,
         s.ascii_key_count, s.key_maximum_length, s.maximum_depth);
  return EXIT_SUCCESS;
}