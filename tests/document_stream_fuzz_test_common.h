#ifndef SIMDJSON_TESTS_DOCUMENT_STREAM_FUZZ_TEST_COMMON_H
#define SIMDJSON_TESTS_DOCUMENT_STREAM_FUZZ_TEST_COMMON_H

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "simdjson.h"

namespace document_stream_fuzz {

constexpr uint64_t fixed_seed = 0x5eed1234ULL;
constexpr size_t batch_size = 512;
constexpr size_t minimum_total_bytes = 4096;
constexpr size_t minimum_document_count = 128;

struct stream_case {
  const char *name;
  simdjson::stream_format format;
  std::string input;
  std::vector<std::string> expected_documents;
};

inline char random_char(std::mt19937_64 &rng) {
  static constexpr char alphabet[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789 _-/.";
  std::uniform_int_distribution<size_t> dist(0, sizeof(alphabet) - 2);
  return alphabet[dist(rng)];
}

inline std::string make_ascii_text(std::mt19937_64 &rng, size_t min_length, size_t max_length) {
  std::uniform_int_distribution<size_t> length_dist(min_length, max_length);
  const size_t length = length_dist(rng);
  std::string text;
  text.reserve(length);
  for (size_t i = 0; i < length; i++) {
    text.push_back(random_char(rng));
  }
  return text;
}

inline std::string quote_json_string(const std::string &text) {
  std::string quoted;
  quoted.reserve(text.size() + 2);
  quoted.push_back('"');
  for (char ch : text) {
    if (ch == '\\' || ch == '"') {
      quoted.push_back('\\');
    }
    quoted.push_back(ch);
  }
  quoted.push_back('"');
  return quoted;
}

inline std::string make_integer_literal(std::mt19937_64 &rng) {
  std::uniform_int_distribution<int64_t> dist(-1000000, 1000000);
  return std::to_string(dist(rng));
}

inline std::string make_float_literal(std::mt19937_64 &rng) {
  std::uniform_int_distribution<int64_t> whole_dist(-250000, 250000);
  std::uniform_int_distribution<int64_t> fraction_dist(1, 9999);
  return std::to_string(whole_dist(rng)) + "." + std::to_string(fraction_dist(rng));
}

inline std::string make_scalar(std::mt19937_64 &rng) {
  std::uniform_int_distribution<int> type_dist(0, 4);
  switch (type_dist(rng)) {
    case 0: return quote_json_string(make_ascii_text(rng, 0, 20));
    case 1: return make_integer_literal(rng);
    case 2: return make_float_literal(rng);
    case 3: return (rng() & 1) ? "true" : "false";
    default: return "null";
  }
}

inline std::string make_value(std::mt19937_64 &rng, int depth);

inline std::string make_array(std::mt19937_64 &rng, int depth) {
  std::uniform_int_distribution<int> count_dist(0, depth == 0 ? 5 : 3);
  const int count = count_dist(rng);
  std::string out = "[";
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      out.push_back(',');
    }
    out += make_value(rng, depth + 1);
  }
  out.push_back(']');
  return out;
}

inline std::string make_object(std::mt19937_64 &rng, int depth) {
  std::uniform_int_distribution<int> count_dist(0, depth == 0 ? 5 : 3);
  const int count = count_dist(rng);
  std::string out = "{";
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      out.push_back(',');
    }
    std::string key = "k";
    key += std::to_string(depth);
    key += '_';
    key += std::to_string(i);
    key += '_';
    key += make_ascii_text(rng, 1, 6);
    out += quote_json_string(key);
    out.push_back(':');
    out += make_value(rng, depth + 1);
  }
  out.push_back('}');
  return out;
}

inline std::string make_value(std::mt19937_64 &rng, int depth) {
  if (depth >= 3) {
    return make_scalar(rng);
  }
  std::uniform_int_distribution<int> type_dist(0, 6);
  switch (type_dist(rng)) {
    case 0: return quote_json_string(make_ascii_text(rng, 0, 20));
    case 1: return make_integer_literal(rng);
    case 2: return make_float_literal(rng);
    case 3: return (rng() & 1) ? "true" : "false";
    case 4: return "null";
    case 5: return make_array(rng, depth);
    default: return make_object(rng, depth);
  }
}

inline std::vector<std::string> make_documents() {
  std::vector<std::string> docs = {
    "0",
    "-17",
    "3.125",
    "true",
    "false",
    "null",
    "\"alpha beta\"",
    "[]",
    "[1,true,\"x\"]",
    "{}",
    "{\"a\":1,\"b\":[2,3],\"c\":{\"d\":false}}"
  };

  size_t total_bytes = 0;
  for (const auto &doc : docs) {
    total_bytes += doc.size();
  }

  std::mt19937_64 rng(fixed_seed);
  while (docs.size() < minimum_document_count || total_bytes < minimum_total_bytes) {
    docs.push_back(make_value(rng, 0));
    total_bytes += docs.back().size();
  }
  return docs;
}

inline std::vector<std::string> make_wrapped_documents(const std::vector<std::string> &docs) {
  std::vector<std::string> wrapped;
  wrapped.reserve(docs.size());
  for (size_t i = 0; i < docs.size(); i++) {
    wrapped.push_back("{\"id\":" + std::to_string(i) + ",\"value\":" + docs[i] + "}");
  }
  return wrapped;
}

inline std::string build_whitespace_input(const std::vector<std::string> &docs) {
  static const char *separators[] = {" ", "\n", "\r\n", "\t", " \n\t", "\r\t "};
  std::string out = " \n\t";
  for (size_t i = 0; i < docs.size(); i++) {
    out += docs[i];
    if (i + 1 < docs.size()) {
      out += separators[i % (sizeof(separators) / sizeof(separators[0]))];
    }
  }
  out += "\n\t ";
  return out;
}

inline std::string build_json_sequence_input(const std::vector<std::string> &docs) {
  std::string out;
  for (size_t i = 0; i < docs.size(); i++) {
    out.push_back('\x1e');
    out += docs[i];
    out.push_back('\n');
  }
  return out;
}

inline std::string build_comma_delimited_input(const std::vector<std::string> &docs) {
  std::string out;
  for (size_t i = 0; i < docs.size(); i++) {
    if (i > 0) {
      out += (i % 3 == 0) ? ",\n" : ", ";
    }
    out += docs[i];
  }
  return out;
}

inline std::string build_comma_delimited_array_input(const std::vector<std::string> &docs) {
  std::string out = " \t\n[";
  for (size_t i = 0; i < docs.size(); i++) {
    if (i > 0) {
      out += (i % 4 == 0) ? ",\n" : ", ";
    }
    out += docs[i];
  }
  out += "]\r\n";
  return out;
}

inline std::vector<stream_case> make_stream_cases(const std::vector<std::string> &docs) {
  std::vector<std::string> whitespace_docs = make_wrapped_documents(docs);
  return {
    {"whitespace_delimited", simdjson::stream_format::whitespace_delimited, build_whitespace_input(whitespace_docs), whitespace_docs},
    {"json_sequence", simdjson::stream_format::json_sequence, build_json_sequence_input(whitespace_docs), whitespace_docs},
    {"comma_delimited", simdjson::stream_format::comma_delimited, build_comma_delimited_input(whitespace_docs), whitespace_docs},
    {"comma_delimited_array", simdjson::stream_format::comma_delimited_array, build_comma_delimited_array_input(whitespace_docs), whitespace_docs}
  };
}

} // namespace document_stream_fuzz

#endif