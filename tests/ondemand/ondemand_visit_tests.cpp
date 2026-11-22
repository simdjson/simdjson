#include "simdjson.h"
#include "test_ondemand.h"

#include <algorithm>
#include <cstring>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace simdjson;

namespace visit_tests {

  // Writes the events of a document as text: two documents have the same text if and only if
  // they have the same values, with doubles compared bit for bit.
  struct recorder {
    std::string out{};
    void text(char kind, std::string_view value) {
      out.push_back(kind);
      out.append(std::to_string(value.size()));
      out.push_back(':');
      out.append(value.data(), value.size());
    }
    void number(char kind, uint64_t bits) {
      out.push_back(kind);
      out.append(std::to_string(bits));
      out.push_back(' ');
    }
    void signed_number(int64_t value) {
      out.push_back('i');
      out.append(std::to_string(value));
      out.push_back(' ');
    }
    void real(double value) {
      uint64_t bits;
      std::memcpy(&bits, &value, sizeof(bits));
      number('d', bits);
    }
  };

  struct recording_visitor {
    recorder rec{};
    error_code on_object_start() { rec.out.push_back('{'); return SUCCESS; }
    error_code on_object_end(uint32_t count) { rec.number('}', count); return SUCCESS; }
    error_code on_array_start() { rec.out.push_back('['); return SUCCESS; }
    error_code on_array_end(uint32_t count) { rec.number(']', count); return SUCCESS; }
    error_code on_key(std::string_view key) { rec.text('k', key); return SUCCESS; }
    error_code on_string(std::string_view value) { rec.text('s', value); return SUCCESS; }
    error_code on_int64(int64_t value, const char *) { rec.signed_number(value); return SUCCESS; }
    error_code on_uint64(uint64_t value, const char *) { rec.number('u', value); return SUCCESS; }
    error_code on_double(double value, const char *) { rec.real(value); return SUCCESS; }
    error_code on_bool(bool value) { rec.out.append(value ? "t" : "f"); return SUCCESS; }
    error_code on_null() { rec.out.push_back('n'); return SUCCESS; }
  };

  // Writes the events of a DOM element as the visitor does; false on an error.
  bool record_dom(dom::element element, recorder &rec) {
    switch (element.type()) {
      case dom::element_type::OBJECT: {
        dom::object object;
        if (element.get(object)) { return false; }
        rec.out.push_back('{');
        for (dom::key_value_pair field : object) {
          rec.text('k', field.key);
          if (!record_dom(field.value, rec)) { return false; }
        }
        rec.number('}', object.size());
        return true;
      }
      case dom::element_type::ARRAY: {
        dom::array array;
        if (element.get(array)) { return false; }
        rec.out.push_back('[');
        for (dom::element child : array) {
          if (!record_dom(child, rec)) { return false; }
        }
        rec.number(']', array.size());
        return true;
      }
      case dom::element_type::STRING: {
        std::string_view value;
        if (element.get(value)) { return false; }
        rec.text('s', value);
        return true;
      }
      case dom::element_type::INT64: {
        int64_t value;
        if (element.get(value)) { return false; }
        rec.signed_number(value);
        return true;
      }
      case dom::element_type::UINT64: {
        uint64_t value;
        if (element.get(value)) { return false; }
        rec.number('u', value);
        return true;
      }
      case dom::element_type::DOUBLE: {
        double value;
        if (element.get(value)) { return false; }
        rec.real(value);
        return true;
      }
      case dom::element_type::BOOL: {
        bool value;
        if (element.get(value)) { return false; }
        rec.out.append(value ? "t" : "f");
        return true;
      }
      case dom::element_type::NULL_VALUE: rec.out.push_back('n'); return true;
      default: return false;
    }
  }

  error_code visit_text(ondemand::parser &parser, const std::string &text, std::string *events = nullptr) {
    padded_string json(text);
    ondemand::document doc;
    SIMDJSON_TRY(parser.iterate(json).get(doc));
    recording_visitor visitor;
    const error_code error = doc.visit(visitor);
    if (events) { *events = visitor.rec.out; }
    return error;
  }

  error_code parser_visit_text(ondemand::parser &parser, const std::string &text, std::string *events = nullptr) {
    padded_string json(text);
    recording_visitor visitor;
    const error_code error = parser.visit(json, visitor);
    if (events) { *events = visitor.rec.out; }
    return error;
  }

  // Both walks give the same error code. On success they give the same events. On an error, the
  // one-pass walk of parser::visit() may go further than document::visit(), never elsewhere.
  bool same_visit(error_code error, const std::string &events, error_code expected_error, const std::string &expected) {
    if (error != expected_error) { return false; }
    if (!error) { return events == expected; }
    return events.compare(0, expected.size(), expected) == 0;
  }

