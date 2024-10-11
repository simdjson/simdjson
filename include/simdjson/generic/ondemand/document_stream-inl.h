#ifndef SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_STREAM_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_STREAM_INL_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/document_stream.h"
#include "simdjson/generic/ondemand/document-inl.h"
#include "simdjson/generic/implementation_simdjson_result_base-inl.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <algorithm>
#include <stdexcept>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

#ifdef SIMDJSON_THREADS_ENABLED

inline void stage1_worker::finish() {
  // After calling "run" someone would call finish() to wait
  // for the end of the processing.
  // This function will wait until either the thread has done
  // the processing or, else, the destructor has been called.
  std::unique_lock<std::mutex> lock(locking_mutex);
  cond_var.wait(lock, [this]{return has_work == false;});
}

inline stage1_worker::~stage1_worker() {
  // The thread may never outlive the stage1_worker instance
  // and will always be stopped/joined before the stage1_worker
  // instance is gone.
  stop_thread();
}

inline void stage1_worker::start_thread() {
  std::unique_lock<std::mutex> lock(locking_mutex);
  if(thread.joinable()) {
    return; // This should never happen but we never want to create more than one thread.
  }
  thread = std::thread([this]{
      while(true) {
        std::unique_lock<std::mutex> thread_lock(locking_mutex);
        // We wait for either "run" or "stop_thread" to be called.
        cond_var.wait(thread_lock, [this]{return has_work || !can_work;});
        // If, for some reason, the stop_thread() method was called (i.e., the
        // destructor of stage1_worker is called, then we want to immediately destroy
        // the thread (and not do any more processing).
        if(!can_work) {
          break;
        }
        this->owner->stage1_thread_error = this->owner->run_stage1(*this->stage1_thread_parser,
              this->_next_batch_start);
        this->has_work = false;
        // The condition variable call should be moved after thread_lock.unlock() for performance
        // reasons but thread sanitizers may report it as a data race if we do.
        // See https://stackoverflow.com/questions/35775501/c-should-condition-variable-be-notified-under-lock
        cond_var.notify_one(); // will notify "finish"
        thread_lock.unlock();
      }
    }
  );
}


inline void stage1_worker::stop_thread() {
  std::unique_lock<std::mutex> lock(locking_mutex);
  // We have to make sure that all locks can be released.
  can_work = false;
  has_work = false;
  cond_var.notify_all();
  lock.unlock();
  if(thread.joinable()) {
    thread.join();
  }
}

inline void stage1_worker::run(document_stream * ds, parser * stage1, size_t next_batch_start) {
  std::unique_lock<std::mutex> lock(locking_mutex);
  owner = ds;
  _next_batch_start = next_batch_start;
  stage1_thread_parser = stage1;
  has_work = true;
  // The condition variable call should be moved after thread_lock.unlock() for performance
  // reasons but thread sanitizers may report it as a data race if we do.
  // See https://stackoverflow.com/questions/35775501/c-should-condition-variable-be-notified-under-lock
  cond_var.notify_one(); // will notify the thread lock that we have work
  lock.unlock();
}

#endif  // SIMDJSON_THREADS_ENABLED

simdjson_inline document_stream::document_stream(
  ondemand::parser &_parser,
  const uint8_t *_buf,
  size_t _len,
  size_t _batch_size,
  bool _allow_comma_separated
) noexcept
  : parser{&_parser},
    buf{_buf},
    len{_len},
    batch_size{_batch_size <= MINIMAL_BATCH_SIZE ? MINIMAL_BATCH_SIZE : _batch_size},
    allow_comma_separated{_allow_comma_separated},
    error{SUCCESS}
    #ifdef SIMDJSON_THREADS_ENABLED
    , use_thread(_parser.threaded) // we need to make a copy because _parser.threaded can change
    #endif
{
#ifdef SIMDJSON_THREADS_ENABLED
  if(worker.get() == nullptr) {
    error = MEMALLOC;
  }
#endif
}

simdjson_inline document_stream::document_stream() noexcept
  : parser{nullptr},
    buf{nullptr},
    len{0},
    batch_size{0},
    allow_comma_separated{false},
    error{UNINITIALIZED}
    #ifdef SIMDJSON_THREADS_ENABLED
    , use_thread(false)
    #endif
{
}

simdjson_inline document_stream::~document_stream() noexcept
{
  #ifdef SIMDJSON_THREADS_ENABLED
  worker.reset();
  #endif
}

inline size_t document_stream::size_in_bytes() const noexcept {
  return len;
}

inline size_t document_stream::truncated_bytes() const noexcept {
  if(error == CAPACITY) { return len - batch_start; }
  return parser->implementation->structural_indexes[parser->implementation->n_structural_indexes] - parser->implementation->structural_indexes[parser->implementation->n_structural_indexes + 1];
}

simdjson_inline document_stream::iterator::iterator() noexcept
  : stream{nullptr}, finished{true} {
}

