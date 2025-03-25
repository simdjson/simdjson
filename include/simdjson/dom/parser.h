#ifndef SIMDJSON_DOM_PARSER_H
#define SIMDJSON_DOM_PARSER_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/document.h"

namespace simdjson {

namespace dom {

/**
 * A persistent document parser.
 *
 * The parser is designed to be reused, holding the internal buffers necessary to do parsing,
 * as well as memory for a single document. The parsed document is overwritten on each parse.
 *
 * This class cannot be copied, only moved, to avoid unintended allocations.
 *
 * @note Moving a parser instance may invalidate "dom::element" instances. If you need to
 * preserve both the "dom::element" instances and the parser, consider wrapping the parser
 * instance in a std::unique_ptr instance:
 *
 *   std::unique_ptr<dom::parser> parser(new dom::parser{});
 *   auto error = parser->load(f).get(root);
 *
 * You can then move std::unique_ptr safely.
 *
 * @note This is not thread safe: one parser cannot produce two documents at the same time!
 */
class parser {
public:
  /**
   * Create a JSON parser.
   *
   * The new parser will have zero capacity.
   *
   * @param max_capacity The maximum document length the parser can automatically handle. The parser
   *    will allocate more capacity on an as needed basis (when it sees documents too big to handle)
   *    up to this amount. The parser still starts with zero capacity no matter what this number is:
   *    to allocate an initial capacity, call allocate() after constructing the parser.
   *    Defaults to SIMDJSON_MAXSIZE_BYTES (the largest single document simdjson can process).
   */
  simdjson_inline explicit parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  simdjson_inline parser(parser &&other) noexcept;
  parser(const parser &) = delete; ///< @private Disallow copying
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  simdjson_inline parser &operator=(parser &&other) noexcept;
  parser &operator=(const parser &) = delete; ///< @private Disallow copying

  /** Deallocate the JSON parser. */
  ~parser()=default;