  // The text that DOM parses as the visit does: with SIMDJSON_ALLOW_TRAILING_COMMAS, the
  // commas before a closing bracket or brace are removed.
  std::string dom_text(const std::string &text) {
#if SIMDJSON_ALLOW_TRAILING_COMMAS
    std::string out;
    bool in_string = false;
    char previous = '[';
    for (size_t i = 0; i < text.size(); i++) {
      const char c = text[i];
      if (in_string) {
        out += c;
        if (c == '\\' && i + 1 < text.size()) {
          out += text[++i];
        } else if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == ',' && previous != '[' && previous != '{' && previous != ',' && previous != ':') {
        size_t j = i + 1;
        while (j < text.size() && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r')) { j++; }
        if (j < text.size() && (text[j] == ']' || text[j] == '}')) {
          previous = c;
          continue;
        }
      }
      if (c == '"') { in_string = true; }
      if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { previous = c; }
      out += c;
    }
    return out;
#else
    return text;
#endif
  }

  // Visits text and parses it with DOM: both fail, or both succeed with the same values.
  // parser::visit() gives the same error code and values as document::visit().
  bool same_as_dom(ondemand::parser &parser, dom::parser &dom_parser, const std::string &text) {
    padded_string copy(dom_text(text));
    dom::element element;
    const error_code dom_error = dom_parser.parse(copy).get(element);
    std::string events;
    const error_code error = visit_text(parser, text, &events);
    if (bool(error) != bool(dom_error)) {
      std::cerr << "  [" << text.substr(0, 200) << "] visit: " << error << " dom: " << dom_error << std::endl;
      return false;
    }
    std::string parser_events;
    const error_code parser_error = parser_visit_text(parser, text, &parser_events);
    if (!same_visit(parser_error, parser_events, error, events)) {
      std::cerr << "  [" << text.substr(0, 200) << "] document::visit: " << error << " parser::visit: " << parser_error << std::endl;
      return false;
    }
    if (!error) {
      recorder rec;
      if (!record_dom(element, rec) || rec.out != events) {
        std::cerr << "  [" << text.substr(0, 200) << "] visit and DOM differ" << std::endl;
        return false;
      }
    }
    return true;
  }

  bool valid_documents() {
    TEST_START();
    const char *documents[] = {
      R"({"a":[1,2,{"b":"x"}],"c":true,"d":false,"e":null,"f":-1.5e3})",
      R"([])", R"({})", R"([[[[]]]])", R"([{},[],{"a":{}}])", R"({"a":[],"b":{},"c":[{}]})",
      " \n\t[ 1 , 2 ]\r\n ",
      R"({"key with \"escapes\" \\ \/ \b \f \n \r \t":"\u00e9\ud83d\ude00"})",
      R"(["caf\u00e9","\u0000","plain",""])",
      R"(["a string longer than thirty-two bytes, then an escape \n at the end"])",
      R"([0,-0,1.5,-2.5E-3,1e308,-9223372036854775808,9223372036854775807,9223372036854775808,18446744073709551615])",
      R"("a root string")", R"("a root string with an \"escape\"")", "42", "-1.25", "-0", "18446744073709551615",
      "-9223372036854775808", "true", "false", "null", "  7  ", "1e400", "1e-400",
      // around the short-integer path: 18 and 19 digits, zeros, signs, floats after digits
      "[123456789012345678,-123456789012345678,1234567890123456789,-1234567890123456789]",
      "[999999999999999999,1000000000000000000,-999999999999999999,-1000000000000000000]",
      "[0,-0,0.5,-0.5,0.0,-0.0,0e0,-0e-0,10,1e5,1E-5,-1.5e+3,12345678901234567890.5]",
      "[1 , 2\n,\t3\r]", R"({"a":1,"b":-2,"c":3.25,"d":-4e1})", "[18446744073709551615,9223372036854775807]",
    };
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *text : documents) {
      std::cout << "  " << text << std::endl;
      ASSERT_TRUE(same_as_dom(parser, dom_parser, text));
    }
    TEST_SUCCEED();
  }

  bool example_files() {
    TEST_START();
    const char *files[] = {TWITTER_JSON, CANADA_JSON, MESH_JSON, APACHE_JSON, GSOC_JSON, TWITTER_TIMELINE_JSON, REPEAT_JSON};
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *file : files) {
      std::cout << "  " << file << std::endl;
      padded_string json;
      ASSERT_SUCCESS(padded_string::load(file).get(json));
      ASSERT_TRUE(same_as_dom(parser, dom_parser, std::string(json.data(), json.size())));
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
      {"[123456789012345678901234567890]", BIGINT_ERROR},
      {"[1.5x]", NUMBER_ERROR},
      {"[12a]", NUMBER_ERROR},
      {"[-a]", NUMBER_ERROR},
      {"[01.5]", NUMBER_ERROR},
      {"[-01]", NUMBER_ERROR},
      {"[1.e5]", NUMBER_ERROR},
      {"[1e+]", NUMBER_ERROR},
      {"[00]", NUMBER_ERROR},
      {"-123456789012345678901234567890", BIGINT_ERROR},
      {"+1", TAPE_ERROR},
      {".5", TAPE_ERROR},
      {"[+1]", TAPE_ERROR},
      {"{\"a\":.5}", TAPE_ERROR},
#if !SIMDJSON_ENABLE_NAN_INF
      {"Infinity", TAPE_ERROR},
      {"NaN", TAPE_ERROR},
      {"inf", TAPE_ERROR},
      {"[NaN]", TAPE_ERROR},
#endif
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
      if (dom_text(document.text) != document.text) { continue; }
      ASSERT_ERROR(visit_text(parser, document.text), document.error);
      ASSERT_ERROR(parser_visit_text(parser, document.text), document.error);
    }
    TEST_SUCCEED();
  }

  // Keeps the strings as the visitor receives them.
  struct string_visitor : recording_visitor {
    std::vector<std::string_view> strings{};
    error_code on_key(std::string_view key) { strings.push_back(key); return SUCCESS; }
    error_code on_string(std::string_view value) { strings.push_back(value); return SUCCESS; }
  };

  bool strings_without_escapes_are_views() {
    TEST_START();
    auto json = R"({"plain":"value","esc\naped":"tab\there","long":"a string longer than thirty-two bytes, without escapes"})"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    string_visitor visitor;
    ASSERT_SUCCESS(doc.visit(visitor));
    ASSERT_EQUAL(visitor.strings.size(), 6);
    const std::string_view expected[] = {"plain", "value", "esc\naped", "tab\there", "long",
                                         "a string longer than thirty-two bytes, without escapes"};
    const bool in_document[] = {true, true, false, false, true, true};
    for (size_t i = 0; i < 6; i++) {
      ASSERT_EQUAL(visitor.strings[i], expected[i]);
      const bool inside = visitor.strings[i].data() >= json.data() && visitor.strings[i].data() < json.data() + json.size();
      ASSERT_EQUAL(inside, in_document[i]);
    }
    TEST_SUCCEED();
  }

  bool root_strings_without_escapes_are_views() {
    TEST_START();
    ondemand::parser parser;
    struct expected { const char *text; const char *value; bool in_document; };
    for (const expected &e : {expected{R"("plain")", "plain", true}, expected{R"("es\ncaped")", "es\ncaped", false},
                              expected{R"( "a root string longer than thirty-two bytes" )", "a root string longer than thirty-two bytes", true}}) {
      std::cout << "  " << e.text << std::endl;
      padded_string json{std::string(e.text)};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      string_visitor visitor;
      ASSERT_SUCCESS(doc.visit(visitor));
      ASSERT_EQUAL(visitor.strings.size(), 1);
      ASSERT_EQUAL(visitor.strings[0], e.value);
      const bool inside = visitor.strings[0].data() >= json.data() && visitor.strings[0].data() < json.data() + json.size();
      ASSERT_EQUAL(inside, e.in_document);
    }
    TEST_SUCCEED();
  }

  // Keeps the first byte of each number.
  struct raw_visitor : recording_visitor {
    std::string signs{};
    error_code on_int64(int64_t, const char *raw) { signs.push_back(raw[0]); return SUCCESS; }
    error_code on_uint64(uint64_t, const char *raw) { signs.push_back(raw[0]); return SUCCESS; }
    error_code on_double(double, const char *raw) { signs.push_back(raw[0]); return SUCCESS; }
  };

  bool raw_numbers() {
    TEST_START();
    ondemand::parser parser;
    struct expected { const char *text; const char *signs; };
    for (const expected &e : {expected{"[0,-0,1,-1,-0.0,18446744073709551615]", "0-1--1"}, expected{"-0", "-"}, expected{"0", "0"}}) {
      std::cout << "  " << e.text << std::endl;
      padded_string json{std::string(e.text)};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      raw_visitor visitor;
      ASSERT_SUCCESS(doc.visit(visitor));
      ASSERT_EQUAL(visitor.signs, e.signs);
    }
    TEST_SUCCEED();
  }

  // Fails on the third key.
  struct failing_visitor : recording_visitor {
    size_t keys{0};
    error_code on_key(std::string_view) { return ++keys == 3 ? INCORRECT_TYPE : SUCCESS; }
  };

  bool visitor_error_stops_the_walk() {
    TEST_START();
    auto json = R"({"a":1,"b":2,"c":3,"d":4})"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    failing_visitor visitor;
    ASSERT_ERROR(doc.visit(visitor), INCORRECT_TYPE);
    ASSERT_EQUAL(visitor.keys, 3);
    TEST_SUCCEED();
  }

  bool readable_after_visit() {
    TEST_START();
    auto json = R"({"a":[1,2,3],"b":{"c":"te\nxt"}})"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    string_visitor visitor;
    ASSERT_SUCCESS(doc.visit(visitor));
    std::string_view c;
    ASSERT_SUCCESS(doc["b"]["c"].get(c));
    ASSERT_EQUAL(c, "te\nxt");
    // A partly read document is rewound first.
    recording_visitor again;
    ASSERT_SUCCESS(doc.visit(again));
    std::string events;
    ASSERT_SUCCESS(visit_text(parser, std::string(json.data(), json.size()), &events));
    ASSERT_EQUAL(again.rec.out, events);
    TEST_SUCCEED();
  }

  // The strings of a visit, then those of an iteration, all with escapes: the string buffer
  // holds about 5/3 of the document, not twice as much.
  bool iterate_after_visit_with_escapes() {
    TEST_START();
    std::string text = "[\"";
    for (size_t i = 0; i < 1000000; i++) { text.push_back(i % 1000 == 0 ? '\\' : 'x'); if (i % 1000 == 0) { text.push_back('n'); } }
    text.append("\"]");
    padded_string json(text);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    recording_visitor visitor;
    ASSERT_SUCCESS(doc.visit(visitor));
    for (size_t pass = 0; pass < 3; pass++) {
      ondemand::array array;
      ASSERT_SUCCESS(doc.get_array().get(array));
      std::string_view value;
      for (auto element : array) { ASSERT_SUCCESS(element.get_string().get(value)); }
      ASSERT_EQUAL(value.size(), 1000000);
      doc.rewind();
    }
    ASSERT_SUCCESS(doc.visit(visitor));
    TEST_SUCCEED();
  }

  bool readable_root_scalar_after_visit() {
    TEST_START();
    ondemand::parser parser;
    auto number = "1234"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(number).get(doc));
    recording_visitor visitor;
    ASSERT_SUCCESS(doc.visit(visitor));
    int64_t value;
    ASSERT_SUCCESS(doc.get(value));
    ASSERT_EQUAL(value, 1234);
    auto text = R"("te\nxt")"_padded;
    ASSERT_SUCCESS(parser.iterate(text).get(doc));
    ASSERT_SUCCESS(doc.visit(visitor));
    std::string_view view;
    ASSERT_SUCCESS(doc.get(view));
    ASSERT_EQUAL(view, "te\nxt");
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS
  // Throws on the second key.
  struct throwing_visitor : recording_visitor {
    size_t keys{0};
    error_code on_key(std::string_view) {
      if (++keys == 2) { throw std::runtime_error("visitor failure"); }
      return SUCCESS;
    }
  };

  bool throwing_visitor_propagates() {
    TEST_START();
    auto json = R"({"a":"x\ny","b":2,"c":3})"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    throwing_visitor visitor;
    bool thrown = false;
    try {
      if (doc.visit(visitor)) { return false; }
    } catch (const std::runtime_error &) {
      thrown = true;
    }
    ASSERT_TRUE(thrown);
    doc.rewind();
    std::string events;
    recording_visitor again;
    ASSERT_SUCCESS(doc.visit(again));
    ASSERT_SUCCESS(visit_text(parser, std::string(json.data(), json.size()), &events));
    ASSERT_EQUAL(again.rec.out, events);
    TEST_SUCCEED();
  }
