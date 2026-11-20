#include "simdjson.h"
#include "test_ondemand.h"

#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace simdjson;

namespace validate_tests {

  error_code validate_text(ondemand::parser &parser, const std::string &text) {
    padded_string json(text);
    ondemand::document doc;
    SIMDJSON_TRY(parser.iterate(json).get(doc));
    return doc.validate();
  }

  bool valid_documents() {
    TEST_START();
    const char *documents[] = {
      R"({"a":[1,2,{"b":"x"}],"c":true,"d":false,"e":null,"f":-1.5e3})",
      R"([])", R"({})", R"([[[[]]]])", R"([{},[],{"a":{}}])",
      " \n\t[ 1 , 2 ]\r\n ",
      R"({"key with \"escapes\" \\ \/ \b \f \n \r \t":"\u00e9\ud83d\ude00"})",
      R"(["caf\u00e9","\u0000","plain"])",
      R"([0,-0,1.5,-2.5E-3,1e308,123456789012345678901234567890])",
      R"("a root string")", "42", "-1.25", "true", "false", "null", "  7  ",
    };
    ondemand::parser parser;
    for (const char *text : documents) {
      std::cout << "  " << text << std::endl;
      ASSERT_SUCCESS(validate_text(parser, text));
    }
    TEST_SUCCEED();
  }

  bool invalid_documents() {
    TEST_START();
    struct invalid { const char *text; error_code error; };
    const invalid documents[] = {
      {R"({"a":1,})", TAPE_ERROR},
      {R"([1,2,])", TAPE_ERROR},
      {R"({"a" 1})", TAPE_ERROR},
      {R"({"a":1 "b":2})", TAPE_ERROR},
      {R"([1 2])", TAPE_ERROR},
      {R"({"a":[1,2})", TAPE_ERROR},
      {R"({"a":})", TAPE_ERROR},
      {R"([,1])", TAPE_ERROR},
      {R"({,})", TAPE_ERROR},
      {R"({"a":1:2})", TAPE_ERROR},
      {R"({1:2})", TAPE_ERROR},
      {R"([:])", TAPE_ERROR},
      {R"([1,2]])", TRAILING_CONTENT},
      {R"([1] 2)", TRAILING_CONTENT},
      {R"({} {})", TRAILING_CONTENT},
      {R"("a" "b")", TRAILING_CONTENT},
      {"4 2", TRAILING_CONTENT},
      {R"([tru])", T_ATOM_ERROR},
      {R"([fals])", F_ATOM_ERROR},
      {R"([nul])", N_ATOM_ERROR},
      {R"({"a":truex})", T_ATOM_ERROR},
      {"tru", T_ATOM_ERROR},
      {"nul", N_ATOM_ERROR},
      {R"([01])", NUMBER_ERROR},
      {R"([1.])", NUMBER_ERROR},
      {R"([-])", NUMBER_ERROR},
      {R"({"a":1e})", NUMBER_ERROR},
      {"-", NUMBER_ERROR},
      {R"(["\x"])", STRING_ERROR},
      {R"({"\q":1})", STRING_ERROR},
      {R"(["\ud800"])", STRING_ERROR},
      {R"(["\u12"])", STRING_ERROR},
      {"[", INCOMPLETE_ARRAY_OR_OBJECT},
      {R"({"a")", INCOMPLETE_ARRAY_OR_OBJECT},
      {R"({"a":1)", INCOMPLETE_ARRAY_OR_OBJECT},
      {R"([1,[2,3])", INCOMPLETE_ARRAY_OR_OBJECT},
    };
    ondemand::parser parser;
    for (const invalid &document : documents) {
      std::cout << "  " << document.text << std::endl;
      ASSERT_ERROR(validate_text(parser, document.text), document.error);
    }
    TEST_SUCCEED();
  }

  bool readable_after_validate() {
    TEST_START();
    auto json = R"({"a":[1,2,3],"b":{"c":"text"}})"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ASSERT_SUCCESS(doc.validate());
    ASSERT_SUCCESS(doc.validate());
    std::string_view c;
    ASSERT_SUCCESS(doc["b"]["c"].get(c));
    ASSERT_EQUAL(c, "text");
    // A partly read document is rewound first.
    ASSERT_SUCCESS(doc.validate());
    int64_t sum = 0;
    ondemand::array a;
    ASSERT_SUCCESS(doc["a"].get_array().get(a));
    for (auto element : a) {
      int64_t v;
      ASSERT_SUCCESS(element.get(v));
      sum += v;
    }
    ASSERT_EQUAL(sum, 6);
    TEST_SUCCEED();
  }

  bool readable_root_scalar_after_validate() {
    TEST_START();
    auto json = "1234"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ASSERT_SUCCESS(doc.validate());
    int64_t v;
    ASSERT_SUCCESS(doc.get(v));
    ASSERT_EQUAL(v, 1234);
    TEST_SUCCEED();
  }

  bool result_forwarding() {
    TEST_START();
    auto good = R"([1,{"a":"b"}])"_padded;
    auto bad = R"([1,{"a" "b"}])"_padded;
    ondemand::parser parser;
    simdjson_result<ondemand::document> doc = parser.iterate(good);
    ASSERT_SUCCESS(doc.validate());
    doc = parser.iterate(bad);
    ASSERT_ERROR(doc.validate(), TAPE_ERROR);
    TEST_SUCCEED();
  }

  // The bytes after a document, inside its padding, may be anything.
  bool garbage_padding() {
    TEST_START();
    struct checked { const char *text; bool valid; };
    const checked documents[] = {
      {R"([1,2])", true}, {R"({"a":"b"})", true}, {R"(["x"])", true}, {"12", true}, {"true", true},
      {"[1,2", false}, {R"({"a":"b")", false}, {R"(["x")", false}, {"[12", false}, {"[tru", false},
      {R"({"a")", false}, {"[", false},
    };
    const char fills[] = {'"', '\\', '1', ']', '}', ',', 'e', ' ', 't', '\0'};
    ondemand::parser parser;
    for (const checked &document : documents) {
      for (char fill : fills) {
        const size_t len = std::strlen(document.text);
        std::vector<char> buffer(len + SIMDJSON_PADDING, fill);
        std::memcpy(buffer.data(), document.text, len);
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(buffer.data(), len, buffer.size()).get(doc));
        error_code error = doc.validate();
        if (bool(error) == document.valid) {
          std::cerr << "  " << document.text << " with padding '" << fill << "': " << error << std::endl;
          return false;
        }
      }
    }
    TEST_SUCCEED();
  }

  // Alternating arrays and objects, depth levels deep.
  std::string nested(size_t depth, const char *leaf = "1") {
    std::string text, close;
    for (size_t i = 0; i < depth; i++) {
      if (i % 2 == 0) { text.append("["); close.push_back(']'); }
      else { text.append(R"({"a":)"); close.push_back('}'); }
    }
    text.append(leaf);
    text.append(close.rbegin(), close.rend());
    return text;
  }

  bool nesting_limit_matches_dom() {
    TEST_START();
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *leaf : {"1", "[]", "{}"}) {
      for (size_t depth = DEFAULT_MAX_DEPTH - 4; depth <= DEFAULT_MAX_DEPTH + 4; depth++) {
        const std::string text = nested(depth, leaf);
        padded_string copy(text);
        const error_code dom_error = dom_parser.parse(copy).error();
        ASSERT_EQUAL(validate_text(parser, text), dom_error);
      }
    }
    TEST_SUCCEED();
  }

  // An empty container does not count toward the depth, as in DOM.
  bool empty_containers_at_depth_one() {
    TEST_START();
    ondemand::parser parser;
    ASSERT_SUCCESS(parser.allocate(1000, 1));
    dom::parser dom_parser;
    ASSERT_SUCCESS(dom_parser.allocate(1000, 1));
    for (const char *text : {"[]", "{}", " [ ] ", "[1]", R"({"a":1})", "[[]]"}) {
      std::cout << "  " << text << std::endl;
      padded_string copy{std::string(text)};
      ASSERT_EQUAL(validate_text(parser, text), dom_parser.parse(copy).error());
    }
    ASSERT_SUCCESS(validate_text(parser, "[]"));
    ASSERT_SUCCESS(validate_text(parser, "{}"));
    TEST_SUCCEED();
  }

