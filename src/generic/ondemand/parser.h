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
  inline parser() noexcept = default;
  inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;
  inline ~parser() noexcept = default;

  SIMDJSON_WARN_UNUSED error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_result<document> iterate(const padded_string &json) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_result<json_iterator> iterate_raw(const padded_string &json) noexcept;
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

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::parser &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline simdjson_result(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser> &&a) noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson
