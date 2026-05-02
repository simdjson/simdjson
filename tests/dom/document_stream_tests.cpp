#include <cctype>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>
using namespace std::string_literals;

#include "simdjson.h"
#include "test_macros.h"

void print_hex(const simdjson::padded_string& s) {
  printf("hex  : ");
  for(size_t i = 0; i < s.size(); i++) { printf("%02X ", uint8_t(s.data()[i])); }
  printf("\n");
  printf("ascii: ");
  for(size_t i = 0; i < s.size(); i++) {
    auto v = uint8_t(s.data()[i]);
    if((v <= 32) || (v >= 127)) {
      printf(" __");
    } else {
      printf("%c__", v);
    }
  }
  printf("\n");
}

int char_to_byte(char character) {
  if (('A' <= character && character <= 'Z')) {
    return (character - 'A');
  } else if (('a' <= character && character <= 'z')) {
    return 26 + (character - 'a');
  } else if (('0' <= character && character <= '9')) {
    return 52 + (character - '0');
  } else if (character == '+') {
    return 62;
  } else if (character == '/') {
    return 63;
  } else if (character == '=') {
    return 0;
  }
  return -1;
}

std::string decode_base64(const std::string &src) {
  std::vector<uint8_t> answer;
  for (size_t i = 0; i < src.size(); i += 4) {
    int three_bytes = char_to_byte(src[i]) << 18 |
                      char_to_byte(src[i + 1]) << 12 |
                      char_to_byte(src[i + 2]) << 6 | char_to_byte(src[i + 3]);
    if (three_bytes < 0) {
      std::cerr << "invalid base64" << std::endl;
      abort();
    }
    answer.push_back(uint8_t((three_bytes & 0x00FF0000) >> 16));
    answer.push_back(uint8_t((three_bytes & 0x0000FF00) >> 8));
    answer.push_back(uint8_t(three_bytes & 0x000000FF));
  }
  return std::string(answer.begin(), answer.end());
}


std::string trim(const std::string s) {
	auto start = s.begin();
	auto end = s.end();
	while (start != s.end() && std::isspace(*start)) {
		start++;
	}
	do {
		end--;
	} while (std::distance(start, end) > 0 && std::isspace(*end));
	return std::string(start, end + 1);
}

namespace document_stream_tests {
  static simdjson::dom::document_stream parse_many_stream_return(simdjson::dom::parser &parser, simdjson::padded_string &str) {
    simdjson::dom::document_stream stream;
    simdjson_unused auto error = parser.parse_many(str).get(stream);
    return stream;
  }
  // this is a compilation test
  simdjson_unused static void parse_many_stream_assign() {
      simdjson::dom::parser parser;
      simdjson::padded_string str("{}",2);
      simdjson::dom::document_stream s1 = parse_many_stream_return(parser, str);
  }

  bool stress_data_race() {
    std::cout << "Running " << __func__ << std::endl;
    // Correct JSON.
    const simdjson::padded_string input = R"([1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(input, 32).get(stream));
    for(auto doc: stream) {
      auto error = doc.error();
      if(error) {
        std::cout << "Expected no error but got " << error << std::endl;
        return false;
      }
    }
    return true;
  }

  bool stress_data_race_with_error() {
    std::cout << "Running " << __func__ << std::endl;
    // Intentionally broken
    const simdjson::padded_string input = R"([1,23] [1,23] [1,23] [1,23 [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(input, 32).get(stream));
    size_t count = 0;
    for(auto doc: stream) {
          auto error = doc.error();
          if(count <= 2) {
            if(error) {
              std::cout << "Expected no error but got " << error << std::endl;
              return false;
            }
          } else {
            if(!error) {
              std::cout << "Expected an error but got " << error << std::endl;
              return false;
            }
            break;
          }
          count++;
    }
    return true;
  }

