/**
 * Locale-independent stream implementation for simdjson, optimized
 * for performance.
 */
#include <cstdint>

struct string_stream {
    template <size_t size> 
    void print(const char (&array)[size]) {
      for(size_t i = 0; i < size; ++i) { *this << array[i]; };
    }

    string_stream & operator<<(char ) {
        return *this;
    }

    string_stream & operator<<(uint64_t ) {
        return *this;
    }

    string_stream & operator<<(int64_t ) {
        return *this;        
    }

    string_stream & operator<<(double ) {
        return *this;
    }


    std::string str() {
        return std::string("nothing");
    }
};