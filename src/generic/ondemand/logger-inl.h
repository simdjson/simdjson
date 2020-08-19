namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
namespace logger {

static constexpr const int LOG_EVENT_LEN = 20;
static constexpr const int LOG_BUFFER_LEN = 30;
static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
using SIMDJSON_IMPLEMENTATION::logger::DASHES;
using SIMDJSON_IMPLEMENTATION::logger::printable_char;

simdjson_really_inline void log_event(const token_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}
simdjson_really_inline void log_value(const token_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}
simdjson_really_inline void log_start_value(const token_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_line(iter, "+", type, "", delta, depth_delta);
}
simdjson_really_inline void log_end_value(const token_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_line(iter, "-", type, "", delta, depth_delta);
}
simdjson_really_inline void log_error(const token_iterator &iter, const char *error, int delta, int depth_delta) noexcept {
  log_line(iter, "", "ERROR", error, delta, depth_delta);
}

simdjson_really_inline void log_headers() noexcept {
  if (LOG_ENABLED) {
    printf("\n");
    printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
    printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
  }
}

simdjson_really_inline void log_line(const token_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept {
  if (LOG_ENABLED) {
    const int indent = (iter.depth+depth_delta)*2;
    printf("| %*s%s%-*s ",
      indent, "",
      title_prefix,
      LOG_EVENT_LEN - indent - int(strlen(title_prefix)), title
      );
    {
      // Print the current structural.
      printf("| ");
      for (int i=0;i<LOG_BUFFER_LEN;i++) {
        printf("%c", printable_char(iter.peek(delta)[i]));
      }
      printf(" ");
    }
    {
      // Print the next structural.
      printf("| ");
      for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
        printf("%c", printable_char(iter.peek(delta+1)[i]));
      }
      printf(" ");
    }
    printf("| %5u ", iter.peek_index(delta+1));
    printf("| %.*s ", int(detail.size()), detail.data());
    printf("|\n");
  }
}

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