#endif

  bool result_forwarding() {
    TEST_START();
    auto good = R"([1,{"a":"b"}])"_padded;
    auto bad = R"([1,{"a" "b"}])"_padded;
    ondemand::parser parser;
    recording_visitor visitor;
    simdjson_result<ondemand::document> doc = parser.iterate(good);
    ASSERT_SUCCESS(doc.visit(visitor));
    doc = parser.iterate(bad);
    ASSERT_ERROR(doc.visit(visitor), TAPE_ERROR);
    TEST_SUCCEED();
  }

  // The bytes after a document, inside its padding, may be anything.
  bool garbage_padding() {
    TEST_START();
    struct checked { const char *text; bool valid; };
    const checked documents[] = {
      {R"([1,2])", true}, {R"({"a":"b"})", true}, {R"(["x"])", true}, {"12", true}, {"true", true},
      {R"("x")", true}, {"[1,2", false}, {R"({"a":"b")", false}, {R"(["x")", false}, {"[12", false},
      {"[tru", false}, {R"({"a")", false}, {"[", false},
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
        recording_visitor visitor;
        error_code error = doc.visit(visitor);
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
        ASSERT_EQUAL(visit_text(parser, text), dom_error);
        ASSERT_TRUE(same_as_dom(parser, dom_parser, text));
      }
    }
    TEST_SUCCEED();
  }

  // Two children under one array, each nested `inner` levels.
  // The second child is objects when second_is_object is set.
  std::string two_deep(size_t inner, bool second_is_object) {
    const std::string arrays(inner, '[');
    const std::string array_close(inner, ']');
    std::string second_open = arrays;
    std::string second_close = array_close;
    if (second_is_object) {
      second_open.clear();
      for (size_t i = 0; i < inner; i++) { second_open.append("{\"a\":"); }
      second_close.assign(inner, '}');
    }
    std::string text;
    text.reserve(2 + 2 * inner + second_open.size() + second_close.size() + 4);
    text.push_back('[');
    text.append(arrays);
    text.push_back('1');
    text.append(array_close);
    text.push_back(',');
    text.append(second_open);
    text.push_back('2');
    text.append(second_close);
    text.push_back(']');
    return text;
  }

  // The walk keeps 64 levels on a fixed stack, then moves to the heap. A second
  // container at that depth must not copy the fixed stack again.
  bool deep_siblings_after_stack_spill() {
    TEST_START();
    ondemand::parser parser;
    dom::parser dom_parser;
    constexpr size_t inner = 63;
    ASSERT_TRUE(same_as_dom(parser, dom_parser, two_deep(inner, false)));
    ASSERT_TRUE(same_as_dom(parser, dom_parser, two_deep(inner, true)));
    TEST_SUCCEED();
  }

  bool deep_nesting_with_larger_max_depth() {
    TEST_START();
    ondemand::parser parser;
    ASSERT_SUCCESS(parser.allocate(1000000, 5000));
    // Past the depth kept on the stack: the walk moves to the heap.
    std::string events;
    ASSERT_SUCCESS(visit_text(parser, nested(3000), &events));
    ASSERT_EQUAL(std::count(events.begin(), events.end(), '['), 1500);
    ASSERT_ERROR(visit_text(parser, nested(3000) + "]"), TRAILING_CONTENT);
    ASSERT_ERROR(visit_text(parser, nested(5001)), DEPTH_ERROR);
    TEST_SUCCEED();
  }

  bool stream_documents() {
    TEST_START();
    auto json = R"({"a":[1,2]} [3,"x"] 4 "five" {"b":null} true)"_padded;
    const char *expected[] = {R"({"a":[1,2]})", R"([3,"x"])", "4", R"("five")", R"({"b":null})", "true"};
    ondemand::parser stream_parser;
    ondemand::parser parser;
    ondemand::document_stream stream;
    ASSERT_SUCCESS(stream_parser.iterate_many(json).get(stream));
    size_t count = 0;
    for (auto result : stream) {
      ondemand::document_reference doc_ref;
      ASSERT_SUCCESS(std::move(result).get(doc_ref));
      ondemand::document &doc = doc_ref;
      recording_visitor visitor;
      ASSERT_SUCCESS(doc.visit(visitor));
      std::string events;
      ASSERT_SUCCESS(visit_text(parser, expected[count], &events));
      ASSERT_EQUAL(visitor.rec.out, events);
      count++;
    }
    ASSERT_EQUAL(count, 6);
    TEST_SUCCEED();
  }

  // A number that fails to parse is not passed to the visitor.
  bool invalid_number_is_not_visited() {
    TEST_START();
    const char *documents[] = {
      "[12a]", "[1.5x]", "[-7x]", "[18446744073709551615x]", "[1.2345678901234567890123x]",
      "{\"a\":12a}", "[1e400x]",
    };
    ondemand::parser parser;
    for (const char *text : documents) {
      std::cout << "  " << text << std::endl;
      padded_string json{std::string(text)};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      recording_visitor visitor;
      ASSERT_ERROR(doc.visit(visitor), NUMBER_ERROR);
      ASSERT_TRUE(visitor.rec.out == "[" || visitor.rec.out == "{k1:a");
      recording_visitor one_pass;
      ASSERT_ERROR(parser.visit(json, one_pass), NUMBER_ERROR);
      ASSERT_EQUAL(one_pass.rec.out, visitor.rec.out);
    }
    TEST_SUCCEED();
  }

