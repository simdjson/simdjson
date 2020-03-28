#ifndef SIMDJSON_INLINE_DOCUMENT_STREAM_H
#define SIMDJSON_INLINE_DOCUMENT_STREAM_H

#include "simdjson/document_stream.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <thread>

namespace simdjson::internal {

/**
 * This algorithm is used to quickly identify the buffer position of
 * the last JSON document inside the current batch.
 *
 * It does its work by finding the last pair of structural characters
 * that represent the end followed by the start of a document.
 *
 * Simply put, we iterate over the structural characters, starting from
 * the end. We consider that we found the end of a JSON document when the
 * first element of the pair is NOT one of these characters: '{' '[' ';' ','
 * and when the second element is NOT one of these characters: '}' '}' ';' ','.
 *
 * This simple comparison works most of the time, but it does not cover cases
 * where the batch's structural indexes contain a perfect amount of documents.
 * In such a case, we do not have access to the structural index which follows
 * the last document, therefore, we do not have access to the second element in
 * the pair, and means that we cannot identify the last document. To fix this
 * issue, we keep a count of the open and closed curly/square braces we found
 * while searching for the pair. When we find a pair AND the count of open and
 * closed curly/square braces is the same, we know that we just passed a
 * complete
 * document, therefore the last json buffer location is the end of the batch
 * */
inline size_t find_last_json_buf_idx(const uint8_t *buf, size_t size, const dom::parser &parser) {
  // this function can be generally useful
  if (parser.n_structural_indexes == 0)
    return 0;
  auto last_i = parser.n_structural_indexes - 1;
  if (parser.structural_indexes[last_i] == size) {
    if (last_i == 0)
      return 0;
    last_i = parser.n_structural_indexes - 2;
  }
  auto arr_cnt = 0;
  auto obj_cnt = 0;
  for (auto i = last_i; i > 0; i--) {
    auto idxb = parser.structural_indexes[i];
    switch (buf[idxb]) {
    case ':':
    case ',':
      continue;
    case '}':
      obj_cnt--;
      continue;
    case ']':
      arr_cnt--;
      continue;
    case '{':
      obj_cnt++;
      break;
    case '[':
      arr_cnt++;
      break;
    }
    auto idxa = parser.structural_indexes[i - 1];
    switch (buf[idxa]) {
    case '{':
    case '[':
    case ':':
    case ',':
      continue;
    }
    if (!arr_cnt && !obj_cnt) {
      return last_i + 1;
    }
    return i;
  }
  return 0;
}

// returns true if the provided byte value is an ASCII character
static inline bool is_ascii(char c) {
  return ((unsigned char)c) <= 127;
}

// if the string ends with  UTF-8 values, backtrack 
// up to the first ASCII character. May return 0.
static inline size_t trimmed_length_safe_utf8(const char * c, size_t len) {
  while ((len > 0) and (not is_ascii(c[len - 1]))) {
    len--;
  }
  return len;
}

} // namespace simdjson::internal

namespace simdjson::dom {

really_inline document_stream::document_stream(
  dom::parser &_parser,
  const uint8_t *buf,
  size_t len,
  size_t batch_size,
  error_code _error
) noexcept : parser{_parser}, _buf{buf}, _len{len}, _batch_size(batch_size), error{_error} {
  if (!error) { error = json_parse(); }
}

inline document_stream::~document_stream() noexcept {
#ifdef SIMDJSON_THREADS_ENABLED
  if (stage_1_thread.joinable()) {
    stage_1_thread.join();
  }
#endif
}

really_inline document_stream::iterator document_stream::begin() noexcept {
  return iterator(*this, false);
}

really_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(*this, true);
}