  bool test_leading_spaces() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = R"(                               [1,23] [1,23] [1,23]  [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;;
    size_t count = 0;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(input, 32).get(stream));
    count = 0;
    for(auto doc: stream) {
      auto error = doc.error();
      if(error) {
        std::cout << "Expected no error but got " << error << std::endl;
        return false;
      }
      count++;
    }
    return count == 15;
  }


  bool test_crazy_leading_spaces() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = R"(                                                                                                                                                           [1,23] [1,23] [1,23]  [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] [1,23] )"_padded;;
    size_t count = 0;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(input, 32).get(stream));
    count = 0;
    for(auto doc: stream) {
      auto error = doc.error();
      if(error) {
        std::cout << "Expected no error but got " << error << std::endl;
        return false;
      }
      count++;
    }
    return count == 15;
  }

  bool issue1307() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = decode_base64("AgAMACA=");
    print_hex(input);
    for(size_t window = 0; window <= 100; window++) {
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(input, window).get(stream));
      for(auto doc: stream) {
        auto error = doc.error();
        if(!error) {
          std::cout << "Expected an error but got " << error << std::endl;
          std::cout << "Window = " << window << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  bool issue1308() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = decode_base64("bcdtW0E=");
    print_hex(input);
    for(size_t window = 0; window <= 100; window++) {
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(input, window).get(stream));
      for(auto doc: stream) {
        auto error = doc.error();
        if(!error) {
          std::cout << "Expected an error but got " << error << std::endl;
          std::cout << "Window = " << window << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  bool issue1309() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = decode_base64("CQA5OAo5CgoKCiIiXyIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiJiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiXyIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiJiIiIiIiIiIiIiIiIiIiIiLb29vb29vb29vb29vb29vz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz29vb29vb29vbIiIiIiIiIiIiIiIiIiIiIiIiIiIiJiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiYiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiIiI=");
    print_hex(input);
    for(size_t window = 0; window <= 100; window++) {
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(input, window).get(stream));
      for(auto doc: stream) {
        auto error = doc.error();
        if(!error) {
          std::cout << "Expected an error but got " << error << std::endl;
          std::cout << "Window = " << window << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  bool issue2170() {
      TEST_START();
      auto json = R"(1 2 3)"_padded;
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(json).get(stream));
      auto i = stream.begin();
      size_t count{0};
      std::vector<size_t> indexes = { 0, 2, 4 };
      std::vector<std::string_view> expected = { "1", "2", "3" };

      for(; i != stream.end(); ++i) {
          auto doc = *i;
          ASSERT_SUCCESS(doc);
          ASSERT_TRUE(count < 3);
          ASSERT_EQUAL(i.current_index(), indexes[count]);
          ASSERT_EQUAL(i.source(), expected[count]);
          count++;
      }
      TEST_SUCCEED();
  }

  bool issue2181() {
      TEST_START();
      auto json = R"(1 2 34)"_padded;
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(json).get(stream));
      auto i = stream.begin();
      size_t count{0};
      std::vector<size_t> indexes = { 0, 2, 4 };
      std::vector<std::string_view> expected = { "1", "2", "34" };

      for(; i != stream.end(); ++i) {
          auto doc = *i;
          ASSERT_SUCCESS(doc);
          ASSERT_TRUE(count < 3);
          ASSERT_EQUAL(i.current_index(), indexes[count]);
          ASSERT_EQUAL(i.source(), expected[count]);
          count++;
      }
      TEST_SUCCEED();
  }

  bool issue_non_ascii_separator_source() {
      TEST_START();
      std::string bytes = "1 ";
      bytes.push_back(char(0xFF));
      bytes += " 2";
      simdjson::padded_string json(bytes);

      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(json).get(stream));

      auto i = stream.begin();
      ASSERT_TRUE(i != stream.end());
      std::string_view source = i.source();
      ASSERT_TRUE(!source.empty());
      ASSERT_EQUAL(source.front(), '1');
      TEST_SUCCEED();
  }
  bool issue1310() {
    std::cout << "Running " << __func__ << std::endl;
    // hex  : 20 20 5B 20 33 2C 31 5D 20 22 22 22 22 22 22 22 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
    // ascii:  __ __[__ __3__,__1__]__ __"__"__"__"__"__"__"__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
    // We have four full documents followed by an unclosed string.
    const simdjson::padded_string input = decode_base64("ICBbIDMsMV0gIiIiIiIiIiAgICAgICAgICAgICAgICAg");
    print_hex(input);
    for(size_t window = 0; window <= 100; window++) {
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(input, window).get(stream));
      size_t count{0};
      for(auto doc: stream) {
        auto error = doc.error();
        count++;
        if(count <= 4) {
          if(window < input.size() && error) {
            std::cout << "Expected no error but got " << error << std::endl;
            std::cout << "Window = " << window << std::endl;
            return false;
          }
        } else {
          if(!error) {
            std::cout << "Expected an error but got " << error << std::endl;
            std::cout << "Window = " << window << std::endl;
            return false;
          }
        }
      }
    }
    return true;
  }

  bool issue1311() {
    std::cout << "Running " << __func__ << std::endl;
    const simdjson::padded_string input = decode_base64("NSMwW1swDPw=");
    print_hex(input);
    for(size_t window = 0; window <= 100; window++) {
      simdjson::dom::parser parser;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS(parser.parse_many(input, window).get(stream));
      for(auto doc: stream) {
        auto error = doc.error();
        if(!error) {
          std::cout << "Expected an error but got " << error << std::endl;
          std::cout << "Window = " << window << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  bool test_current_index() {
    std::cout << "Running " << __func__ << std::endl;
    std::string base1("1         ");// one JSON!
    std::string base2("{\"k\":1}   ");// one JSON!
    std::string base3("[1,2]     ");// one JSON!
    assert(base1.size() == base2.size());
    assert(base2.size() == base3.size());
    std::vector<std::string> source_strings = {base1, base2, base3};

    std::string json;
    for(size_t k =  0; k < 1000; k++) {
      json += base1;
      json += base2;
      json += base3;
    }
    simdjson::dom::parser parser;
    const size_t window = 32; // deliberately small
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json,window).get(stream) );
    auto i = stream.begin();
    size_t count = 0;
    for(; i != stream.end(); ++i) {
      auto doc = *i;
      ASSERT_SUCCESS(doc);
      if( i.current_index() != count) {
        std::cout << "index:" << i.current_index() << std::endl;
        std::cout << "expected index:" << count << std::endl;
        return false;
      }
      std::string answer = source_strings[(count / base1.size()) % source_strings.size()];
      if(trim(std::string(i.source())) != trim(answer)) {
        std::cout << "got:       '" << i.source() << "'" << std::endl;
        std::cout << "expected : '" << answer << "'"  << std::endl;
        return false;
      }
      count += base1.size();
    }
    return true;
  }

  bool test_naked_iterators() {
    std::cout << "Running " << __func__ << std::endl;
    auto json = R"([1,23] "lone string" {"key":"unfinished value}  )"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json).get(stream) );
    size_t count = 0;
    simdjson::dom::document_stream::iterator previous_i; // just to check we can copy iters
    // We do not touch the document, intentionally.
    for(auto i = stream.begin(); i != stream.end(); ++i) {
      if(count > 10) { break; }
      count++;
      previous_i = i;
    }
    // We should have two documents
    if(count != 2) {
      std::cout << "finished with count = " << count << std::endl;
      return false;
    }
    return true;
  }

  bool adversarial_single_document() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = R"({"f[)"_padded;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(json).get(stream));
    size_t count = 0;
    for (auto doc : stream) {
        (void)doc;
        count += 1;
    }
    std::cout << "number of documents (0 expected) = " << count << std::endl;
    return count == 0;
  }

  bool adversarial_single_document_array() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = R"(["this is an unclosed string ])"_padded;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(json).get(stream));
    size_t count = 0;
    for (auto doc : stream) {
        (void)doc;
        count += 1;
    }
    std::cout << "number of documents (0 expected) = " << count << std::endl;
    return count == 0;
  }

  bool single_document() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = R"({"hello": "world"})"_padded;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(json).get(stream));
    size_t count = 0;
    for (auto doc : stream) {
        if(doc.error()) {
          std::cerr << "Unexpected error: " << doc.error() << std::endl;
          return false;
        }
        std::string expected = R"({"hello":"world"})";
        simdjson::dom::element this_document;
        ASSERT_SUCCESS(doc.get(this_document));

        std::string answer = simdjson::minify(this_document);
        if(answer != expected) {
          std::cout << this_document << std::endl;
          return false;
        }
        count += 1;
    }
    std::cout << "number of documents " << count << std::endl;
    return count == 1;
  }

  bool skipbom() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = "\xEF\xBB\xBF{\"hello\": \"world\"}"_padded;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS(parser.parse_many(json).get(stream));
    size_t count = 0;
    for (auto doc : stream) {
        if(doc.error()) {
          std::cerr << "Unexpected error: " << doc.error() << std::endl;
          return false;
        }
        std::string expected = R"({"hello":"world"})";
        simdjson::dom::element this_document;
        ASSERT_SUCCESS(doc.get(this_document));

        std::string answer = simdjson::minify(this_document);
        if(answer != expected) {
          std::cout << this_document << std::endl;
          return false;
        }
        count += 1;
    }
    std::cout << "number of documents " << count << std::endl;
    return count == 1;
  }
#if SIMDJSON_EXCEPTIONS
  bool single_document_exceptions() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = R"({"hello": "world"})"_padded;
    size_t count = 0;
    for (simdjson::dom::element doc : parser.parse_many(json)) {
        std::string expected = R"({"hello":"world"})";
        std::string answer = simdjson::minify(doc);
        if(answer != expected) {
          std::cout << "got     : "  << answer << std::endl;
          std::cout << "expected: "  << expected << std::endl;
          return false;
        }
        count += 1;
    }
    return count == 1;
  }

  bool issue1133() {
    std::cout << "Running " << __func__ << std::endl;
    simdjson::dom::parser parser;
    auto json = "{\"hello\": \"world\"}"_padded;
    simdjson::dom::document_stream docs = parser.parse_many(json);
    size_t count = 0;
    for (simdjson::dom::element doc : docs) {
        std::string expected = R"({"hello":"world"})";
        std::string answer = simdjson::minify(doc);
        if(answer != expected) {
          std::cout << "got     : "  << answer << std::endl;
          std::cout << "expected: "  << expected << std::endl;
          return false;
        }
        count += 1;
    }
    if(docs.truncated_bytes() != 0) {
      std::cerr << "Unexpected truncation : " << docs.truncated_bytes() << std::endl;
      return false;
    }
    return count == 1;
  }
