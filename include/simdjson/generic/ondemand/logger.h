namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class json_iterator;

namespace logger {

#if SIMDJSON_VERBOSE_LOGGING
  static constexpr const bool LOG_ENABLED = true;
#else
  static constexpr const bool LOG_ENABLED = false;
#endif

static simdjson_really_inline void log_headers() noexcept;
static simdjson_really_inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
static simdjson_really_inline void log_event(const json_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_value(const json_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_start_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_end_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_error(const json_iterator &iter, const char *error, const char *detail="", int delta=-1, int depth_delta=0) noexcept;

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