really_inline document_stream::iterator::iterator(document_stream& _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

really_inline simdjson_result<element> document_stream::iterator::operator*() noexcept {
  error_code err = stream.error == SUCCESS_AND_HAS_MORE ? SUCCESS : stream.error;
  if (err) { return err; }
  return stream.parser.doc.root();
}

really_inline document_stream::iterator& document_stream::iterator::operator++() noexcept {
  if (stream.error == SUCCESS_AND_HAS_MORE) {
    stream.error = stream.json_parse();
  } else {
    finished = true;
  }
  return *this;
}

really_inline bool document_stream::iterator::operator!=(const document_stream::iterator &other) const noexcept {
  return finished != other.finished;
}

#ifdef SIMDJSON_THREADS_ENABLED

// threaded version of json_parse
// todo: simplify this code further
inline error_code document_stream::json_parse() noexcept {
  error = parser.ensure_capacity(_batch_size);
  if (error) { return error; }
  error = parser_thread.ensure_capacity(_batch_size);
  if (error) { return error; }

  if (unlikely(load_next_batch)) {
    // First time loading
    if (!stage_1_thread.joinable()) {
      _batch_size = (std::min)(_batch_size, remaining());
      _batch_size = internal::trimmed_length_safe_utf8((const char *)buf(), _batch_size);
      if (_batch_size == 0) {
        return simdjson::UTF8_ERROR;
      }
      auto stage1_is_ok = error_code(simdjson::active_implementation->stage1(buf(), _batch_size, parser, true));
      if (stage1_is_ok != simdjson::SUCCESS) {
        return stage1_is_ok;
      }
      size_t last_index = internal::find_last_json_buf_idx(buf(), _batch_size, parser);
      if (last_index == 0) {
        if (parser.n_structural_indexes == 0) {
          return simdjson::EMPTY;
        }
      } else {
        parser.n_structural_indexes = last_index + 1;
      }
    }
    // the second thread is running or done.
    else {
      stage_1_thread.join();
      if (stage1_is_ok_thread != simdjson::SUCCESS) {
        return stage1_is_ok_thread;
      }
      std::swap(parser.structural_indexes, parser_thread.structural_indexes);
      parser.n_structural_indexes = parser_thread.n_structural_indexes;
      advance(last_json_buffer_loc);
      n_bytes_parsed += last_json_buffer_loc;
    }
    // let us decide whether we will start a new thread
    if (remaining() - _batch_size > 0) {
      last_json_buffer_loc =
          parser.structural_indexes[internal::find_last_json_buf_idx(buf(), _batch_size, parser)];
      _batch_size = (std::min)(_batch_size, remaining() - last_json_buffer_loc);
      if (_batch_size > 0) {
        _batch_size = internal::trimmed_length_safe_utf8(
            (const char *)(buf() + last_json_buffer_loc), _batch_size);
        if (_batch_size == 0) {
          return simdjson::UTF8_ERROR;
        }
        // let us capture read-only variables
        const uint8_t *const b = buf() + last_json_buffer_loc;
        const size_t bs = _batch_size;
        // we call the thread on a lambda that will update
        // this->stage1_is_ok_thread
        // there is only one thread that may write to this value
        stage_1_thread = std::thread([this, b, bs] {
          this->stage1_is_ok_thread = error_code(simdjson::active_implementation->stage1(b, bs, this->parser_thread, true));
        });
      }
    }
    next_json = 0;
    load_next_batch = false;
  } // load_next_batch
  error_code res = simdjson::active_implementation->stage2(buf(), remaining(), parser, next_json);
  if (res == simdjson::SUCCESS_AND_HAS_MORE) {
    n_parsed_docs++;
    current_buffer_loc = parser.structural_indexes[next_json];
    load_next_batch = (current_buffer_loc == last_json_buffer_loc);
  } else if (res == simdjson::SUCCESS) {
    n_parsed_docs++;
    if (remaining() > _batch_size) {
      current_buffer_loc = parser.structural_indexes[next_json - 1];
      load_next_batch = true;
      res = simdjson::SUCCESS_AND_HAS_MORE;
    }
  }
  return res;
}

#else  // SIMDJSON_THREADS_ENABLED

// single-threaded version of json_parse
inline error_code document_stream::json_parse() noexcept {
  error = parser.ensure_capacity(_batch_size);
  if (error) { return error; }

  if (unlikely(load_next_batch)) {
    advance(current_buffer_loc);
    n_bytes_parsed += current_buffer_loc;
    _batch_size = (std::min)(_batch_size, remaining());
    _batch_size = internal::trimmed_length_safe_utf8((const char *)buf(), _batch_size);
    auto stage1_is_ok = (error_code)simdjson::active_implementation->stage1(buf(), _batch_size, parser, true);
    if (stage1_is_ok != simdjson::SUCCESS) {
      return stage1_is_ok;
    }
    size_t last_index = internal::find_last_json_buf_idx(buf(), _batch_size, parser);
    if (last_index == 0) {
      if (parser.n_structural_indexes == 0) {
        return EMPTY;
      }
    } else {
      parser.n_structural_indexes = last_index + 1;
    }
    load_next_batch = false;
  } // load_next_batch
  error_code res = simdjson::active_implementation->stage2(buf(), remaining(), parser, next_json);
  if (likely(res == simdjson::SUCCESS_AND_HAS_MORE)) {
    n_parsed_docs++;
    current_buffer_loc = parser.structural_indexes[next_json];
  } else if (res == simdjson::SUCCESS) {
    n_parsed_docs++;
    if (remaining() > _batch_size) {
      current_buffer_loc = parser.structural_indexes[next_json - 1];
      next_json = 1;
      load_next_batch = true;
      res = simdjson::SUCCESS_AND_HAS_MORE;
    }
  }
  return res;
}
#endif // SIMDJSON_THREADS_ENABLED

} // namespace simdjson::dom
#endif // SIMDJSON_INLINE_DOCUMENT_STREAM_H