  /**
   * Load a JSON document from a file and return a reference to it.
   *
   *   dom::parser parser;
   *   const element doc = parser.load("jsonexamples/twitter.json");
   *
   * The function is eager: the file's content is loaded in memory inside the parser instance
   * and immediately parsed. The file can be deleted after the  `parser.load` call.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * Moving the parser instance is safe, but it invalidates the element instances. You may store
   * the parser instance without moving it by wrapping it inside an `unique_ptr` instance like
   * so: `std::unique_ptr<dom::parser> parser(new dom::parser{});`.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than the file length, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * ## Windows and Unicode
   *
   * Windows users who need to read files with non-ANSI characters in the
   * name should set their code page to UTF-8 (65001) before calling this
   * function. This should be the default with Windows 11 and better.
   * Further, they may use the AreFileApisANSI function to determine whether
   * the filename is interpreted using the ANSI or the system default OEM
   * codepage, and they may call SetFileApisToOEM accordingly.
   *
   * @param path The path to load.
   * @return The document, or an error:
   *         - IO_ERROR if there was an error opening or reading the file.
   *           Be mindful that on some 32-bit systems,
   *           the file size might be limited to 2 GB.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> load(const std::string &path) & noexcept;
  inline simdjson_result<element> load(const std::string &path) &&  = delete ;

  /**
   * Load a JSON document from a file into a provide document instance and return a temporary reference to it.
   * It is similar to the function `load` except that instead of parsing into the internal
   * `document` instance associated with the parser, it allows the user to provide a document
   * instance.
   *
   *   dom::parser parser;
   *   dom::document doc;
   *   element doc_root = parser.load_into_document(doc, "jsonexamples/twitter.json");
   *
   * The function is eager: the file's content is loaded in memory inside the parser instance
   * and immediately parsed. The file can be deleted after the `parser.load_into_document` call.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * After the call to load_into_document, the parser is no longer needed.
   *
   * The JSON document lives in the document instance: you must keep the document
   * instance alive while you navigate through it (i.e., used the returned value from
   * load_into_document). You are encourage to reuse the document instance
   * many times with new data to avoid reallocations:
   *
   *   dom::document doc;
   *   element doc_root1 = parser.load_into_document(doc, "jsonexamples/twitter.json");
   *   //... doc_root1 is a pointer inside doc
   *   element doc_root2 = parser.load_into_document(doc, "jsonexamples/twitter.json");
   *   //... doc_root2 is a pointer inside doc
   *   // at this point doc_root1 is no longer safe
   *
   * Moving the document instance is safe, but it invalidates the element instances. After
   * moving a document, you can recover safe access to the document root with its `root()` method.
   *
   * @param doc The document instance where the parsed data will be stored (on success).
   * @param path The path to load.
   * @return The document, or an error:
   *         - IO_ERROR if there was an error opening or reading the file.
   *           Be mindful that on some 32-bit systems,
   *           the file size might be limited to 2 GB.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> load_into_document(document& doc, const std::string &path) & noexcept;
  inline simdjson_result<element> load_into_document(document& doc, const std::string &path) && =delete;

  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   dom::parser parser;
   *   element doc_root = parser.parse(buf, len);
   *
   * The function eagerly parses the input: the input can be modified and discarded after
   * the `parser.parse(buf, len)` call has completed.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * Moving the parser instance is safe, but it invalidates the element instances. You may store
   * the parser instance without moving it by wrapping it inside an `unique_ptr` instance like
   * so: `std::unique_ptr<dom::parser> parser(new dom::parser{});`.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. These bytes will be read: if you
   * using a sanitizer that verifies that no uninitialized byte is read, then you should initialize the
   * SIMDJSON_PADDING bytes to avoid runtime warnings.
   *
   * If realloc_if_needed is true (the default), it is assumed that the buffer does *not* have enough padding,
   * and it is copied into an enlarged temporary buffer before parsing. Thus the following is safe:
   *
   *   const char *json      = R"({"key":"value"})";
   *   const size_t json_len = std::strlen(json);
   *   simdjson::dom::parser parser;
   *   simdjson::dom::element element = parser.parse(json, json_len);
   *
   * If you set realloc_if_needed to false (e.g., parser.parse(json, json_len, false)),
   * you must provide a buffer with at least SIMDJSON_PADDING extra bytes at the end.
   * The benefit of setting realloc_if_needed to false is that you avoid a temporary
   * memory allocation and a copy.
   *
   * The padded bytes may be read. It is not important how you initialize
   * these bytes though we recommend a sensible default like null character values or spaces.
   * For example, the following low-level code is safe:
   *
   *   const char *json      = R"({"key":"value"})";
   *   const size_t json_len = std::strlen(json);
   *   std::unique_ptr<char[]> padded_json_copy{new char[json_len + SIMDJSON_PADDING]};
   *   std::memcpy(padded_json_copy.get(), json, json_len);
   *   std::memset(padded_json_copy.get() + json_len, '\0', SIMDJSON_PADDING);
   *   simdjson::dom::parser parser;
   *   simdjson::dom::element element = parser.parse(padded_json_copy.get(), json_len, false);
   *
   * ### std::string references
   *
   * If you pass a mutable std::string reference (std::string&), the parser will seek to extend
   * its capacity to SIMDJSON_PADDING bytes beyond the end of the string.
   *
   * Whenever you pass an std::string reference, the parser will access the bytes beyond the end of
   * the string but before the end of the allocated memory (std::string::capacity()).
   * If you are using a sanitizer that checks for reading uninitialized bytes or std::string's
   * container-overflow checks, you may encounter sanitizer warnings.
   * You can safely ignore these warnings. Or you can call simdjson::pad(std::string&) to pad the
   * string with SIMDJSON_PADDING spaces: this function returns a simdjson::padding_string_view
   * which can be be passed to the parser's parse function:
   *
   *    std::string json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )";
   *    element doc = parser.parse(simdjson::pad(json));
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than len, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return An element pointing at the root of the document, or an error:
   *         - MEMALLOC if realloc_if_needed is true or the parser does not have enough capacity,
   *           and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  simdjson_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse(const std::string &s) & noexcept;
  simdjson_inline simdjson_result<element> parse(const std::string &s) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse(const padded_string &s) & noexcept;
  simdjson_inline simdjson_result<element> parse(const padded_string &s) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse(const padded_string_view &v) & noexcept;
  simdjson_inline simdjson_result<element> parse(const padded_string_view &v) && =delete;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_inline simdjson_result<element> parse(const char *buf) noexcept = delete;

  /**
   * Parse a JSON document into a provide document instance and return a temporary reference to it.
   * It is similar to the function `parse` except that instead of parsing into the internal
   * `document` instance associated with the parser, it allows the user to provide a document
   * instance.
   *
   *   dom::parser parser;
   *   dom::document doc;
   *   element doc_root = parser.parse_into_document(doc, buf, len);
   *
   * The function eagerly parses the input: the input can be modified and discarded after
   * the `parser.parse(buf, len)` call has completed.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * After the call to parse_into_document, the parser is no longer needed.
   *
   * The JSON document lives in the document instance: you must keep the document
   * instance alive while you navigate through it (i.e., used the returned value from
   * parse_into_document). You are encourage to reuse the document instance
   * many times with new data to avoid reallocations:
   *
   *   dom::document doc;
   *   element doc_root1 = parser.parse_into_document(doc, buf1, len);
   *   //... doc_root1 is a pointer inside doc
   *   element doc_root2 = parser.parse_into_document(doc, buf1, len);
   *   //... doc_root2 is a pointer inside doc
   *   // at this point doc_root1 is no longer safe
   *
   * Moving the document instance is safe, but it invalidates the element instances. After
   * moving a document, you can recover safe access to the document root with its `root()` method.
   *
   * @param doc The document instance where the parsed data will be stored (on success).
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return An element pointing at the root of document, or an error:
   *         - MEMALLOC if realloc_if_needed is true or the parser does not have enough capacity,
   *           and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> parse_into_document(document& doc, const uint8_t *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  inline simdjson_result<element> parse_into_document(document& doc, const uint8_t *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const char *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const char *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const std::string &s) & noexcept;
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const std::string &s) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const padded_string &s) & noexcept;
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const padded_string &s) && =delete;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_inline simdjson_result<element> parse_into_document(document& doc, const char *buf) noexcept = delete;

  /**
   * Load a file containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (const element doc : parser.load_many(path)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * The file is loaded in memory and can be safely deleted after the `parser.load_many(path)`
   * function has returned. The memory is held by the `parser` instance.
   *
   * The function is lazy: it may be that no more than one JSON document at a time is parsed.
   * And, possibly, no document many have been parsed when the `parser.load_many(path)` function
   * returned.
   *
   * If there is a UTF-8 BOM, the parser skips it.
   *
   * ### Format
   *
   * The file must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * Documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * The documents must not exceed batch_size bytes (by default 1MB) or they will fail to parse.
   * Setting batch_size to excessively large or excessively small values may impact negatively the
   * performance.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   dom::document_stream docs;
   *   auto error = parser.load_many(path).get(docs);
   *   if (error) { cerr << error << endl; exit(1); }
   *   for (auto doc : docs) {
   *     std::string_view title;
   *     if ((error = doc["title"].get(title)) { cerr << error << endl; exit(1); }
   *     cout << title << endl;
   *   }
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param path File name pointing at the concatenated JSON to parse.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 1MB (as simdjson::dom::DEFAULT_BATCH_SIZE), which has been a reasonable sweet
   *                   spot in our tests.
   *                   If you set the batch_size to a value smaller than simdjson::dom::MINIMAL_BATCH_SIZE
   *                   (currently 32B), it will be replaced by simdjson::dom::MINIMAL_BATCH_SIZE.
   * @return The stream, or an error. An empty input will yield 0 documents rather than an EMPTY error. Errors:
   *         - IO_ERROR if there was an error opening or reading the file.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<document_stream> load_many(const std::string &path, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (element doc : parser.parse_many(buf, len)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * No copy of the input buffer is made.
   *
   * The function is lazy: it may be that no more than one JSON document at a time is parsed.
   * And, possibly, no document many have been parsed when the `parser.load_many(path)` function
   * returned.
   *
   * The caller is responsabile to ensure that the input string data remains unchanged and is
   * not deleted during the loop. In particular, the following is unsafe and will not compile:
   *
   *   auto docs = parser.parse_many("[\"temporary data\"]"_padded);
   *   // here the string "[\"temporary data\"]" may no longer exist in memory
   *   // the parser instance may not have even accessed the input yet
   *   for (element doc : docs) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * The following is safe:
   *
   *   auto json = "[\"temporary data\"]"_padded;
   *   auto docs = parser.parse_many(json);
   *   for (element doc : docs) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * If there is a UTF-8 BOM, the parser skips it.
   *
   * ### Format
   *
   * The buffer must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * The documents must not exceed batch_size bytes (by default 1MB) or they will fail to parse.
   * Setting batch_size to excessively large or excessively small values may impact negatively the
   * performance.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   dom::document_stream docs;
   *   auto error = parser.load_many(path).get(docs);
   *   if (error) { cerr << error << endl; exit(1); }
   *   for (auto doc : docs) {
   *     std::string_view title;
   *     if ((error = doc["title"].get(title)) { cerr << error << endl; exit(1); }
   *     cout << title << endl;
   *   }
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated. These bytes will be read: if you
   * using a sanitizer that verifies that no uninitialized byte is read, then you should initialize the
   * SIMDJSON_PADDING bytes to avoid runtime warnings.
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream, or an error. An empty input will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<document_stream> parse_many(const uint8_t *buf, size_t len, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const char *buf, size_t len, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const std::string &s, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> parse_many(const std::string &&s, size_t batch_size) = delete;// unsafe
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const padded_string &s, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> parse_many(const padded_string &&s, size_t batch_size) = delete;// unsafe

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_result<document_stream> parse_many(const char *buf, size_t batch_size = dom::DEFAULT_BATCH_SIZE) noexcept = delete;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused inline error_code allocate(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
  /**
   * @private deprecated because it returns bool instead of error_code, which is our standard for
   * failures. Use allocate() instead.
   *
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return true if successful, false if allocation failed.
   */
  [[deprecated("Use allocate() instead.")]]
  simdjson_warn_unused inline bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API
  /**
   * The largest document this parser can support without reallocating.
   *
   * @return Current capacity, in bytes.
   */
  simdjson_inline size_t capacity() const noexcept;