#endif
  bool simple_example() {
    std::cout << "Running " << __func__ << std::endl;
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2,3]  )"_padded;
    simdjson::dom::parser parser;
    size_t count = 0;
    simdjson::dom::document_stream stream;
    // We use a window of json.size() though any large value would do.
    ASSERT_SUCCESS( parser.parse_many(json, json.size()).get(stream) );
    auto i = stream.begin();
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if(!doc.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << i.source() << std::endl;
          count++;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
          return false;
        }
    }
    if(count != 3) {
      std::cerr << "Expected to get three full documents " << std::endl;
      return false;
    }
    if(stream.truncated_bytes() != 0) {
      std::cerr << "Unexpected truncation : " << stream.truncated_bytes() << std::endl;
      return false;
    }
    size_t index = i.current_index();
    if(index != 38) {
      std::cerr << "Expected to stop after the three full documents " << std::endl;
      std::cerr << "index = " << index << std::endl;
      return false;
    }
    return true;
  }

  bool unquoted_key() {
    std::cout << "Running " << __func__ << std::endl;
    auto json = R"({unquoted_key: "keys must be quoted"})"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    // We use a window of json.size() though any large value would do.
    ASSERT_SUCCESS( parser.parse_many(json, json.size()).get(stream) );
    auto i = stream.begin();
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if(!doc.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << i.source() << std::endl;
          return false;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
          return true;
        }
    }
    return false;
  }


  bool truncated_window() {
    std::cout << "Running " << __func__ << std::endl;
    // The last JSON document is
    // intentionally truncated.
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2  )"_padded;
    std::cout << "input size " << json.size() << std::endl;
    simdjson::dom::parser parser;
    size_t count = 0;
    simdjson::dom::document_stream stream;
    // We use a window of json.size() though any large value would do.
    ASSERT_SUCCESS( parser.parse_many(json, json.size()).get(stream) );
    auto i = stream.begin();
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if(!doc.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << i.source() << std::endl;
          count++;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
        }
    }
    if(count != 2) {
      std::cerr << "Expected to get two full documents " << std::endl;
      return false;
    }
    if(stream.truncated_bytes() == 0) {
      std::cerr << "Expected truncation : " << stream.truncated_bytes() << std::endl;
      return false;
    }
    if(stream.truncated_bytes() != 6) {
      std::cerr << "Expected truncation of 6 bytes got " << stream.truncated_bytes() << std::endl;
      return false;
    }
    return true;
  }

  bool truncated_window_unclosed_string() {
    std::cout << "Running " << __func__ << std::endl;
    // The last JSON document is intentionally truncated.
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} "intentionally unclosed string  )"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    // We use a window of json.size() though any large value would do.
    ASSERT_SUCCESS( parser.parse_many(json,json.size()).get(stream) );
    auto i = stream.begin();
    size_t counter{0};
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if(!doc.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << "the document is " << i.source() << std::endl;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
          std::cout << doc.error() << std::endl;
        }
        counter++;
    }
    std::cout << "final index is " << i.current_index() << std::endl;
    if(counter != 2) {
      std::cerr << "You should have parsed two documents. I found " << counter << "." << std::endl;
      return false;
    }
    if(stream.truncated_bytes() == 0) {
      std::cerr << "Expected truncation : " << stream.truncated_bytes() << std::endl;
      return false;
    }
    if(stream.truncated_bytes() != 32) {
      std::cerr << "Expected truncation of 32 bytes got " << stream.truncated_bytes() << std::endl;
      return false;
    }
    return true;
  }


  bool truncated_window_unclosed_string_in_object() {
    std::cout << "Running " << __func__ << std::endl;
    // The last JSON document is intentionally truncated.
    auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} {"key":"intentionally unclosed string  )"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;
    // We use a window of json.size() though any large value would do.
    ASSERT_SUCCESS( parser.parse_many(json,json.size()).get(stream) );
    auto i = stream.begin();
    size_t counter{0};
    for(; i != stream.end(); ++i) {
        auto doc = *i;
        if(!doc.error()) {
          std::cout << "got full document at " << i.current_index() << std::endl;
          std::cout << "the document is " << i.source() << std::endl;
        } else {
          std::cout << "got broken document at " << i.current_index() << std::endl;
          std::cout << doc.error() << std::endl;
        }
        counter++;
    }
    std::cout << "final index is " << i.current_index() << std::endl;
    if(counter != 2) {
      std::cerr << "You should have parsed two documents. I found " << counter << "." << std::endl;
      return false;
    }
    if(stream.truncated_bytes() == 0) {
      std::cerr << "Expected truncation : " << stream.truncated_bytes() << std::endl;
      return false;
    }
    if(stream.truncated_bytes() != 39) {
      std::cerr << "Expected truncation of 39 bytes got " << stream.truncated_bytes() << std::endl;
      return false;
    }
    return true;
  }

  bool issue1668() {
    TEST_START();
    auto json = R"([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;
    simdjson::dom::parser odparser;
    simdjson::dom::document_stream odstream;
    ASSERT_SUCCESS( odparser.parse_many(json.data(), json.length(), 50).get(odstream) );
    for (auto doc: odstream) {
      simdjson::dom::element val;
      ASSERT_ERROR(doc.at_pointer("/40").get(val), simdjson::CAPACITY);
      ASSERT_EQUAL(odstream.truncated_bytes(), json.length());
    }
    TEST_SUCCEED();
  }

  bool issue1668_long() {
    TEST_START();
    auto json = R"([1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;
    simdjson::dom::parser odparser;
    simdjson::dom::document_stream odstream;
    size_t counter{0};
    ASSERT_SUCCESS( odparser.parse_many(json.data(), json.length(), 50).get(odstream) );
    for (auto doc: odstream) {
      if(counter < 6) {
        int64_t val{};
        ASSERT_SUCCESS(doc.at_pointer("/4").get(val));
        ASSERT_EQUAL(val, 5);
      } else {
        simdjson::dom::element val;
        ASSERT_ERROR(doc.at_pointer("/4").get(val), simdjson::CAPACITY);
        // We left 293 bytes unprocessed.
        ASSERT_EQUAL(odstream.truncated_bytes(), 293);
      }
      counter++;
    }
    TEST_SUCCEED();
  }

  bool small_window() {
    std::cout << "Running " << __func__ << std::endl;
    std::vector<char> input;
    input.push_back('[');
    for(size_t i = 1; i < 1024; i++) {
      input.push_back('1');
      input.push_back(i < 1023 ? ',' : ']');
    }
    auto json = simdjson::padded_string(input.data(),input.size());
    simdjson::dom::parser parser;
    size_t count = 0;
    size_t window_size = 1024; // deliberately too small
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json, window_size).get(stream) );
    for (auto doc : stream) {
      if (!doc.error()) {
          std::cerr << "Expected a capacity error " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
    if(count == 2) {
      std::cerr << "Expected a capacity error " << std::endl;
      return false;
    }
    return true;
  }

  bool window_too_small_issue1370() {
    std::cout << "Running " << __func__ << std::endl;
    std::vector<char> input;
    input.push_back('[');
    for(size_t i = 1; i < 1024; i++) {
      input.push_back('1');
      input.push_back(i < 1023 ? ',' : ']');
    }
    auto json = simdjson::padded_string(input.data(), input.size());
    // We are going to repeat this test 1000 times so
    // that if there is an issue, we are more likely to
    // trigger it systematically.
    for(size_t trial = 0; trial < 1000; trial++) {
      std::cout << ".";
      std::cout.flush();
      simdjson::dom::parser parser;
      size_t count = 0;
      size_t window_size = 1024; // deliberately too small
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS( parser.parse_many(json, window_size).get(stream) );
      for (auto doc : stream) {
        if (!doc.error()) {
          std::cerr << "Expected a capacity error but got: " << doc.error() << std::endl;
          std::cerr << "The input was: " << json << std::endl;
          simdjson::dom::element this_document;
          ASSERT_SUCCESS(doc.get(this_document));
          std::cerr << "We parsed the document: " << this_document << std::endl;
          return false;
        }
        count++;
      }
      if(count == 2) {
        std::cerr << "Expected a single document " << std::endl;
        return false;
      }
    }
    std::cout << std::endl;
    return true;
  }

  bool large_window() {
    std::cout << "Running " << __func__ << std::endl;
#if SIZE_MAX > 17179869184
    auto json = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;
    simdjson::dom::parser parser;
    size_t count = 0;
    uint64_t window_size{17179869184}; // deliberately too big
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json, size_t(window_size)).get(stream) );
    for (auto doc : stream) {
      if (!doc.error()) {
          std::cerr << "I expected a failure (too big) but got  " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
    if(count != 1) {
        std::cerr << "bad count" << std::endl;
        return false;
    }
#endif
    return true;
  }
  static bool parse_json_message_issue467(simdjson::padded_string &json, size_t expectedcount) {
    simdjson::dom::parser parser;
    size_t count = 0;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json).get(stream) );
    for (auto doc : stream) {
      if (doc.error()) {
          std::cerr << "Failed with simdjson error= " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
    if(count != expectedcount) {
        std::cerr << "bad count" << std::endl;
        return false;
    }
    return true;
  }

  bool json_issue467() {
    std::cout << "Running " << __func__ << std::endl;
    auto single_message = R"({"error":[],"result":{"token":"xxx"}})"_padded;
    auto two_messages = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;

    if(!parse_json_message_issue467(single_message, 1)) {
      return false;
    }
    if(!parse_json_message_issue467(two_messages, 2)) {
      return false;
    }
    return true;
  }

  // returns true if successful
  bool document_stream_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 100;
    std::string data;
    std::vector<char> buf(1024);
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf.data(),
                          buf.size(),
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                      i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
      if (n >= buf.size()) { abort(); }
      data += std::string(buf.data(), n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS( parser.parse_many(str, batch_size).get(stream) );
      for (auto doc : stream) {
        int64_t keyid{};
        ASSERT_SUCCESS( doc["id"].get(keyid) );
        ASSERT_EQUAL( keyid, int64_t(count) );

        count++;
      }
      if(count != n_records) {
        printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
        return false;
      }
    }
    printf("ok\n");
    return true;
  }

  // returns true if successful
  bool document_stream_utf8_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 100;
    std::string data;
    std::vector<char> buf(1024);
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf.data(),
                        buf.size(),
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"\xC3\xA9t\xC3\xA9\": {\"id\": %zu, \"name\": \"\xC3\xA9ventail%zu\"}}",
                      i, i, (i % 2) ? "\xE2\xBA\x83" : "\xE2\xBA\x95", i % 10, i % 10);
      if (n >= buf.size()) { abort(); }
      data += std::string(buf.data(), n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS( parser.parse_many(str, batch_size).get(stream) );
      for (auto doc : stream) {
        int64_t keyid{};
        ASSERT_SUCCESS( doc["id"].get(keyid) );
        ASSERT_EQUAL( keyid, int64_t(count) );

        count++;
      }
      ASSERT_EQUAL( count, n_records )
    }
    printf("ok\n");
    return true;
  }

  bool issue1649() {
    std::cout << "Running " << __func__ << std::endl;
    std::size_t batch_size = 637;
    const auto json="\xd7"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream docs;
    if(parser.parse_many(json,batch_size).get(docs)) {
          return false;
    }
    size_t bool_count=0;
    for (auto doc : docs) {
      bool_count += doc.is_bool();
    }
    return (bool_count == 0);
  }

  bool fuzzaccess() {
    std::cout << "Running " << __func__ << std::endl;
    // Issue 38801 in oss-fuzz
    auto json = "\xff         \n~~\n{}"_padded;
    simdjson::dom::parser parser;
    simdjson::dom::document_stream docs;
    ASSERT_SUCCESS(parser.parse_many(json).get(docs));
    size_t bool_count = 0;
    size_t total_count = 0;

    for (auto doc : docs) {
      total_count++;
      bool_count += doc.is_bool();
    }
    return (bool_count == 0) && (total_count == 1);
  }

  bool baby_fuzzer() {
    std::cout << "Running " << __func__ << std::endl;
    std::mt19937 gen(637);
    std::uniform_int_distribution<size_t> bs(0,1000);
    std::uniform_int_distribution<size_t> len(0,10);
    std::uniform_int_distribution<int> ascii;
    for(size_t i = 0; i < 100000; i++) {
      if((i%1000) == 0) { std::cout << "."; std::cout.flush(); }
      size_t batch_size = bs(gen);
      const size_t l = len(gen);
      char * buffer = new char[l];
      for(size_t z = 0; z < l; z++) {
        buffer[z] = char(ascii(gen));
      }
      const auto json = simdjson::padded_string(buffer, l);
      delete[] buffer;
      simdjson::dom::parser parser;
      simdjson::dom::document_stream docs;
      if(parser.parse_many(json,batch_size).get(docs)) {
        return false;
      }
      simdjson_unused size_t bool_count = 0;
      for (auto doc : docs) {
        bool_count += doc.is_bool();
      }
    }
    std::cout << std::endl;
    return true;
  }

  bool json_sequence_tests() {
    TEST_START();
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;

    // Verify enum values
    static_assert(static_cast<int>(simdjson::stream_format::whitespace_delimited) == 0, "whitespace_delimited should be 0");
    static_assert(static_cast<int>(simdjson::stream_format::json_sequence) == 1, "json_sequence should be 1");

    SUBTEST("whitespace_delimited format works", ([&]() {
      auto input = R"({"a":1} {"b":2} {"c":3})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::whitespace_delimited).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("basic RS-delimited objects with LF", ([&]() {
      auto input = "\x1e{\"a\":1}\n\x1e{\"b\":2}\n\x1e{\"c\":3}\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      const char* keys[] = {"a", "b", "c"};
      int64_t expected[] = {1, 2, 3};
      size_t count = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        int64_t value;
        ASSERT_SUCCESS(doc[keys[count]].get(value));
        ASSERT_EQUAL(value, expected[count]);
        count++;
      }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("multiple documents without trailing LF", ([&]() {
      auto input = "\x1e{\"a\":1}\x1e{\"b\":2}\x1e{\"c\":3}"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("single document without LF", ([&]() {
      auto input = "\x1e{\"a\":1}"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(1));
      return true;
    }()));

    SUBTEST("scalar documents", ([&]() {
      auto input = "\x1e" "42\n" "\x1e" "\"hello\"\n" "\x1e" "true\n" "\x1e" "null\n" "\x1e" "[1,2,3]\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(5));
      return true;
    }()));

    SUBTEST("current_index points to JSON value not RS", ([&]() {
      // Layout: RS(0) 1(1) LF(2) RS(3) 2(4) LF(5)
      auto input = "\x1e" "1\n" "\x1e" "2\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.current_index(), size_t(1));  // "1" at position 1
      ++it;
      ASSERT_EQUAL(it.current_index(), size_t(4));  // "2" at position 4
      return true;
    }()));

    SUBTEST("RS-only input produces 0 documents", ([&]() {
      auto input = "\x1e\n\x1e\n"_padded;
      auto result = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream);
      if (result == simdjson::SUCCESS) {
        size_t count = 0;
        for (auto doc : stream) { (void)doc; count++; }
        ASSERT_EQUAL(count, size_t(0));
      }
      // EMPTY error is also acceptable
      return true;
    }()));

    SUBTEST("RS-only input variants accept EMPTY or zero documents", ([&]() {
      // Several RS-only patterns. For each, parse_many must either:
      //   (a) succeed and yield zero documents on iteration, or
      //   (b) report simdjson::EMPTY.
      // Any other outcome (including a non-EMPTY error, or producing
      // ghost documents) is a failure.
      const std::string inputs[] = {
        "\x1e"s,                  // single RS, no LF
        "\x1e\n"s,                // RS + LF
        "\x1e\x1e"s,              // back-to-back RS
        "\x1e \t\n"s,             // RS + whitespace
        "\x1e\n\x1e\n\x1e\n"s,    // three RSes, no payloads
      };
      for (const auto& s : inputs) {
        auto input = simdjson::padded_string(s);
        auto result = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream);
        if (result == simdjson::SUCCESS) {
          size_t count = 0;
          for (auto doc : stream) { (void)doc; count++; }
          ASSERT_EQUAL(count, size_t(0));
        } else {
          ASSERT_EQUAL(result, simdjson::EMPTY);
        }
      }
      return true;
    }()));

    SUBTEST("string documents current_index and source", ([&]() {
      // Layout: RS(0) "(1) h(2) e(3) l(4) l(5) o(6) "(7) LF(8)
      //         RS(9) "(10) w(11) o(12) r(13) l(14) d(15) "(16) LF(17)
      // Regression test: when RS is followed by a JSON string, the
      // first structural after the RS is the opening quote. The stream
      // must report the document position at the opening quote (not the
      // RS) and source() must yield the quoted string verbatim.
      auto input = "\x1e\"hello\"\n\x1e\"world\"\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.current_index(), size_t(1));
      ASSERT_EQUAL(it.source(), std::string_view("\"hello\""));
      std::string_view s1;
      ASSERT_SUCCESS((*it).get(s1));
      ASSERT_EQUAL(s1, std::string_view("hello"));
      ++it;
      ASSERT_EQUAL(it.current_index(), size_t(10));
      ASSERT_EQUAL(it.source(), std::string_view("\"world\""));
      std::string_view s2;
      ASSERT_SUCCESS((*it).get(s2));
      ASSERT_EQUAL(s2, std::string_view("world"));
      return true;
    }()));

    SUBTEST("whitespace between RS and value", ([&]() {
      auto input = "\x1e  {\"a\":1}\n\x1e\t\n{\"b\":2}\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(2));
      return true;
    }()));

    SUBTEST("mixed document types in sequence", ([&]() {
      auto input = "\x1e" "123\n" "\x1e" "{\"x\":1}\n" "\x1e" "[1,2]\n" "\x1e" "\"str\"\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      // Doc 0: number
      int64_t num; ASSERT_SUCCESS((*it).get(num)); ASSERT_EQUAL(num, int64_t(123)); ++it;
      // Doc 1: object
      int64_t x; ASSERT_SUCCESS((*it)["x"].get(x)); ASSERT_EQUAL(x, int64_t(1)); ++it;
      // Doc 2: array
      simdjson::dom::array arr; ASSERT_SUCCESS((*it).get(arr)); ++it;
      // Doc 3: string
      std::string_view s; ASSERT_SUCCESS((*it).get(s)); ASSERT_EQUAL(s, std::string_view("str"));
      return true;
    }()));

    SUBTEST("source returns correct document slice", ([&]() {
      auto input = "\x1e" "[1,2,3]\n" "\x1e" "{\"a\":1}\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.source(), std::string_view("[1,2,3]"));
      ++it;
      ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
      return true;
    }()));

    SUBTEST("multibatch current_index and source", ([&]() {
      auto input = "\x1e {\"a\":1}\n\x1e\t{\"b\":2}\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, 10, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.current_index(), size_t(2));
      ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
      ++it;
      ASSERT_EQUAL(it.current_index(), size_t(12));
      ASSERT_EQUAL(it.source(), std::string_view("{\"b\":2}"));
      return true;
    }()));

    SUBTEST("small batch reports capacity", ([&]() {
      std::string json_sequence_input = "\x1e[";
      for (size_t i = 0; i < 2000; i++) {
        json_sequence_input += '1';
        json_sequence_input += (i + 1 < 2000 ? ',' : ']');
      }
      json_sequence_input += '\n';
      auto input = simdjson::padded_string(json_sequence_input);
      ASSERT_SUCCESS(parser.parse_many(input, 1024, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();
      auto doc = *it;
      ASSERT_ERROR(doc.error(), simdjson::CAPACITY);
      return true;
    }()));

    SUBTEST("scalar trio: number, true, string", ([&]() {
      // Layout: RS(0) 1(1) LF(2) RS(3) t(4) r(5) u(6) e(7) LF(8)
      //         RS(9) "(10) x(11) "(12) LF(13)
      auto input = "\x1e" "1\n" "\x1e" "true\n" "\x1e" "\"x\"\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();

      // doc 0: number 1
      ASSERT_EQUAL(it.current_index(), size_t(1));
      ASSERT_EQUAL(it.source(), std::string_view("1"));
      int64_t i; ASSERT_SUCCESS((*it).get(i)); ASSERT_EQUAL(i, int64_t(1));
      ++it;

      // doc 1: true
      ASSERT_EQUAL(it.current_index(), size_t(4));
      ASSERT_EQUAL(it.source(), std::string_view("true"));
      bool b{}; ASSERT_SUCCESS((*it).get(b)); ASSERT_TRUE(b);
      ++it;

      // doc 2: "x"
      ASSERT_EQUAL(it.current_index(), size_t(10));
      ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
      std::string_view s; ASSERT_SUCCESS((*it).get(s)); ASSERT_EQUAL(s, std::string_view("x"));
      return true;
    }()));

    SUBTEST("scalar trio multibatch (small batch_size)", ([&]() {
      // Same input as the scalar trio test, but batch_size=6 forces the
      // stream through multiple stage1 invocations. Behavior must match
      // the single-batch case exactly.
      auto input = "\x1e" "1\n" "\x1e" "true\n" "\x1e" "\"x\"\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, 6, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();

      ASSERT_EQUAL(it.current_index(), size_t(1));
      ASSERT_EQUAL(it.source(), std::string_view("1"));
      int64_t i; ASSERT_SUCCESS((*it).get(i)); ASSERT_EQUAL(i, int64_t(1));
      ++it;

      ASSERT_EQUAL(it.current_index(), size_t(4));
      ASSERT_EQUAL(it.source(), std::string_view("true"));
      bool b{}; ASSERT_SUCCESS((*it).get(b)); ASSERT_TRUE(b);
      ++it;

      ASSERT_EQUAL(it.current_index(), size_t(10));
      ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
      std::string_view s; ASSERT_SUCCESS((*it).get(s)); ASSERT_EQUAL(s, std::string_view("x"));
      return true;
    }()));

    SUBTEST("large synthetic scalar stream multibatch", ([&]() {
      // Stress the windowing / multi-stage1 path with 256 RS-prefixed
      // scalar documents rotating through all four scalar types. A small
      // batch_size (64) forces ~25 stage1 invocations over ~1.5 KB of
      // input. Every document must parse and appear in order.
      constexpr size_t N = 256;
      std::string bytes;
      for (size_t i = 0; i < N; i++) {
        bytes += '\x1e';
        switch (i % 4) {
          case 0: bytes += std::to_string(i); break;
          case 1: bytes += (i & 1) ? "true" : "false"; break;
          case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
          case 3: bytes += "null"; break;
        }
        bytes += '\n';
      }
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 64, simdjson::stream_format::json_sequence).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        switch (i % 4) {
          case 0: {
            int64_t v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, int64_t(i));
            break;
          }
          case 1: {
            bool v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, bool(i & 1));
            break;
          }
          case 2: {
            std::string_view v; ASSERT_SUCCESS(doc.get(v));
            std::string want = "s" + std::to_string(i);
            ASSERT_EQUAL(v, std::string_view(want));
            break;
          }
          case 3: {
            simdjson::dom::element e; ASSERT_SUCCESS(doc.get(e));
            ASSERT_TRUE(e.is_null());
            break;
          }
        }
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    SUBTEST("large synthetic scalar stream multibatch with separator noise", ([&]() {
      // Same as the large stress test, but with extra RSes sprinkled every
      // 7 documents (empty leading record within a valid record sequence)
      // plus a trailing bare RS. Strict assertions: every document must
      // come through and parse cleanly, and the integer documents must
      // match the values we emitted.
      constexpr size_t N = 256;
      std::string bytes;
      for (size_t i = 0; i < N; i++) {
        bytes += '\x1e';
        if (i % 7 == 0 && i > 0) { bytes += '\x1e'; }  // empty record
        switch (i % 4) {
          case 0: bytes += std::to_string(i); break;
          case 1: bytes += (i & 1) ? "true" : "false"; break;
          case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
          case 3: bytes += "null"; break;
        }
        bytes += '\n';
      }
      bytes += '\x1e';  // trailing bare RS
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 64, simdjson::stream_format::json_sequence).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        switch (i % 4) {
          case 0: {
            int64_t v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, int64_t(i));
            break;
          }
          case 1: {
            bool v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, bool(i & 1));
            break;
          }
          case 2: {
            std::string_view v; ASSERT_SUCCESS(doc.get(v));
            std::string want = "s" + std::to_string(i);
            ASSERT_EQUAL(v, std::string_view(want));
            break;
          }
          case 3: {
            simdjson::dom::element e; ASSERT_SUCCESS(doc.get(e));
            ASSERT_TRUE(e.is_null());
            break;
          }
        }
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    // -------- RS spacing variants --------
    // The scanner classifies 0x1E as a scalar character, so the way an RS
    // is positioned relative to surrounding whitespace affects what
    // structural_indexes look like after stage1. These tests lock down
    // every reasonable arrangement: RS followed by the canonical LF, by
    // CRLF, by space, by tab, by mixed whitespace, with whitespace on both
    // sides, and tight (no surrounding whitespace at all) around
    // containers and strings.

    SUBTEST("RS followed by LF then value (canonical RFC)", ([&]() {
      // bytes: 1e 31 0a 1e 32 0a 1e 33 0a
      auto input = "\x1e" "1\n" "\x1e" "2\n" "\x1e" "3\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[3] = {1, 2, 3};
      size_t expected_idx[3] = {1, 4, 7};
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("RS followed by CRLF then value", ([&]() {
      // bytes: 1e 31 0d 0a 1e 32 0d 0a 1e 33 0d 0a
      auto input = "\x1e" "1\r\n" "\x1e" "2\r\n" "\x1e" "3\r\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[3] = {1, 2, 3};
      size_t expected_idx[3] = {1, 5, 9};
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("RS followed by single space then value", ([&]() {
      // bytes: 1e 20 31 0a 1e 20 32 0a 1e 20 33 0a
      auto input = "\x1e 1\n\x1e 2\n\x1e 3\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[3] = {1, 2, 3};
      size_t expected_idx[3] = {2, 6, 10};  // value sits after RS + 1 space
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("RS followed by tab then value", ([&]() {
      // bytes: 1e 09 31 0a 1e 09 32 0a 1e 09 33 0a
      auto input = "\x1e\t1\n\x1e\t2\n\x1e\t3\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[3] = {1, 2, 3};
      size_t expected_idx[3] = {2, 6, 10};  // value sits after RS + 1 tab
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("RS followed by mixed whitespace then value", ([&]() {
      // All four JSON whitespace characters between the RS and the value.
      // bytes: 1e 20 09 0d 0a 31 0a 1e 20 09 0d 0a 32 0a
      auto input = "\x1e \t\r\n1\n\x1e \t\r\n2\n"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[2] = {1, 2};
      size_t expected_idx[2] = {5, 12};  // value sits after RS + 4 ws chars
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 2);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(2));
      return true;
    }()));

    SUBTEST("RS with whitespace on both sides of scalar value", ([&]() {
      // Whitespace after the RS AND between the value and the next RS.
      // bytes: 1e 20 31 20 1e 20 32 20 1e 20 33
      auto input = "\x1e 1 \x1e 2 \x1e 3"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[3] = {1, 2, 3};
      size_t expected_idx[3] = {2, 6, 10};
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("tight RS around arrays (no whitespace anywhere)", ([&]() {
      // No whitespace between RS and value, between values, or anywhere
      // else. Arrays survive because their closing ']' is a structural
      // character that naturally terminates the preceding scalar run.
      // bytes: 1e 5b 31 2c 32 5d 1e 5b 33 2c 34 5d 1e 5b 35 2c 36 5d
      auto input = "\x1e[1,2]\x1e[3,4]\x1e[5,6]"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      size_t expected_idx[3] = {1, 7, 13};
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        simdjson::dom::array arr;
        ASSERT_SUCCESS(doc.get(arr));
        size_t n = 0;
        for (auto e : arr) { (void)e; n++; }
        ASSERT_EQUAL(n, size_t(2));
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("tight RS around string scalars (no whitespace anywhere)", ([&]() {
      // bytes: 1e 22 61 22 1e 22 62 22 1e 22 63 22
      auto input = "\x1e\"a\"\x1e\"b\"\x1e\"c\""_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      const char* expected[3] = {"a", "b", "c"};
      size_t expected_idx[3] = {1, 5, 9};
      size_t i = 0;
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 3);
        ASSERT_EQUAL(it.current_index(), expected_idx[i]);
        std::string_view sv;
        ASSERT_SUCCESS(doc.get(sv));
        ASSERT_EQUAL(sv, std::string_view(expected[i]));
        i++;
      }
      ASSERT_EQUAL(i, size_t(3));
      return true;
    }()));

    SUBTEST("tight RS around mixed containers (no whitespace anywhere)", ([&]() {
      // Object, array, object - all back-to-back with only RS between.
      // bytes: 1e 7b 22 61 22 3a 31 7d 1e 5b 31 2c 32 5d 1e 7b 22 62 22 3a 32 7d
      auto input = "\x1e{\"a\":1}\x1e[1,2]\x1e{\"b\":2}"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      auto it = stream.begin();

      // doc 0: {"a":1}
      ASSERT_EQUAL(it.current_index(), size_t(1));
      int64_t a; ASSERT_SUCCESS((*it)["a"].get(a)); ASSERT_EQUAL(a, int64_t(1));
      ++it;

      // doc 1: [1,2]
      ASSERT_EQUAL(it.current_index(), size_t(9));
      simdjson::dom::array arr;
      ASSERT_SUCCESS((*it).get(arr));
      size_t arr_count = 0;
      for (auto e : arr) { (void)e; arr_count++; }
      ASSERT_EQUAL(arr_count, size_t(2));
      ++it;

      // doc 2: {"b":2}
      ASSERT_EQUAL(it.current_index(), size_t(15));
      int64_t b; ASSERT_SUCCESS((*it)["b"].get(b)); ASSERT_EQUAL(b, int64_t(2));
      return true;
    }()));

    SUBTEST("tight RS multibatch: 100 objects no whitespace, bs=32", ([&]() {
      // 100 tight objects `\x1e{"i":0}\x1e{"i":1}...` with a small batch
      // size to stress the multibatch path. Objects are delimited by their
      // closing '}' so this case works without any whitespace.
      constexpr size_t N = 100;
      std::string bytes;
      for (size_t i = 0; i < N; i++) {
        bytes += '\x1e';
        bytes += "{\"i\":";
        bytes += std::to_string(i);
        bytes += "}";
      }
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 32, simdjson::stream_format::json_sequence).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        int64_t v;
        ASSERT_SUCCESS(doc["i"].get(v));
        ASSERT_EQUAL(v, int64_t(i));
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    SUBTEST("spaced RS multibatch: 100 int scalars RS+space+value+LF, bs=32", ([&]() {
      // 100 int scalars shaped as `\x1e <n>\n` each. Small batch size
      // (32) forces ~16 stage1 invocations over the ~500 byte input.
      constexpr size_t N = 100;
      std::string bytes;
      for (size_t i = 0; i < N; i++) {
        bytes += '\x1e';
        bytes += ' ';
        bytes += std::to_string(i);
        bytes += '\n';
      }
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 32, simdjson::stream_format::json_sequence).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        int64_t v;
        ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, int64_t(i));
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    SUBTEST("leading/trailing/repeated RS separators around scalars", ([&]() {
      // RFC 7464 says each JSON text is preceded by exactly one RS. The
      // sensible interpretation of degenerate inputs:
      //   - leading RS-RS (empty record before the first scalar): skip
      //   - repeated RSes between two scalars (empty intermediate record):
      //     skip the empty record, both real scalars are reported
      //   - trailing bare RS with no payload: skip, no phantom document
      // Layout: RS(0) RS(1) 1(2) LF(3) RS(4) RS(5) RS(6) 2(7) LF(8) RS(9)
      auto input = "\x1e\x1e" "1\n" "\x1e\x1e\x1e" "2\n" "\x1e"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::json_sequence).get(stream));
      int64_t expected[] = {1, 2};
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < 2);
        int64_t v; ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, expected[i]);
        i++;
      }
      ASSERT_EQUAL(i, size_t(2));
      return true;
    }()));

    TEST_SUCCEED();
  }

  bool comma_delimited_tests() {
    TEST_START();
    simdjson::dom::parser parser;
    simdjson::dom::document_stream stream;

    SUBTEST("basic comma-separated objects", ([&]() {
      auto input = R"({"a":1},{"b":2},{"c":3})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      const char* keys[] = {"a", "b", "c"};
      int64_t expected[] = {1, 2, 3};
      size_t count = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        int64_t value;
        ASSERT_SUCCESS(doc[keys[count]].get(value));
        ASSERT_EQUAL(value, expected[count]);
        count++;
      }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("comma-separated with whitespace", ([&]() {
      auto input = R"({"a":1} , {"b":2} , {"c":3})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("comma-separated arrays", ([&]() {
      auto input = R"([1,2],[3,4],[5,6])"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(3));
      return true;
    }()));

    SUBTEST("mixed document types", ([&]() {
      auto input = R"({"a":1},[2,3],"string",42,true,null)"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(6));
      return true;
    }()));

    SUBTEST("nested commas preserved", ([&]() {
      auto input = R"({"a":[1,2,3]},{"b":{"c":4,"d":5}})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();
      // First doc: {"a":[1,2,3]}
      simdjson::dom::array arr;
      ASSERT_SUCCESS((*it)["a"].get(arr));
      size_t arr_count = 0;
      for (auto elem : arr) { (void)elem; arr_count++; }
      ASSERT_EQUAL(arr_count, size_t(3));
      ++it;
      // Second doc: {"b":{"c":4,"d":5}}
      int64_t c_val, d_val;
      ASSERT_SUCCESS((*it)["b"]["c"].get(c_val));
      ASSERT_SUCCESS((*it)["b"]["d"].get(d_val));
      ASSERT_EQUAL(c_val, int64_t(4));
      ASSERT_EQUAL(d_val, int64_t(5));
      return true;
    }()));

    SUBTEST("single document no comma", ([&]() {
      auto input = R"({"a":1})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(1));
      return true;
    }()));

    SUBTEST("current_index points to JSON value", ([&]() {
      // Layout: {(0) "(1) a(2) "(3) :(4) 1(5) }(6) ,(7) {(8) ...
      auto input = R"({"a":1},{"b":2})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.current_index(), size_t(0));  // First { at position 0
      ++it;
      ASSERT_EQUAL(it.current_index(), size_t(8));  // Second { at position 8
      return true;
    }()));

    SUBTEST("source returns correct document slice", ([&]() {
      auto input = R"([1,2,3],{"a":1})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.source(), std::string_view("[1,2,3]"));
      ++it;
      ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
      return true;
    }()));

    SUBTEST("multibatch current_index and source", ([&]() {
      auto input = R"({"a":1}, {"b":2})"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, 10, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();
      ASSERT_EQUAL(it.current_index(), size_t(0));
      ASSERT_EQUAL(it.source(), std::string_view("{\"a\":1}"));
      ++it;
      ASSERT_EQUAL(it.current_index(), size_t(9));
      ASSERT_EQUAL(it.source(), std::string_view("{\"b\":2}"));
      return true;
    }()));

    SUBTEST("small batch reports capacity", ([&]() {
      std::string comma_input = "  [";
      for (size_t i = 0; i < 2000; i++) {
        comma_input += '1';
        comma_input += (i + 1 < 2000 ? ',' : ']');
      }
      auto input = simdjson::padded_string(comma_input);
      ASSERT_SUCCESS(parser.parse_many(input, 1024, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();
      auto doc = *it;
      ASSERT_ERROR(doc.error(), simdjson::CAPACITY);
      return true;
    }()));

    SUBTEST("scalar quintet 1,2,\"x\",true,null", ([&]() {
      // Five comma-delimited scalars: number, number, string, true, null.
      // Layout: 1(0) ,(1) 2(2) ,(3) "(4) x(5) "(6) ,(7) t(8) r(9) u(10) e(11) ,(12) n(13) u(14) l(15) l(16)
      //
      // Note on source(): the comma_delimited filter strips root-level
      // commas from structural_indexes but the bytes remain in the buffer.
      // source() for a scalar slices [current_index, next_doc_index) and
      // strips trailing whitespace; trailing delimiter is stripped for
      // consistency with json_sequence. The assertions below lock down
      // current_index() and parsed values strictly, and source() returns the
      // JSON value without trailing delimiter.
      auto input = R"(1,2,"x",true,null)"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      auto it = stream.begin();

      // doc 0: 1
      ASSERT_EQUAL(it.current_index(), size_t(0));
      int64_t i1; ASSERT_SUCCESS((*it).get(i1)); ASSERT_EQUAL(i1, int64_t(1));
      ASSERT_EQUAL(it.source(), std::string_view("1"));
      ++it;

      // doc 1: 2
      ASSERT_EQUAL(it.current_index(), size_t(2));
      int64_t i2; ASSERT_SUCCESS((*it).get(i2)); ASSERT_EQUAL(i2, int64_t(2));
      ASSERT_EQUAL(it.source(), std::string_view("2"));
      ++it;

      // doc 2: "x"
      ASSERT_EQUAL(it.current_index(), size_t(4));
      std::string_view sv; ASSERT_SUCCESS((*it).get(sv)); ASSERT_EQUAL(sv, std::string_view("x"));
      ASSERT_EQUAL(it.source(), std::string_view("\"x\""));
      ++it;

      // doc 3: true
      ASSERT_EQUAL(it.current_index(), size_t(8));
      bool b{}; ASSERT_SUCCESS((*it).get(b)); ASSERT_TRUE(b);
      ASSERT_EQUAL(it.source(), std::string_view("true"));
      ++it;

      // doc 4: null (last doc, no trailing comma in source either way)
      ASSERT_EQUAL(it.current_index(), size_t(13));
      simdjson::dom::element e; ASSERT_SUCCESS((*it).get(e));
      ASSERT_TRUE(e.is_null());
      ASSERT_EQUAL(it.source(), std::string_view("null"));
      return true;
    }()));

    SUBTEST("scalar quintet multibatch (small batch_size)", ([&]() {
      // Same input as the scalar quintet test, but batch_size=8 forces the
      // stream through multiple stage1 invocations. Behavior must match
      // the single-batch case exactly.
      auto input = R"(1,2,"x",true,null)"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, 8, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      const size_t expected_idx[5] = {0, 2, 4, 8, 13};
      for (auto it = stream.begin(); it != stream.end(); ++it) {
        auto doc = *it;
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(count < 5);
        ASSERT_EQUAL(it.current_index(), expected_idx[count]);
        count++;
      }
      ASSERT_EQUAL(count, size_t(5));
      return true;
    }()));

    SUBTEST("large synthetic scalar stream multibatch", ([&]() {
      // Stress the windowing / multi-stage1 path with 256 comma-delimited
      // scalar documents rotating through all four scalar types. A small
      // batch_size (64) forces ~20 stage1 invocations over ~1.3 KB of
      // input. Every document must parse and appear in order.
      constexpr size_t N = 256;
      std::string bytes;
      for (size_t i = 0; i < N; i++) {
        if (i > 0) { bytes += ','; }
        switch (i % 4) {
          case 0: bytes += std::to_string(i); break;
          case 1: bytes += (i & 1) ? "true" : "false"; break;
          case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
          case 3: bytes += "null"; break;
        }
      }
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 64, simdjson::stream_format::comma_delimited).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        switch (i % 4) {
          case 0: {
            int64_t v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, int64_t(i));
            break;
          }
          case 1: {
            bool v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, bool(i & 1));
            break;
          }
          case 2: {
            std::string_view v; ASSERT_SUCCESS(doc.get(v));
            std::string want = "s" + std::to_string(i);
            ASSERT_EQUAL(v, std::string_view(want));
            break;
          }
          case 3: {
            simdjson::dom::element e; ASSERT_SUCCESS(doc.get(e));
            ASSERT_TRUE(e.is_null());
            break;
          }
        }
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    SUBTEST("large synthetic scalar stream multibatch with separator noise", ([&]() {
      // Same as the large stress test, but with a leading comma, extra
      // commas sprinkled every 5 documents, and two trailing commas. The
      // comma_delimited filter is lenient about degenerate separators,
      // so this test is STRICT: exactly N documents must still come out,
      // in order, with correct values.
      constexpr size_t N = 256;
      std::string bytes;
      bytes += ',';  // leading comma
      for (size_t i = 0; i < N; i++) {
        if (i > 0) { bytes += ','; }
        if (i % 5 == 0 && i > 0) { bytes += ','; }  // extra comma
        switch (i % 4) {
          case 0: bytes += std::to_string(i); break;
          case 1: bytes += (i & 1) ? "true" : "false"; break;
          case 2: bytes += "\"s" + std::to_string(i) + "\""; break;
          case 3: bytes += "null"; break;
        }
      }
      bytes += ",,";  // trailing commas
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 64, simdjson::stream_format::comma_delimited).get(stream));
      size_t i = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(i < N);
        switch (i % 4) {
          case 0: {
            int64_t v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, int64_t(i));
            break;
          }
          case 1: {
            bool v; ASSERT_SUCCESS(doc.get(v));
            ASSERT_EQUAL(v, bool(i & 1));
            break;
          }
          case 2: {
            std::string_view v; ASSERT_SUCCESS(doc.get(v));
            std::string want = "s" + std::to_string(i);
            ASSERT_EQUAL(v, std::string_view(want));
            break;
          }
          case 3: {
            simdjson::dom::element e; ASSERT_SUCCESS(doc.get(e));
            ASSERT_TRUE(e.is_null());
            break;
          }
        }
        i++;
      }
      ASSERT_EQUAL(i, N);
      return true;
    }()));

    SUBTEST("leading/trailing/repeated commas around scalars", ([&]() {
      // Comma-delimited filter is expected to be lenient: leading commas,
      // trailing commas, and repeated commas between scalars all collapse
      // to single separators. The three real scalars (1, 2, "x") must be
      // reported with no phantom documents.
      // Layout: ,(0) 1(1) ,(2) ,(3) 2(4) ,(5) ,(6) "(7) x(8) "(9) ,(10) ,(11)
      auto input = R"(,1,,2,,"x",,)"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited).get(stream));
      size_t count = 0;
      int64_t i1 = -1, i2 = -1;
      std::string_view sx;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        if (count == 0) { ASSERT_SUCCESS(doc.get(i1)); }
        else if (count == 1) { ASSERT_SUCCESS(doc.get(i2)); }
        else if (count == 2) { ASSERT_SUCCESS(doc.get(sx)); }
        count++;
      }
      ASSERT_EQUAL(count, size_t(3));
      ASSERT_EQUAL(i1, int64_t(1));
      ASSERT_EQUAL(i2, int64_t(2));
      ASSERT_EQUAL(sx, std::string_view("x"));
      return true;
    }()));

    // -------- stream_format::comma_delimited_array --------
    // The comma_delimited_array mode strips the outer [ ] (plus any
    // surrounding JSON whitespace) and then delegates to comma_delimited.

    SUBTEST("comma_delimited_array basic [1,2,3]", ([&]() {
      auto input = R"([1,2,3])"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      int64_t got[3] = {0, 0, 0};
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(count < 3);
        ASSERT_SUCCESS(doc.get(got[count]));
        count++;
      }
      ASSERT_EQUAL(count, size_t(3));
      ASSERT_EQUAL(got[0], int64_t(1));
      ASSERT_EQUAL(got[1], int64_t(2));
      ASSERT_EQUAL(got[2], int64_t(3));
      return true;
    }()));

    SUBTEST("comma_delimited_array mixed scalar types", ([&]() {
      auto input = R"([1, "a", true, null, {"x":1}])"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      for (auto doc : stream) { ASSERT_SUCCESS(doc.error()); count++; }
      ASSERT_EQUAL(count, size_t(5));
      return true;
    }()));

    SUBTEST("comma_delimited_array empty []", ([&]() {
      auto input = R"([])"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      for (auto doc : stream) { (void)doc; count++; }
      ASSERT_EQUAL(count, size_t(0));
      return true;
    }()));

    SUBTEST("comma_delimited_array empty with interior whitespace [ ]", ([&]() {
      // [ ] should also yield zero documents.
      auto input = "[  \t\n]"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      for (auto doc : stream) { (void)doc; count++; }
      ASSERT_EQUAL(count, size_t(0));
      return true;
    }()));

    SUBTEST("comma_delimited_array surrounding whitespace stripped", ([&]() {
      // All four JSON whitespace characters before the '[' and after the ']'.
      auto input = " \t\n\r[ 1, 2, 3 ] \t\n\r"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      int64_t sum = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        int64_t v;
        ASSERT_SUCCESS(doc.get(v));
        sum += v;
        count++;
      }
      ASSERT_EQUAL(count, size_t(3));
      ASSERT_EQUAL(sum, int64_t(6));
      return true;
    }()));

    SUBTEST("comma_delimited_array nested commas preserved", ([&]() {
      auto input = R"([{"a":1,"b":2},{"c":[3,4,5]}])"_padded;
      ASSERT_SUCCESS(parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream));
      auto it = stream.begin();
      // doc 0: {"a":1,"b":2}
      int64_t a, b;
      ASSERT_SUCCESS((*it)["a"].get(a));
      ASSERT_SUCCESS((*it)["b"].get(b));
      ASSERT_EQUAL(a, int64_t(1));
      ASSERT_EQUAL(b, int64_t(2));
      ++it;
      // doc 1: {"c":[3,4,5]}
      simdjson::dom::array arr;
      ASSERT_SUCCESS((*it)["c"].get(arr));
      size_t arr_count = 0;
      for (auto e : arr) { (void)e; arr_count++; }
      ASSERT_EQUAL(arr_count, size_t(3));
      return true;
    }()));

    SUBTEST("comma_delimited_array missing leading bracket is TAPE_ERROR", ([&]() {
      auto input = R"(1,2,3)"_padded;
      auto err = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
      ASSERT_ERROR(err, simdjson::TAPE_ERROR);
      return true;
    }()));

    SUBTEST("comma_delimited_array missing trailing bracket is TAPE_ERROR", ([&]() {
      auto input = R"([1,2,3)"_padded;
      auto err = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
      ASSERT_ERROR(err, simdjson::TAPE_ERROR);
      return true;
    }()));

    SUBTEST("comma_delimited_array empty input is TAPE_ERROR", ([&]() {
      auto input = ""_padded;
      auto err = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
      ASSERT_ERROR(err, simdjson::TAPE_ERROR);
      return true;
    }()));

    SUBTEST("comma_delimited_array whitespace-only input is TAPE_ERROR", ([&]() {
      auto input = "   \t\n"_padded;
      auto err = parser.parse_many(input, simdjson::dom::DEFAULT_BATCH_SIZE, simdjson::stream_format::comma_delimited_array).get(stream);
      ASSERT_ERROR(err, simdjson::TAPE_ERROR);
      return true;
    }()));

    SUBTEST("comma_delimited_array multibatch large array", ([&]() {
      // Generate [0,1,2,...,N-1] and iterate with a small batch_size so the
      // stream runs stage1 across many windows. All N values must come out
      // in order with no phantom documents.
      constexpr size_t N = 256;
      std::string bytes = "[";
      for (size_t i = 0; i < N; i++) {
        if (i > 0) { bytes += ','; }
        bytes += std::to_string(i);
      }
      bytes += "]";
      auto input = simdjson::padded_string(bytes);
      ASSERT_SUCCESS(parser.parse_many(input, 64, simdjson::stream_format::comma_delimited_array).get(stream));
      size_t count = 0;
      for (auto doc : stream) {
        ASSERT_SUCCESS(doc.error());
        ASSERT_TRUE(count < N);
        int64_t v;
        ASSERT_SUCCESS(doc.get(v));
        ASSERT_EQUAL(v, int64_t(count));
        count++;
      }
      ASSERT_EQUAL(count, N);
      return true;
    }()));

    TEST_SUCCEED();
  }

  bool run() {
    return json_sequence_tests() &&
           comma_delimited_tests() &&
           issue2181() &&
           issue2170() &&
          issue_non_ascii_separator_source() &&
           skipbom() &&
           fuzzaccess() &&
           baby_fuzzer() &&
           issue1649() &&
           adversarial_single_document_array() &&
           adversarial_single_document() &&
           unquoted_key() &&
           stress_data_race() &&
           stress_data_race_with_error() &&
           test_leading_spaces() &&
           test_crazy_leading_spaces() &&
           simple_example() &&
           truncated_window() &&
           truncated_window_unclosed_string_in_object() &&
           truncated_window_unclosed_string() &&
           issue1307() &&
           issue1308() &&
           issue1309() &&
           issue1310() &&
           issue1311() &&
           test_naked_iterators() &&
           test_current_index()  &&
           single_document() &&
#if SIMDJSON_EXCEPTIONS
           issue1133() &&
           single_document_exceptions() &&
#endif
           window_too_small_issue1370() &&
           small_window() &&
           large_window() &&
           json_issue467() &&
           document_stream_test() &&
           document_stream_utf8_test();
  }

} // namespace document_stream_tests


int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::get_available_implementations()[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      if(!impl->supported_by_runtime_system()) {
        fprintf(stderr, "The selected implementation does not match your current CPU: -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::get_active_implementation() = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::get_active_implementation()->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  std::cout << "Running tests against this implementation: " << simdjson::get_active_implementation()->name();
  std::cout << " (" << simdjson::get_active_implementation()->description() << ")" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running document_stream tests." << std::endl;
  if (document_stream_tests::run()) {
    std::cout << "document_stream tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
