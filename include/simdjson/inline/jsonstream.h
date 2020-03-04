// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_INLINE_JSONSTREAM_H
#define SIMDJSON_INLINE_JSONSTREAM_H

#include "simdjson/jsonstream.h"
#include "simdjson/document.h"
#include "simdjson/document_stream.h"

namespace simdjson {

template <class string_container>
inline JsonStream<string_container>::JsonStream(const string_container &s, size_t _batch_size) noexcept
    : str(s), batch_size(_batch_size) {
}

template <class string_container>
inline JsonStream<string_container>::~JsonStream() noexcept {
  if (stream) { delete stream; }
}

template <class string_container>
inline int JsonStream<string_container>::json_parse(document::parser &parser) noexcept {
  if (unlikely(stream == nullptr)) {
    stream = new document::stream(parser, reinterpret_cast<const uint8_t*>(str.data()), str.length(), batch_size);
  } else {
    if (&parser != &stream->parser) { return stream->error = TAPE_ERROR; }
    stream->error = stream->json_parse();
  }
  return stream->error;
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_JSONSTREAM_H