simdjson_inline document_stream::iterator::iterator(document_stream* _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

simdjson_inline simdjson_result<ondemand::document_reference> document_stream::iterator::operator*() noexcept {
  return simdjson_result<ondemand::document_reference>(stream->doc, stream->error);
}

simdjson_inline document_stream::iterator& document_stream::iterator::operator++() noexcept {
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

simdjson_inline bool document_stream::iterator::operator!=(const document_stream::iterator &other) const noexcept {
  return finished != other.finished;
}

simdjson_inline document_stream::iterator document_stream::begin() noexcept {
  start();
  // If there are no documents, we're finished.
  return iterator(this, error == EMPTY);
}

simdjson_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(this, true);
}

inline void document_stream::start() noexcept {
  if (error) { return; }
  error = parser->allocate(batch_size);
  if (error) { return; }
  // Always run the first stage 1 parse immediately
  batch_start = 0;
  error = run_stage1(*parser, batch_start);
  while(error == EMPTY) {
    // In exceptional cases, we may start with an empty block
    batch_start = next_batch_start();
    if (batch_start >= len) { return; }
    error = run_stage1(*parser, batch_start);
  }
  if (error) { return; }
  doc_index = batch_start;
  doc = document(json_iterator(&buf[batch_start], parser));
  doc.iter._streaming = true;

  #ifdef SIMDJSON_THREADS_ENABLED
  if (use_thread && next_batch_start() < len) {
    // Kick off the first thread on next batch if needed
    error = stage1_thread_parser.allocate(batch_size);
    if (error) { return; }
    worker->start_thread();
    start_stage1_thread();
    if (error) { return; }
  }
  #endif // SIMDJSON_THREADS_ENABLED
}

inline void document_stream::next() noexcept {
  // We always enter at once once in an error condition.
  if (error) { return; }
  next_document();
  if (error) { return; }
  auto cur_struct_index = doc.iter._root - parser->implementation->structural_indexes.get();
  doc_index = batch_start + parser->implementation->structural_indexes[cur_struct_index];

  // Check if at end of structural indexes (i.e. at end of batch)
  if(cur_struct_index >= static_cast<int64_t>(parser->implementation->n_structural_indexes)) {
    error = EMPTY;
    // Load another batch (if available)
    while (error == EMPTY) {
      batch_start = next_batch_start();
      if (batch_start >= len) { break; }
      #ifdef SIMDJSON_THREADS_ENABLED
      if(use_thread) {
        load_from_stage1_thread();
      } else {
        error = run_stage1(*parser, batch_start);
      }
      #else
      error = run_stage1(*parser, batch_start);
      #endif
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
      doc.iter._streaming = true;
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
  // consume comma if comma separated is allowed
  if (allow_comma_separated) { doc.iter.consume_character(','); }
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

simdjson_inline size_t document_stream::iterator::current_index() const noexcept {
  return stream->doc_index;
}

simdjson_inline std::string_view document_stream::iterator::source() const noexcept {
  auto depth = stream->doc.iter.depth();
  auto cur_struct_index = stream->doc.iter._root - stream->parser->implementation->structural_indexes.get();

  // If at root, process the first token to determine if scalar value
  if (stream->doc.iter.at_root()) {
    switch (stream->buf[stream->batch_start + stream->parser->implementation->structural_indexes[cur_struct_index]]) {
      case '{': case '[':   // Depth=1 already at start of document
        break;
      case '}': case ']':
        depth--;
        break;
      default:    // Scalar value document
        // TODO: We could remove trailing whitespaces
        // This returns a string spanning from start of value to the beginning of the next document (excluded)
        {
          auto next_index = stream->parser->implementation->structural_indexes[++cur_struct_index];
          // normally the length would be next_index - current_index() - 1, except for the last document
          size_t svlen = next_index - current_index();
          const char *start = reinterpret_cast<const char*>(stream->buf) + current_index();
          while(svlen > 1 && (std::isspace(start[svlen-1]) || start[svlen-1] == '\0')) {
            svlen--;
          }
          return std::string_view(start, svlen);
        }
    }
    cur_struct_index++;
  }

  while (cur_struct_index <= static_cast<int64_t>(stream->parser->implementation->n_structural_indexes)) {
    switch (stream->buf[stream->batch_start + stream->parser->implementation->structural_indexes[cur_struct_index]]) {
      case '{': case '[':
        depth++;
        break;
      case '}': case ']':
        depth--;
        break;
    }
    if (depth == 0) { break; }
    cur_struct_index++;
  }

  return std::string_view(reinterpret_cast<const char*>(stream->buf) + current_index(), stream->parser->implementation->structural_indexes[cur_struct_index] - current_index() + stream->batch_start + 1);;
}

inline error_code document_stream::iterator::error() const noexcept {
  return stream->error;
}

#ifdef SIMDJSON_THREADS_ENABLED

inline void document_stream::load_from_stage1_thread() noexcept {
  worker->finish();
  // Swap to the parser that was loaded up in the thread. Make sure the parser has
  // enough memory to swap to, as well.
  std::swap(stage1_thread_parser,*parser);
  error = stage1_thread_error;
  if (error) { return; }

  // If there's anything left, start the stage 1 thread!
  if (next_batch_start() < len) {
    start_stage1_thread();
  }
}

inline void document_stream::start_stage1_thread() noexcept {
  // we call the thread on a lambda that will update
  // this->stage1_thread_error
  // there is only one thread that may write to this value
  // TODO this is NOT exception-safe.
  this->stage1_thread_error = UNINITIALIZED; // In case something goes wrong, make sure it's an error
  size_t _next_batch_start = this->next_batch_start();

  worker->run(this, & this->stage1_thread_parser, _next_batch_start);
}

#endif // SIMDJSON_THREADS_ENABLED

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(error)
{
}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::document_stream &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::document_stream>(value)
    )
{
}

}

#endif // SIMDJSON_GENERIC_ONDEMAND_DOCUMENT_STREAM_INL_H