#if SIMDJSON_ENABLE_NAN_INF
  // A malformed NaN or infinity gives the error code of DOM.
  bool nan_inf_errors_match_dom() {
    TEST_START();
    struct invalid { const char *text; error_code error; };
    const invalid documents[] = {
      {"Nope", TAPE_ERROR}, {"ix", TAPE_ERROR}, {"infx", TAPE_ERROR}, {"Infinityx", TAPE_ERROR},
      {"[Nope]", TAPE_ERROR}, {"[ix]", TAPE_ERROR}, {"[infx]", TAPE_ERROR}, {"[Infinityx]", TAPE_ERROR},
      {"{\"a\":Nan1}", TAPE_ERROR}, {"nul", N_ATOM_ERROR}, {"[nul]", N_ATOM_ERROR}, {"[nanx]", N_ATOM_ERROR},
      {"+1", TAPE_ERROR},
    };
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const invalid &document : documents) {
      std::cout << "  " << document.text << std::endl;
      padded_string copy{std::string(document.text)};
      ASSERT_ERROR(dom_parser.parse(copy).error(), document.error);
      ASSERT_ERROR(visit_text(parser, document.text), document.error);
      ASSERT_ERROR(parser_visit_text(parser, document.text), document.error);
    }
    TEST_SUCCEED();
  }

  bool nan_inf_matches_dom() {
    TEST_START();
    const char *documents[] = {
      "nan", "NaN", "Infinity", "inf", "-Infinity", "nul", "Nope", "ix",
      "[nan]", "[NaN,Infinity,inf,-Infinity]", "[nul]", "[Nope]", "[ix]", R"({"a":nan,"b":Infinity})",
    };
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *text : documents) {
      std::cout << "  " << text << std::endl;
      ASSERT_TRUE(same_as_dom(parser, dom_parser, text));
    }
    TEST_SUCCEED();
  }
