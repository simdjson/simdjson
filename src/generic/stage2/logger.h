// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace logger {

  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 20;
  static constexpr const int LOG_BUFFER_LEN = 30;
  static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
  static constexpr const int LOG_INDEX_LEN = 5;

  static int log_depth; // Not threadsafe. Log only.

  // Helper to turn unprintable or newline characters into spaces
  static really_inline char printable_char(char c) {
    if (c >= 0x20) {
      return c;
    } else {
      return ' ';
    }
  }

  // Print the header and set up log_start
  static really_inline void log_start() {
    if (LOG_ENABLED) {
      log_depth = 0;
      printf("\n");
      printf("| %-*s | %-*s | %-*s | %-*s | Detail |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_SMALL_BUFFER_LEN, "Next", 5, "Next#");
      printf("|%.*s|%.*s|%.*s|%.*s|--------|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_SMALL_BUFFER_LEN+2, DASHES, 5+2, DASHES);
    }
  }

  // Logs a single line of 
  template<typename S>
  static really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      auto current_index = structurals.at_beginning() ? nullptr : structurals.next_structural-1;
      auto next_index = structurals.next_structural;
      auto current = current_index ? &structurals.buf[*current_index] : (const uint8_t*)"                                                       ";
      auto next = &structurals.buf[*next_index];
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(current[i]));
        }
        printf(" ");
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
          printf("%c", printable_char(next[i]));
        }
        printf(" ");
      }
      if (current_index) {
        printf("| %*u ", LOG_INDEX_LEN, *current_index);
      } else {
        printf("| %-*s ", LOG_INDEX_LEN, "");
      }
      // printf("| %*u ", LOG_INDEX_LEN, structurals.next_tape_index());
      printf("| %-s ", detail);
      printf("|\n");
    }
  }

} // namespace logger
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
