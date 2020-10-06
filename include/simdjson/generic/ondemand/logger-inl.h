namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {
namespace logger {

static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
static constexpr const int LOG_EVENT_LEN = 20;
static constexpr const int LOG_BUFFER_LEN = 30;
static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
static int log_depth = 0; // Not threadsafe. Log only.

// Helper to turn unprintable or newline characters into spaces
static simdjson_really_inline char printable_char(char c) {
  if (c >= 0x20) {
    return c;
  } else {
    return ' ';
  }
}

simdjson_really_inline void log_event(const json_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}
simdjson_really_inline void log_value(const json_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}
simdjson_really_inline void log_start_value(const json_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_line(iter, "+", type, "", delta, depth_delta);
  log_depth++;
}
simdjson_really_inline void log_end_value(const json_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_depth--;
  log_line(iter, "-", type, "", delta, depth_delta);
}
simdjson_really_inline void log_error(const json_iterator &iter, const char *error, const char *detail, int delta, int depth_delta) noexcept {
  log_line(iter, "ERROR: ", error, detail, delta, depth_delta);
}

simdjson_really_inline void log_headers() noexcept {
  log_depth = 0;
  if (LOG_ENABLED) {
    printf("\n");
    printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
    printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    fflush(stdout);
  }
}

simdjson_really_inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept {
  if (LOG_ENABLED) {
    const int indent = (log_depth+depth_delta)*2;
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
    fflush(stdout);
  }
}

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
