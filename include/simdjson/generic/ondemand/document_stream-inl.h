#include <algorithm>
#include <limits>
#include <stdexcept>
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document_stream::document_stream(
  ondemand::parser &_parser,
  const uint8_t *_buf,
  size_t _len,
  size_t _batch_size
) noexcept
  : parser{&_parser},
    buf{_buf},
    len{_len},
    batch_size{_batch_size <= MINIMAL_BATCH_SIZE ? MINIMAL_BATCH_SIZE : _batch_size},
    error{SUCCESS}
{}

simdjson_really_inline document_stream::document_stream() noexcept
  : parser{nullptr},
    buf{nullptr},
    len{0},
    batch_size{0},
    error{UNINITIALIZED}
{
}

simdjson_really_inline document_stream::~document_stream() noexcept
{
}

inline size_t document_stream::size_in_bytes() const noexcept {
  return len;
}

simdjson_really_inline document_stream::iterator::iterator() noexcept
  : stream{nullptr}, finished{true} {
}

simdjson_really_inline document_stream::iterator::iterator(document_stream* _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

simdjson_really_inline ondemand::document& document_stream::iterator::operator*() noexcept {
  return stream->doc;
}

simdjson_really_inline document_stream::iterator& document_stream::iterator::operator++() noexcept {
  // If there is an error, then we want the iterator
  // to be finished, no matter what. (E.g., we do not
  // keep generating documents with errors, or go beyond
  // a document with errors.)
  //
  // Users do not have to call "operator*()" when they use operator++,
  // so we need to end the stream in the operator++ function.
  //
  // Note that setting finished = true is essential otherwise
  // we would enter an infinite loop.
  if (stream->error) { finished = true; }
  // Note that stream->error() is guarded against error conditions
  // (it will immediately return if stream->error casts to false).
  // In effect, this next function does nothing when (stream->error)
  // is true (hence the risk of an infinite loop).
  stream->next();
  // If that was the last document, we're finished.
  // It is the only type of error we do not want to appear
  // in operator*.
  if (stream->error == EMPTY) { finished = true; }
  // If we had any other kind of error (not EMPTY) then we want
  // to pass it along to the operator* and we cannot mark the result
  // as "finished" just yet.
  return *this;
}

simdjson_really_inline bool document_stream::iterator::operator!=(const document_stream::iterator &other) const noexcept {
  return finished != other.finished;
}

simdjson_really_inline document_stream::iterator document_stream::begin() noexcept {
  start();
  // If there are no documents, we're finished.
  return iterator(this, error == EMPTY);
}

simdjson_really_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(this, true);
}

inline void document_stream::start() noexcept {
  if (error) { return; }
  error = parser->allocate(batch_size);
  if (error) { return; }
  // Always run the first stage 1 parse immediately
  batch_start = 0;
  error = run_stage1(*parser, batch_start);
  if(error == EMPTY) {
    // In exceptional cases, we may start with an empty block
    batch_start = next_batch_start();
    if (batch_start >= len) { return; }
    error = run_stage1(*parser, batch_start);
  }
  if (error) { return; }
  doc = document(json_iterator(buf, parser));
}

inline void document_stream::next() noexcept {
  // We always enter at once once in an error condition.
  if (error) { return; }
  next_document();
  if (error) { return; }
  doc_index = batch_start + parser->implementation->structural_indexes[doc.iter._root - parser->implementation->structural_indexes.get()];

  // Check if at end of structural indexes (i.e. at end of batch)
  if(doc.iter._root - parser->implementation->structural_indexes.get() >= parser->implementation->n_structural_indexes) {
    error = EMPTY;
    // Load another batch (if available)
    while (error == EMPTY) {
      batch_start = next_batch_start();
      if (batch_start >= len) { break; }
      error = run_stage1(*parser, batch_start);
      /**
       * Whenever we move to another window, we need to update all pointers to make
       * it appear as if the input buffer started at the beginning of the window.
       *
       * Take this input:
       *
       * {"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]  [15,  11,   12, 13]  [154,  110,   112, 1311]
       *
       * Say you process the following window...
       *
       * '{"z":5}  {"1":1,"2":2,"4":4} [7,  10,   9]'
       *
       * When you do so, the json_iterator has a pointer at the beginning of the memory region
       * (pointing at the beginning of '{"z"...'.
       *
       * When you move to the window that starts at...
       *
       * '[7,  10,   9]  [15,  11,   12, 13] ...
       *
       * then it is not sufficient to just run stage 1. You also need to re-anchor the
       * json_iterator so that it believes we are starting at '[7,  10,   9]...'.
       *
       * Under the DOM front-end, this gets done automatically because the parser owns
       * the pointer the data, and when you call stage1 and then stage2 on the same
       * parser, then stage2 will run on the pointer acquired by stage1.
       *
       * That is, stage1 calls "this->buf = _buf" so the parser remembers the buffer that
       * we used. But json_iterator has no callback when stage1 is called on the parser.
       * In fact, I think that the parser is unaware of json_iterator.
       *
       *
       * So we need to re-anchor the json_iterator after each call to stage 1 so that
       * all of the pointers are in sync.
       */
      doc.iter = json_iterator(&buf[batch_start], parser);
      /**
       * End of resync.
       */

      if (error) { continue; } // If the error was EMPTY, we may want to load another batch.
      doc_index = batch_start;
    }
  }
}

inline void document_stream::next_document() noexcept {
  // Go to next place where depth=0 (document depth)
  error = doc.iter.skip_child(0);
  if (error) { return; }
  // Always set depth=1 at the start of document
  doc.iter._depth = 1;
  // Resets the string buffer at the beginning, thus invalidating the strings.
  doc.iter._string_buf_loc = parser->string_buf.get();
  doc.iter._root = doc.iter.position();
}

inline size_t document_stream::next_batch_start() const noexcept {
  return batch_start + parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
}

inline error_code document_stream::run_stage1(ondemand::parser &p, size_t _batch_start) noexcept {
  // This code only updates the structural index in the parser, it does not update any json_iterator
  // instance.
  size_t remaining = len - _batch_start;
  if (remaining <= batch_size) {
    return p.implementation->stage1(&buf[_batch_start], remaining, stage1_mode::streaming_final);
  } else {
    return p.implementation->stage1(&buf[_batch_start], batch_size, stage1_mode::streaming_partial);
  }
}

simdjson_really_inline size_t document_stream::iterator::current_index() const noexcept {
  return stream->doc_index;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(error)
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document_stream &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(value)
    )
{
}

}