  /**
   * The largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount.
   *
   * @return Maximum capacity, in bytes.
   */
  simdjson_inline size_t max_capacity() const noexcept;

  /**
   * The maximum level of nested object and arrays supported by this parser.
   *
   * @return Maximum depth, in bytes.
   */
  simdjson_pure simdjson_inline size_t max_depth() const noexcept;

  /**
   * Set max_capacity. This is the largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount as documents are passed
   * to it.
   *
   * Note: To avoid limiting the memory to an absurd value, such as zero or two bytes,
   * iff you try to set max_capacity to a value lower than MINIMAL_DOCUMENT_CAPACITY,
   * then the maximal capacity is set to MINIMAL_DOCUMENT_CAPACITY.
   *
   * This call will not allocate or deallocate, even if capacity is currently above max_capacity.
   *
   * @param max_capacity The new maximum capacity, in bytes.
   */
  simdjson_inline void set_max_capacity(size_t max_capacity) noexcept;

#ifdef SIMDJSON_THREADS_ENABLED
  /**
   * The parser instance can use threads when they are available to speed up some
   * operations. It is enabled by default. Changing this attribute will change the
   * behavior of the parser for future operations. Set to true by default.
   */
  bool threaded{true};
#else
  /**
   * When SIMDJSON_THREADS_ENABLED is not defined, the parser instance cannot use threads.
   */
  bool threaded{false};
#endif
  /** @private Use the new DOM API instead */
  class Iterator;
  /** @private Use simdjson_error instead */
  using InvalidJSON [[deprecated("Use simdjson_error instead")]] = simdjson_error;

