#ifndef SIMDJSON_GENERIC_ONDEMAND_LOGGER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_LOGGER_H
#include "simdjson/generic/ondemand/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

// Logging should be free unless SIMDJSON_VERBOSE_LOGGING is set. Importantly, it is critical
// that the call to the log functions be side-effect free. Thus, for example, you should not
// create temporary std::string instances.
namespace logger {

enum class log_level : int32_t {
  info = 0,
  error = 1
};

#if SIMDJSON_VERBOSE_LOGGING
  static constexpr const bool LOG_ENABLED = true;
#else
  static constexpr const bool LOG_ENABLED = false;
#endif

// We do not want these functions to be 'really inlined' since real inlining is
// for performance purposes and if you are using the loggers, you do not care about
// performance (or should not).
static inline void log_headers() noexcept;
// If args are provided, title will be treated as format string
template <typename... Args>
static inline void log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail, logger::log_level level, Args&&... args) noexcept;
template <typename... Args>
static inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta, logger::log_level level, Args&&... args) noexcept;
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

#endif // SIMDJSON_GENERIC_ONDEMAND_LOGGER_H