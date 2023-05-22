namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class json_iterator;
class value_iterator;

namespace logger {

enum class log_level : int32_t {
  LOG_INFO = 0,
  LOG_ERROR = 1
};

#if SIMDJSON_VERBOSE_LOGGING
  static constexpr const bool LOG_ENABLED = true;
#else
  static constexpr const bool LOG_ENABLED = false;
#endif

// We do not want these functions to be 'really inlined' since real inlining is
// for performance purposes and if you are using the loggers, you do not care about
// performance (or should not).
template<typename... Args>
static inline std::string string_format(const std::string& format, const Args&... args);
static inline void log_headers() noexcept;
static inline void log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail, log_level level) noexcept;
static inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta, log_level level) noexcept;
static inline void log_event(const json_iterator &iter, const char *type, std::string_view detail="", int delta=0, int depth_delta=0) noexcept;
static inline void log_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail="") noexcept;
static inline void log_value(const json_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static inline void log_start_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail="") noexcept;
static inline void log_start_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static inline void log_end_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static inline void log_error(const json_iterator &iter, token_position index, depth_t depth, const char *error, const char *detail="") noexcept;
static inline void log_error(const json_iterator &iter, const char *error, const char *detail="", int delta=-1, int depth_delta=0) noexcept;

static inline void log_event(const value_iterator &iter, const char *type, std::string_view detail="", int delta=0, int depth_delta=0) noexcept;
static inline void log_value(const value_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static inline void log_start_value(const value_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static inline void log_end_value(const value_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static inline void log_error(const value_iterator &iter, const char *error, const char *detail="", int delta=-1, int depth_delta=0) noexcept;

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
