// This is for an internal-only stage 2 specific logger.
// Set LOG_ENABLED = true to log what stage 2 is doing!
namespace logger {
  static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

  static constexpr const bool LOG_ENABLED = false;
  static constexpr const int LOG_EVENT_LEN = 30;
  static constexpr const int LOG_BUFFER_LEN = 20;
  static constexpr const int LOG_DETAIL_LEN = 50;
  static constexpr const int LOG_INDEX_LEN = 10;

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
      printf("| %-*s | %-*s | %*s | %*s | %*s | %-*s | %-*s | %-*s |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", 4, "Curr", 4, "Next", 5, "Next#", 5, "Tape#", LOG_DETAIL_LEN, "Detail", LOG_INDEX_LEN, "index");
      printf("|%.*s|%.*s|%.*s|%.*s|%.*s|%.*s|%.*s|%.*s|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, 4+2, DASHES, 4+2, DASHES, 5+2, DASHES, 5+2, DASHES, LOG_DETAIL_LEN+2, DASHES, LOG_INDEX_LEN+2, DASHES);
    }
  }

  static really_inline void log_string(const char *message) {
    if (LOG_ENABLED) {
      printf("%s\n", message);
    }
  }

  // Logs a single line of 
  template<typename S>
  static really_inline void log_line(S &structurals, const char *title_prefix, const char *title, const char *detail) {
    if (LOG_ENABLED) {
      printf("| %*s%s%-*s ", log_depth*2, "", title_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(title_prefix)), title);
      {
        // Print the next N characters in the buffer.
        printf("| ");
        // Otherwise, print the characters starting from the buffer position.
        // Print spaces for unprintable or newline characters.
        for (int i=0;i<LOG_BUFFER_LEN;i++) {
          printf("%c", printable_char(structurals.current()[i]));
        }
        printf(" ");
      }
      printf("|    %c ", printable_char(structurals.current_char()));
      printf("|    %c ", printable_char(structurals.peek_next_char()));
      printf("| %5u ", structurals.parser.structural_indexes[*(structurals.current_structural+1)]);
      printf("| %5u ", structurals.next_tape_index());
      printf("| %-*s ", LOG_DETAIL_LEN, detail);
      printf("| %*u ", LOG_INDEX_LEN, *structurals.current_structural);
      printf("|\n");
    }
  }
} // namespace logger

