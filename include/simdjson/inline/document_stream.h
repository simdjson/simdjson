#ifndef SIMDJSON_INLINE_DOCUMENT_STREAM_H
#define SIMDJSON_INLINE_DOCUMENT_STREAM_H

#include "simdjson/dom/document_stream.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace simdjson {
namespace dom {

#ifdef SIMDJSON_THREADS_ENABLED
inline void stage1_worker::finish() {
  // spinlock (burning cycles)
  while (has_work.load(std::memory_order_acquire)) {}
  // The acquire ensures follows the this->has_work.store(false, std::memory_order_release);
  // line in the thread and ensures that all stores are now visible in the main thread.
}

inline stage1_worker::~stage1_worker() {
  stop_thread();
}

inline void stage1_worker::start_thread() {
  thread = std::thread([this]{
      while(true) {
        // spinlock (burning cycles)
        while (!has_work.load(std::memory_order_acquire)) {
          if (!can_work) {
            return;
          }
        }
        this->owner->stage1_thread_error = this->owner->run_stage1(*this->stage1_thread_parser,
              this->_next_batch_start);
        this->has_work.store(false, std::memory_order_release); // going to finish
      }
  }
  );
}


inline void stage1_worker::stop_thread() {
  // We have to make sure that all locks can be released.
  can_work = false;
  has_work.store(false, std::memory_order_release);
  // The  release matches the while (!has_work.load(std::memory_order_acquire))  line
  // in the thread. We ensure that it sees the variable that we just store.
  // The thread will then terminate on its own. We could dispatch it.
  if(thread.joinable()) {
    thread.join();
  }
}

inline void stage1_worker::run(document_stream * ds, dom::parser * stage1, size_t next_batch_start) {
  // Experimentally, it does not seem to happen that the thread is not already started.
  owner = ds;
  _next_batch_start = next_batch_start;
  stage1_thread_parser = stage1;
  has_work.store(true, std::memory_order_release);
  // The  release matches the while (!has_work.load(std::memory_order_acquire))  line
  // in the thread. We ensure that it sees the variable that we just store.
}
#endif

really_inline document_stream::document_stream(
  dom::parser &_parser,
  const uint8_t *_buf,
  size_t _len,
  size_t _batch_size,
  error_code _error
) noexcept
  : parser{_parser},
    buf{_buf},
    len{_len},
    batch_size{_batch_size},
    error{_error}
{
#ifdef SIMDJSON_THREADS_ENABLED
  if(worker.get() == nullptr) {
    error = MEMALLOC;
  }
#endif
}

inline document_stream::~document_stream() noexcept {
}

really_inline document_stream::iterator document_stream::begin() noexcept {
  start();
  // If there are no documents, we're finished.
  return iterator(*this, error == EMPTY);
}

really_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(*this, true);
}

really_inline document_stream::iterator::iterator(document_stream& _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

really_inline simdjson_result<element> document_stream::iterator::operator*() noexcept {
  // Once we have yielded any errors, we're finished.
  if (stream.error) { finished = true; return stream.error; }
  return stream.parser.doc.root();
}

really_inline document_stream::iterator& document_stream::iterator::operator++() noexcept {
  stream.next();
  // If that was the last document, we're finished.
  if (stream.error == EMPTY) { finished = true; }
  return *this;
}

really_inline bool document_stream::iterator::operator!=(const document_stream::iterator &other) const noexcept {
  return finished != other.finished;
}

inline void document_stream::start() noexcept {
  if (error) { return; }

  error = parser.ensure_capacity(batch_size);
  if (error) { return; }

  // Always run the first stage 1 parse immediately
  batch_start = 0;
  error = run_stage1(parser, batch_start);
  if (error) { return; }

#ifdef SIMDJSON_THREADS_ENABLED
  if (next_batch_start() < len) {
    // Kick off the first thread if needed
    error = stage1_thread_parser.ensure_capacity(batch_size);
    if (error) { return; }
    worker->start_thread();
    /**
     * If the thread took too long to start, we could move
     * worker->start_thread(); earlier.
     **/
    start_stage1_thread();
    if (error) { return; }
  }
#endif // SIMDJSON_THREADS_ENABLED

  next();
}

inline void document_stream::next() noexcept {
  if (error) { return; }

  // Load the next document from the batch
  error = parser.implementation->stage2_next(parser.doc);
  // If that was the last document in the batch, load another batch (if available)
  while (error == EMPTY) {
    batch_start = next_batch_start();
    if (batch_start >= len) { break; }

#ifdef SIMDJSON_THREADS_ENABLED
    load_from_stage1_thread();
#else
    error = run_stage1(parser, batch_start);
#endif
    if (error) { continue; } // If the error was EMPTY, we may want to load another batch.
    // Run stage 2 on the first document in the batch
    error = parser.implementation->stage2_next(parser.doc);
  }
}

inline size_t document_stream::next_batch_start() const noexcept {
  return batch_start + parser.implementation->structural_indexes[parser.implementation->n_structural_indexes];
}

inline error_code document_stream::run_stage1(dom::parser &p, size_t _batch_start) noexcept {
  // If this is the final batch, pass partial = false
  size_t remaining = len - _batch_start;
  if (remaining <= batch_size) {
    return p.implementation->stage1(&buf[_batch_start], remaining, false);
  } else {
    return p.implementation->stage1(&buf[_batch_start], batch_size, true);
  }
}

#ifdef SIMDJSON_THREADS_ENABLED

inline void document_stream::load_from_stage1_thread() noexcept {
  worker->finish();
  // Swap to the parser that was loaded up in the thread. Make sure the parser has
  // enough memory to swap to, as well.
  std::swap(parser, stage1_thread_parser);
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
} // namespace simdjson
#endif // SIMDJSON_INLINE_DOCUMENT_STREAM_H