#endif

  // Random edits of valid documents: visit() and DOM parsing must agree on validity, and on
  // the values of the valid documents.
  bool agrees_with_dom() {
    TEST_START();
    const char *seeds[] = {
      R"({"a":[1,2,{"b":"x\n\u00e9"}],"c":true,"d":null,"e":-1.5e3,"f":{}})",
      R"([[1,[2,[3,[]]]],{"k":"v","w":[true,false,null]},"s\"t",0.5,-0,1e10])",
      R"({"key":[{"k1":"v1"},{"k1":"v1"},{"k1":"v1"}]})",
      R"({"A\n":"\\","":[],"x":{"y":{"z":[[[[{}]]]]}}})",
      R"(["a string longer than thirty-two bytes","and another one, with \"escapes\""])",
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
      if (!same_as_dom(parser, dom_parser, text)) { return false; }
      padded_string copy(text);
      if (!dom_parser.parse(copy).error()) { valid++; }
    }
    std::cout << "  " << valid << " valid documents out of 20000" << std::endl;
    TEST_SUCCEED();
  }

  // Visits the first len bytes of buffer with iterate() and document::visit().
  error_code document_visit_buffer(ondemand::parser &parser, const std::vector<char> &buffer, size_t len, std::string &events) {
    ondemand::document doc;
    SIMDJSON_TRY(parser.iterate(buffer.data(), len, buffer.size()).get(doc));
    recording_visitor visitor;
    const error_code error = doc.visit(visitor);
    events = visitor.rec.out;
    return error;
  }

  error_code parser_visit_buffer(ondemand::parser &parser, const std::vector<char> &buffer, size_t len, std::string &events) {
    recording_visitor visitor;
    const error_code error = parser.visit(padded_string_view(buffer.data(), len, buffer.size()), visitor);
    events = visitor.rec.out;
    return error;
  }

  // Random edits of small documents, followed by random bytes: parser::visit() gives the error
  // code and the values of document::visit(), also with a small maximal depth.
  bool parser_visit_matches_document_visit() {
    TEST_START();
    const char *seeds[] = {
      R"({"a":[1,2],"b":"x"})", R"([true,false,null])", R"({"k":"\n\u00e9\"","n":-0.5})",
      R"([[],{},[[1]],{"a":{}}])", "[\"caf\\u00e9\",\"\xc3\xa9\",1e5]", R"({"big":18446744073709551615})",
      R"([12345678901234567890])", " [ 1 ,\t2 ]\n", R"({"a":"b"})", "[]", "{}", R"("str")", "12",
      R"([-0,0.1,1E2,-9223372036854775808])", R"({"a":{"b":{"c":[1]}}})",
      R"({"longer":"than the one-pass limit","x":[1,2,3]})",
    };
    const char alphabet[] = "{}[],:\"\\ 0123456789-+.eEtrufalsn\x01\xc3\xa9\xff";
    std::mt19937 gen(4321);
    ondemand::parser parser;
    ondemand::parser shallow;
    ASSERT_SUCCESS(shallow.allocate(64, 3));
    for (size_t i = 0; i < 200000; i++) {
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
      std::vector<char> buffer(text.begin(), text.end());
      buffer.resize(text.size() + SIMDJSON_PADDING);
      for (size_t j = text.size(); j < buffer.size(); j++) { buffer[j] = alphabet[gen() % (sizeof(alphabet) - 1)]; }
      for (ondemand::parser *p : {&parser, &shallow}) {
        std::string expected, events;
        const error_code expected_error = document_visit_buffer(*p, buffer, text.size(), expected);
        const error_code error = parser_visit_buffer(*p, buffer, text.size(), events);
        if (!same_visit(error, events, expected_error, expected)) {
          std::cerr << "  [" << text << "] document::visit: " << expected_error << " parser::visit: " << error << std::endl;
          return false;
        }
      }
    }
    TEST_SUCCEED();
  }

  bool parser_visit_strings_are_views() {
    TEST_START();
    auto json = R"({"plain":"value","esc\naped":1})"_padded;
    ondemand::parser parser;
    string_visitor visitor;
    ASSERT_SUCCESS(parser.visit(json, visitor));
    ASSERT_EQUAL(visitor.strings.size(), 3);
    const std::string_view expected[] = {"plain", "value", "esc\naped"};
    const bool in_document[] = {true, true, false};
    for (size_t i = 0; i < 3; i++) {
      ASSERT_EQUAL(visitor.strings[i], expected[i]);
      const bool inside = visitor.strings[i].data() >= json.data() && visitor.strings[i].data() < json.data() + json.size();
      ASSERT_EQUAL(inside, in_document[i]);
    }
    TEST_SUCCEED();
  }

  bool parser_visit_visitor_error() {
    TEST_START();
    ondemand::parser parser;
    for (const char *text : {R"({"a":1,"b":2,"c":3,"d":4})", R"({"a":1,"b":2,"c":3,"d":"longer than the one-pass limit"})"}) {
      std::cout << "  " << text << std::endl;
      failing_visitor visitor;
      ASSERT_ERROR(parser.visit(padded_string(std::string(text)), visitor), INCORRECT_TYPE);
      ASSERT_EQUAL(visitor.keys, 3);
    }
    // In one pass, the UTF-8 check comes at the end: the visitor fails first.
    failing_visitor visitor;
    const error_code error = parser.visit(padded_string(std::string("{\"a\":1,\"b\":2,\"c\":3,\"d\":\"\xff\"}")), visitor);
#if SIMDJSON_ENABLE_NAN_INF || SIMDJSON_ALLOW_TRAILING_COMMAS
    ASSERT_ERROR(error, UTF8_ERROR);
#else
    ASSERT_ERROR(error, INCORRECT_TYPE);
#endif
    TEST_SUCCEED();
  }

  // On an invalid document, the one-pass walk may call the visitor before it finds the error.
  bool parser_visit_prefix_on_error() {
    TEST_START();
    ondemand::parser parser;
    std::string events;
    ASSERT_ERROR(visit_text(parser, R"(["\"])", &events), UNCLOSED_STRING);
    ASSERT_EQUAL(events, "");
    ASSERT_ERROR(parser_visit_text(parser, R"(["\"])", &events), UNCLOSED_STRING);
#if SIMDJSON_ENABLE_NAN_INF || SIMDJSON_ALLOW_TRAILING_COMMAS
    ASSERT_EQUAL(events, "");
#else
    ASSERT_EQUAL(events, "[");
#endif
    TEST_SUCCEED();
  }

  // The events of a DOM element, or an empty string on an error.
  std::string dom_events(simdjson_result<dom::element> result) {
    dom::element element;
    recorder rec;
    if (result.get(element) || !record_dom(element, rec)) { return ""; }
    return rec.out;
  }

  bool value_visit_matches_dom() {
    TEST_START();
    const char *values[] = {
      R"({"a":[1,2,{"b":"x"}],"c":true,"d":null,"f":-1.5e3})", "[]", "{}", "[[[[]]]]",
      R"({"key \"esc\"":"\u00e9\ud83d\ude00"})", R"(["caf\u00e9","plain",""])",
      R"("plain")", R"("te\nxt")", "42", "-0", "1.5e-3", "18446744073709551615",
      "-9223372036854775808", "true", "false", "null", R"([0,-0,1.5,9223372036854775808])",
    };
    ondemand::parser parser;
    dom::parser dom_parser;
    for (const char *value_text : values) {
      const std::string text = std::string(R"({"before":[1,{"x":2}],"payload":)") + value_text + R"(,"after":"end"})";
      std::cout << "  " << text << std::endl;
      padded_string json(text);
      const std::string expected = dom_events(dom_parser.parse(json)["payload"]);
      ASSERT_TRUE(!expected.empty());
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      recording_visitor visitor;
      ASSERT_SUCCESS(doc["payload"].visit(visitor));
      ASSERT_EQUAL(visitor.rec.out, expected);
      std::string_view after;
      ASSERT_SUCCESS(doc["after"].get(after));
      ASSERT_EQUAL(after, "end");
    }
    TEST_SUCCEED();
  }

  bool value_visit_array_elements() {
    TEST_START();
    auto json = R"([{"a":1},[2,"t\"x"],"s",3,-2.5,true,null,{"b":{}},[]])"_padded;
    dom::parser dom_parser;
    dom::array dom_array;
    ASSERT_SUCCESS(dom_parser.parse(json).get(dom_array));
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::array array;
    ASSERT_SUCCESS(doc.get_array().get(array));
    size_t i = 0;
    for (auto element : array) {
      ondemand::value value;
      ASSERT_SUCCESS(element.get(value));
      recording_visitor visitor;
      ASSERT_SUCCESS(value.visit(visitor));
      ASSERT_EQUAL(visitor.rec.out, dom_events(dom_array.at(i)));
      i++;
    }
    ASSERT_EQUAL(i, dom_array.size());
    TEST_SUCCEED();
  }

  bool value_visit_root_and_read_value() {
    TEST_START();
    auto json = R"({"payload":{"x":1,"y":[2,"z"]},"after":3})"_padded;
    std::string expected;
    ondemand::parser parser;
    ASSERT_SUCCESS(visit_text(parser, std::string(json.data(), json.size()), &expected));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value root;
    ASSERT_SUCCESS(doc.get_value().get(root));
    recording_visitor root_visitor;
    ASSERT_SUCCESS(root.visit(root_visitor));
    ASSERT_EQUAL(root_visitor.rec.out, expected);

    dom::parser dom_parser;
    const std::string payload = dom_events(dom_parser.parse(json)["payload"]);
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value value;
    ASSERT_SUCCESS(doc["payload"].get(value));
    for (size_t round = 0; round < 2; round++) {
      recording_visitor visitor;
      ASSERT_SUCCESS(value.visit(visitor));
      ASSERT_EQUAL(visitor.rec.out, payload);
    }
    ondemand::object object;
    ASSERT_SUCCESS(value.get_object().get(object));
    int64_t x;
    ASSERT_SUCCESS(object["x"].get(x));
    ASSERT_EQUAL(x, 1);
    recording_visitor late;
    ASSERT_ERROR(value.visit(late), OUT_OF_ORDER_ITERATION);
    ASSERT_EQUAL(late.rec.out, "");
    int64_t after;
    ASSERT_SUCCESS(doc["after"].get(after));
    ASSERT_EQUAL(after, 3);
    TEST_SUCCEED();
  }

  bool value_visit_errors() {
    TEST_START();
    struct invalid { const char *text; error_code error; };
    const invalid documents[] = {
      {R"({"p":[1,tru],"b":2})", T_ATOM_ERROR},
      {R"({"p":{"x" 1},"b":2})", TAPE_ERROR},
      {R"({"p":[1,2,],"b":2})", TAPE_ERROR},
      {R"({"p":["\x"],"b":2})", STRING_ERROR},
      {R"({"p":[01],"b":2})", NUMBER_ERROR},
      {R"({"p":nul,"b":2})", N_ATOM_ERROR},
    };
    ondemand::parser parser;
    for (const invalid &document : documents) {
      std::cout << "  " << document.text << std::endl;
      if (dom_text(document.text) != document.text) { continue; }
      padded_string json{std::string(document.text)};
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      recording_visitor visitor;
      ASSERT_ERROR(doc["p"].visit(visitor), document.error);
    }
    auto json = R"({"p":{"a":1,"b":2,"c":3,"d":4}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    recording_visitor visitor;
    ASSERT_ERROR(doc["missing"].visit(visitor), NO_SUCH_FIELD);
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    failing_visitor failing;
    ASSERT_ERROR(doc["p"].visit(failing), INCORRECT_TYPE);
    ASSERT_EQUAL(failing.keys, 3);
    TEST_SUCCEED();
  }

  // Each visit of a value reuses the same part of the string buffer: escaped strings visited many
  // times, then read, do not overflow it.
  bool value_visit_string_buffer() {
    TEST_START();
    const std::string escaped = std::string(1000, 'a') + "\\n";
    const std::string text = "{\"p\":[\"" + escaped + "\",\"" + escaped + "\"],\"q\":\"" + escaped + "\"}";
    padded_string json(text);
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value value;
    ASSERT_SUCCESS(doc["p"].get(value));
    for (size_t round = 0; round < 4; round++) {
      string_visitor visitor;
      ASSERT_SUCCESS(value.visit(visitor));
      ASSERT_EQUAL(visitor.strings.size(), 2);
      ASSERT_EQUAL(visitor.strings[1], std::string(1000, 'a') + "\n");
    }
    size_t count = 0;
    for (auto element : value.get_array()) {
      std::string_view s;
      ASSERT_SUCCESS(element.get(s));
      ASSERT_EQUAL(s, std::string(1000, 'a') + "\n");
      count++;
    }
    ASSERT_EQUAL(count, 2);
    std::string_view q;
    ASSERT_SUCCESS(doc["q"].get(q));
    ASSERT_EQUAL(q, std::string(1000, 'a') + "\n");
    TEST_SUCCEED();
  }

  // The maximal depth counts from the root of the document, as in DOM, also when the value is
  // deeper than the fixed stack of the walk.
  bool value_visit_depth() {
    TEST_START();
    {
      ondemand::parser parser;
      ASSERT_SUCCESS(parser.allocate(64, 4));
      struct example { const char *text; error_code error; };
      const example examples[] = {
        {R"({"a":{"b":[1]}})", SUCCESS}, {R"({"a":{"b":[[1]]}})", DEPTH_ERROR}, {R"({"a":{"b":[[]]}})", SUCCESS},
      };
      dom::parser dom_parser;
      ASSERT_SUCCESS(dom_parser.allocate(64, 4));
      for (const example &e : examples) {
        std::cout << "  " << e.text << std::endl;
        padded_string json{std::string(e.text)};
        ASSERT_ERROR(dom_parser.parse(json).error(), e.error);
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        recording_visitor visitor;
        ASSERT_ERROR(doc["a"].visit(visitor), e.error);
      }
    }
    const size_t depth = 100;
    const std::string text = nested(depth);
    padded_string json(text);
    for (size_t components : {size_t(10), size_t(63), size_t(64), size_t(70), size_t(98)}) {
      std::string pointer;
      for (size_t i = 0; i < components; i++) { pointer.append(i % 2 == 0 ? "/0" : "/a"); }
      for (size_t max_depth : {size_t(DEFAULT_MAX_DEPTH), size_t(depth)}) {
        dom::parser dom_parser;
        ASSERT_SUCCESS(dom_parser.allocate(json.size(), max_depth));
        const error_code dom_error = dom_parser.parse(json).error();
        ondemand::parser parser;
        ASSERT_SUCCESS(parser.allocate(json.size(), max_depth));
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        recording_visitor visitor;
        const error_code error = doc.at_pointer(pointer).visit(visitor);
        ASSERT_EQUAL(error, dom_error);
        if (!error) {
          ASSERT_EQUAL(visitor.rec.out, dom_events(dom_parser.parse(json).at_pointer(pointer)));
        }
      }
    }
    TEST_SUCCEED();
  }

  bool run() {
    return valid_documents() &&
           example_files() &&
           invalid_documents() &&
           strings_without_escapes_are_views() &&
           root_strings_without_escapes_are_views() &&
           raw_numbers() &&
           visitor_error_stops_the_walk() &&
           readable_after_visit() &&
           iterate_after_visit_with_escapes() &&
           readable_root_scalar_after_visit() &&
#if SIMDJSON_EXCEPTIONS
           throwing_visitor_propagates() &&
#endif
           result_forwarding() &&
           garbage_padding() &&
           nesting_limit_matches_dom() &&
           deep_siblings_after_stack_spill() &&
           deep_nesting_with_larger_max_depth() &&
           stream_documents() &&
           invalid_number_is_not_visited() &&
#if SIMDJSON_ENABLE_NAN_INF
           nan_inf_errors_match_dom() &&
           nan_inf_matches_dom() &&
#endif
           agrees_with_dom() &&
           parser_visit_matches_document_visit() &&
           parser_visit_strings_are_views() &&
           parser_visit_visitor_error() &&
           parser_visit_prefix_on_error() &&
           value_visit_matches_dom() &&
           value_visit_array_elements() &&
           value_visit_root_and_read_value() &&
           value_visit_errors() &&
           value_visit_string_buffer() &&
           value_visit_depth() &&
           true;
  }

} // namespace visit_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, visit_tests::run);
}
