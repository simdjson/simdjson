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
  bool valid;
};

using stat_t = struct stat_s;

bool is_ascii(const char *p, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (static_cast<unsigned char>(p[i]) >= 128) {
      return false;
    }
  }
  return true;
}

void recurse(simdjson::document::iterator &pjh, stat_t &answer, size_t depth) {
  if (depth > answer.maximum_depth) {
    answer.maximum_depth = depth;
  }
  if (pjh.is_object()) {
    answer.object_count++;
    if (pjh.down()) {
      size_t object_count = 1;
      depth++;
      if (is_ascii(pjh.get_string(), pjh.get_string_length())) {
        answer.ascii_key_count++;
        answer.ascii_string_count++;
      }
      answer.string_count++;
      answer.key_count++;
      size_t len = pjh.get_string_length();
      if (len > answer.key_maximum_length)
        answer.key_maximum_length = len;
      pjh.next();
      recurse(pjh, answer, depth); // let us recurse
      while (pjh.next()) {
        object_count++;
        answer.string_count++;
        pjh.next();
        recurse(pjh, answer, depth); // let us recurse
      }
      if(object_count > answer.maximum_object_size) {
        answer.maximum_object_size = object_count;
      }
      pjh.up();
    }
  } else if (pjh.is_array()) {
    answer.array_count++;
    if (pjh.down()) {
      size_t array_count = 1;
      depth++;
      recurse(pjh, answer, depth);
      while (pjh.next()) {
        array_count++;
        recurse(pjh, answer, depth);
      }
      if(array_count > answer.maximum_array_size) {
        answer.maximum_array_size = array_count;
      }
      pjh.up();
    }
  } else {
    if (pjh.is_double()) {
      answer.float_count++;
    } else if (pjh.is_integer() || pjh.is_unsigned_integer()) {
      answer.integer_count++;
    } else if (pjh.is_false()) {
      answer.false_count++;
    } else if (pjh.is_true()) {
      answer.true_count++;
    } else if (pjh.is_false()) {
      answer.false_count++;
    } else if (pjh.is_null()) {
      answer.null_count++;
    } else if (pjh.is_string()) {
      answer.string_count++;
      if (is_ascii(pjh.get_string(), pjh.get_string_length())) {
        answer.ascii_string_count++;
      }
    } else {
      throw std::runtime_error("unrecognized node.");
    }
  }
}

stat_t simdjson_compute_stats(const simdjson::padded_string &p) {
  stat_t answer;
  simdjson::document::parser parser;
  auto [doc, error] = parser.parse(p);
  if (error) {
    answer.valid = false;
    std::cerr << error << std::endl;
    return answer;
  }
  answer.valid = true;
  answer.backslash_count =
      count_backslash(reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.non_ascii_byte_count = count_nonasciibytes(
      reinterpret_cast<const uint8_t *>(p.data()), p.size());
  answer.byte_count = p.size();
  answer.integer_count = 0;
  answer.float_count = 0;
  answer.object_count = 0;
  answer.array_count = 0;
  answer.null_count = 0;
  answer.true_count = 0;
  answer.false_count = 0;
  answer.string_count = 0;
  answer.key_count = 0;
  answer.key_maximum_length = 0;
  answer.maximum_depth = 0;
  answer.ascii_key_count = 0;
  answer.ascii_string_count = 0;
  answer.maximum_object_size = 0;
  answer.maximum_array_size = 0;
  answer.structural_indexes_count = parser.n_structural_indexes;

  simdjson::document::iterator iter(doc);
  recurse(iter, answer, 0);
  return answer;
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

  printf(R"({
      "integer_count"            = %10zu,
      "float_count"              = %10zu,
      "string_count"             = %10zu,
      "ascii_string_count"       = %10zu,
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
         s.backslash_count, s.non_ascii_byte_count, s.object_count, s.maximum_object_size,
         s.array_count, s.maximum_array_size, s.null_count, s.true_count, s.false_count, s.byte_count,
         s.structural_indexes_count, s.key_count, s.ascii_key_count,
         s.key_maximum_length, s.maximum_depth);
  return EXIT_SUCCESS;
}