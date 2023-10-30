#include <string>
#include <vector>
#include <cctype>
#include <unistd.h>
#include <random>

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
    const auto json=simdjson::padded_string(std::string("\xd7"));
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

  bool run() {
    return skipbom() &&
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
}



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
