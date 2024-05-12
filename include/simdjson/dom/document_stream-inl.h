#ifndef SIMDJSON_DOCUMENT_STREAM_INL_H
#define SIMDJSON_DOCUMENT_STREAM_INL_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/document_stream.h"
#include "simdjson/dom/element-inl.h"
#include "simdjson/dom/parser-inl.h"
#include "simdjson/error-inl.h"
#include "simdjson/internal/dom_parser_implementation.h"

namespace simdjson {
namespace dom {

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

inline void stage1_worker::run(document_stream * ds, dom::parser * stage1, size_t next_batch_start) {
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
#endif

simdjson_inline document_stream::document_stream(
  dom::parser &_parser,
  const uint8_t *_buf,
  size_t _len,
  size_t _batch_size
) noexcept
  : parser{&_parser},
    buf{_buf},
    len{_len},
    batch_size{_batch_size <= MINIMAL_BATCH_SIZE ? MINIMAL_BATCH_SIZE : _batch_size},
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
    error{UNINITIALIZED}
#ifdef SIMDJSON_THREADS_ENABLED
    , use_thread(false)
#endif
{
}

simdjson_inline document_stream::~document_stream() noexcept {
#ifdef SIMDJSON_THREADS_ENABLED
  worker.reset();
#endif
}

simdjson_inline document_stream::iterator::iterator() noexcept
  : stream{nullptr}, finished{true} {
}

simdjson_inline document_stream::iterator document_stream::begin() noexcept {
  start();
  // If there are no documents, we're finished.
  return iterator(this, error == EMPTY);
}

simdjson_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(this, true);
}

simdjson_inline document_stream::iterator::iterator(document_stream* _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

simdjson_inline document_stream::iterator::reference document_stream::iterator::operator*() noexcept {
  // Note that in case of error, we do not yet mark
  // the iterator as "finished": this detection is done
  // in the operator++ function since it is possible
  // to call operator++ repeatedly while omitting
  // calls to operator*.
  if (stream->error) { return stream->error; }
  return stream->parser->doc.root();
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

inline void document_stream::start() noexcept {
  if (error) { return; }
  error = parser->ensure_capacity(batch_size);
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
#ifdef SIMDJSON_THREADS_ENABLED
  if (use_thread && next_batch_start() < len) {
    // Kick off the first thread if needed
    error = stage1_thread_parser.ensure_capacity(batch_size);
    if (error) { return; }
    worker->start_thread();
    start_stage1_thread();
    if (error) { return; }
  }
#endif // SIMDJSON_THREADS_ENABLED
  next();
}

simdjson_inline size_t document_stream::iterator::current_index() const noexcept {
  return stream->doc_index;
}

simdjson_inline std::string_view document_stream::iterator::source() const noexcept {
  const char* start = reinterpret_cast<const char*>(stream->buf) + current_index();
  bool object_or_array = ((*start == '[') || (*start == '{'));
  if(object_or_array) {
    size_t next_doc_index = stream->batch_start + stream->parser->implementation->structural_indexes[stream->parser->implementation->next_structural_index - 1];
    return std::string_view(start, next_doc_index - current_index() + 1);
  } else {
    size_t next_doc_index = stream->batch_start + stream->parser->implementation->structural_indexes[stream->parser->implementation->next_structural_index];
    size_t svlen = next_doc_index - current_index();
    while(svlen > 1 && (std::isspace(start[svlen-1]) || start[svlen-1] == '\0')) {
      svlen--;
    }
    return std::string_view(start, svlen);
  }
}


inline void document_stream::next() noexcept {
  // We always exit at once, once in an error condition.
  if (error) { return; }

  // Load the next document from the batch
  doc_index = batch_start + parser->implementation->structural_indexes[parser->implementation->next_structural_index];
  error = parser->implementation->stage2_next(parser->doc);
  // If that was the last document in the batch, load another batch (if available)
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
    if (error) { continue; } // If the error was EMPTY, we may want to load another batch.
    // Run stage 2 on the first document in the batch
    doc_index = batch_start + parser->implementation->structural_indexes[parser->implementation->next_structural_index];
    error = parser->implementation->stage2_next(parser->doc);
  }
}
inline size_t document_stream::size_in_bytes() const noexcept {
  return len;
}

inline size_t document_stream::truncated_bytes() const noexcept {
  if(error == CAPACITY) { return len - batch_start; }
  return parser->implementation->structural_indexes[parser->implementation->n_structural_indexes] - parser->implementation->structural_indexes[parser->implementation->n_structural_indexes + 1];
}

inline size_t document_stream::next_batch_start() const noexcept {
  return batch_start + parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
}

inline error_code document_stream::run_stage1(dom::parser &p, size_t _batch_start) noexcept {
  size_t remaining = len - _batch_start;
  if (remaining <= batch_size) {
    return p.implementation->stage1(&buf[_batch_start], remaining, stage1_mode::streaming_final);
  } else {
    return p.implementation->stage1(&buf[_batch_start], batch_size, stage1_mode::streaming_partial);
  }
}

#ifdef SIMDJSON_THREADS_ENABLED

inline void document_stream::load_from_stage1_thread() noexcept {
  worker->finish();
  // Swap to the parser that was loaded up in the thread. Make sure the parser has
  // enough memory to swap to, as well.
  std::swap(*parser, stage1_thread_parser);
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

} // namespace dom

simdjson_inline simdjson_result<dom::document_stream>::simdjson_result() noexcept
  : simdjson_result_base() {
}
simdjson_inline simdjson_result<dom::document_stream>::simdjson_result(error_code error) noexcept
  : simdjson_result_base(error) {
}
simdjson_inline simdjson_result<dom::document_stream>::simdjson_result(dom::document_stream &&value) noexcept
  : simdjson_result_base(std::forward<dom::document_stream>(value)) {
}

#if SIMDJSON_EXCEPTIONS
simdjson_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
simdjson_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
#else // SIMDJSON_EXCEPTIONS
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
simdjson_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::begin() noexcept {
  first.error = error();
  return first.begin();
}
simdjson_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::end() noexcept {
  first.error = error();
  return first.end();
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API
#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson
#endif // SIMDJSON_DOCUMENT_STREAM_INL_H