#if SIMDJSON_ENABLE_NAN_INF
  // With NaN and infinity enabled, validate() returns the error code DOM returns.
  bool nan_inf_matches_dom() {
    TEST_START();
    const char *documents[] = {
      "nan", "nAn", "NaN", "Infinity", "inf", "-nan", "-Infinity", "nul", "Nope", "ix",
      "[nan]", "[nAn,NaN,Infinity,inf,-nan,-Infinity]", "[nul]", "[Nope]", "[ix]",
      "[1,]", "[1,2,]", R"({"a":nan,"b":Infinity})", R"({"a":nul})",
    };
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *text : documents) {
      std::cout << "  " << text << std::endl;
      padded_string copy{std::string(text)};
      ASSERT_EQUAL(validate_text(parser, text), dom_parser.parse(copy).error());
    }
    TEST_SUCCEED();
  }
#endif

  bool deep_nesting_with_larger_max_depth() {
    TEST_START();
    ondemand::parser parser;
    ASSERT_SUCCESS(parser.allocate(1000000, 5000));
    // Past the depth kept on the stack: the validator moves to the heap.
    ASSERT_SUCCESS(validate_text(parser, nested(3000)));
    ASSERT_ERROR(validate_text(parser, nested(3000) + "]"), TRAILING_CONTENT);
    ASSERT_ERROR(validate_text(parser, nested(5001)), DEPTH_ERROR);
    TEST_SUCCEED();
  }

  bool stream_documents() {
    TEST_START();
    auto json = R"({"a":[1,2]} [3,"x"] 4 "five" {"b":null})"_padded;
    ondemand::parser parser;
    ondemand::document_stream stream;
    ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
    size_t count = 0;
    for (auto result : stream) {
      ondemand::document_reference doc_ref;
      ASSERT_SUCCESS(std::move(result).get(doc_ref));
      ondemand::document &doc = doc_ref;
      ASSERT_SUCCESS(doc.validate());
      count++;
    }
    ASSERT_EQUAL(count, 5);
    TEST_SUCCEED();
  }

  // Random edits of valid documents: validate() and DOM parsing must agree on validity.
  // The seeds avoid integers beyond 64 bits, which DOM rejects and validate() accepts.
  bool agrees_with_dom() {
    TEST_START();
    const char *seeds[] = {
      R"({"a":[1,2,{"b":"x\n\u00e9"}],"c":true,"d":null,"e":-1.5e3,"f":{}})",
      R"([[1,[2,[3,[]]]],{"k":"v","w":[true,false,null]},"s\"t",0.5,-0,1e10])",
      R"({"key":[{"k1":"v1"},{"k1":"v1"},{"k1":"v1"}]})",
      R"({"A\n":"\\","":[],"x":{"y":{"z":[[[[{}]]]]}}})",
      R"("a string")", "123.25", "true", "false", "null", "[]", "{}",
    };
    const char alphabet[] = "{}[],:\"\\ 0123456789-+.eEtrufalsn";
    std::mt19937 gen(1234);
    ondemand::parser parser;
    dom::parser dom_parser;
    size_t valid = 0;
    for (size_t i = 0; i < 20000; i++) {
      std::string text = seeds[gen() % (sizeof(seeds) / sizeof(seeds[0]))];
      const size_t edits = gen() % 4;
      for (size_t e = 0; e < edits && !text.empty(); e++) {
        const size_t at = gen() % text.size();
        const char c = alphabet[gen() % (sizeof(alphabet) - 1)];
        switch (gen() % 4) {
          case 0: text[at] = c; break;
          case 1: text.erase(at, 1); break;
          case 2: text.insert(at, 1, c); break;
          default: text.resize(at); break;
        }
      }
      padded_string copy(text);
      const error_code dom_error = dom_parser.parse(copy).error();
      error_code error;
      ondemand::document doc;
      error = parser.iterate(copy).get(doc);
      if (!error) { error = doc.validate(); }
      if (bool(error) != bool(dom_error)) {
        std::cerr << "  [" << text << "] validate: " << error << " dom: " << dom_error << std::endl;
        return false;
      }
      if (!error) { valid++; }
    }
    std::cout << "  " << valid << " valid documents out of 20000" << std::endl;
    TEST_SUCCEED();
  }

  bool run() {
    return valid_documents() &&
           invalid_documents() &&
           readable_after_validate() &&
           readable_root_scalar_after_validate() &&
           result_forwarding() &&
           garbage_padding() &&
           nesting_limit_matches_dom() &&
           empty_containers_at_depth_one() &&
#if SIMDJSON_ENABLE_NAN_INF
           nan_inf_matches_dom() &&
#endif
           deep_nesting_with_larger_max_depth() &&
           stream_documents() &&
           agrees_with_dom() &&
           true;
  }

} // namespace validate_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, validate_tests::run);
}
