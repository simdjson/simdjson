#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A JSON field in an field.
 */
class field : public std::pair<raw_json_string, value> {
public:
  simdjson_really_inline field() noexcept;
  simdjson_really_inline field(field &&other) noexcept;
  simdjson_really_inline field &operator=(field &&other) noexcept;
  simdjson_really_inline field(const field &other) noexcept = delete;
  simdjson_really_inline field &operator=(const field &other) noexcept = delete;

  simdjson_really_inline raw_json_string key() const noexcept;
  simdjson_really_inline ondemand::value &value() noexcept;
protected:
  simdjson_really_inline field(raw_json_string key, ondemand::value &&value) noexcept;
  static simdjson_really_inline simdjson_result<field> start(document *doc) noexcept;
  static simdjson_really_inline simdjson_result<field> start(document *doc, raw_json_string key) noexcept;
  friend struct simdjson_result<field>;
  friend class object;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> : public internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::field> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::field &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::document *doc, error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> key() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> value() noexcept;
};

} // namespace simdjson
