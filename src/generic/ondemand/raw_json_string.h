#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A string escaped per JSON rules, terminated with quote (")
 *
 * (In other words, a pointer to the beginning of a string, just after the start quote, inside a
 * JSON file.)
 */
class raw_json_string {
public:
  simdjson_really_inline raw_json_string() noexcept;
  simdjson_really_inline raw_json_string(const uint8_t * _buf) noexcept;
  simdjson_really_inline raw_json_string(const raw_json_string &other) noexcept;
  simdjson_really_inline raw_json_string &operator=(const raw_json_string &other) noexcept;
  simdjson_really_inline const char * raw() const noexcept;
  simdjson_really_inline SIMDJSON_WARN_UNUSED simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept;
private:
  const uint8_t * buf;
  friend class object;
};

SIMDJSON_UNUSED simdjson_really_inline bool operator==(const raw_json_string &a, std::string_view b) noexcept;
SIMDJSON_UNUSED simdjson_really_inline bool operator==(std::string_view a, const raw_json_string &b) noexcept;

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