  /** @private [for benchmarking access] The implementation to use */
  std::unique_ptr<internal::dom_parser_implementation> implementation{};

  /** @private Use `if (parser.parse(...).error())` instead */
  bool valid{false};
  /** @private Use `parser.parse(...).error()` instead */
  error_code error{UNINITIALIZED};

  /** @private Use `parser.parse(...).value()` instead */
  document doc{};

  /** @private returns true if the document parsed was valid */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline bool is_valid() const noexcept;

  /**
   * @private return an error code corresponding to the last parsing attempt, see
   * simdjson.h will return UNINITIALIZED if no parsing was attempted
   */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline int get_error_code() const noexcept;

  /** @private return the string equivalent of "get_error_code" */
  [[deprecated("Use error_message() on the result of parser.parse() instead, or cout << error")]]
  inline std::string get_error_message() const noexcept;

  /** @private */
  [[deprecated("Use cout << on the result of parser.parse() instead")]]
  inline bool print_json(std::ostream &os) const noexcept;

  /** @private Private and deprecated: use `parser.parse(...).doc.dump_raw_tape()` instead */
  inline bool dump_raw_tape(std::ostream &os) const noexcept;


private:
  /**
   * The maximum document length this parser will automatically support.
   *
   * The parser will not be automatically allocated above this amount.
   */
  size_t _max_capacity;

  /**
   * The loaded buffer (reused each time load() is called)
   */
  std::unique_ptr<char[]> loaded_bytes;

  /** Capacity of loaded_bytes buffer. */
  size_t _loaded_bytes_capacity{0};

  // all nodes are stored on the doc.tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the doc.tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  /**
   * Ensure we have enough capacity to handle at least desired_capacity bytes,
   * and auto-allocate if not. This also allocates memory if needed in the
   * internal document.
   */
  inline error_code ensure_capacity(size_t desired_capacity) noexcept;
  /**
   * Ensure we have enough capacity to handle at least desired_capacity bytes,
   * and auto-allocate if not. This also allocates memory if needed in the
   * provided document.
   */
  inline error_code ensure_capacity(document& doc, size_t desired_capacity) noexcept;

  /** Read the file into loaded_bytes */
  inline simdjson_result<size_t> read_file(const std::string &path) noexcept;

  friend class parser::Iterator;
  friend class document_stream;


}; // class parser

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_PARSER_H
