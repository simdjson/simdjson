#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class object;
class value;
class raw_json_string;

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class parser {
public:
  simdjson_really_inline parser() noexcept = default;
  simdjson_really_inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;

  SIMDJSON_WARN_UNUSED error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_result<document> parse(const padded_string &json) noexcept;
private:
  dom_parser_implementation dom_parser{};
  size_t _capacity{0};
  size_t _max_depth{0};
  std::unique_ptr<uint8_t[]> string_buf{};

  friend class json_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
