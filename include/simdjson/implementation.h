#ifndef SIMDJSON_IMPLEMENTATION_H
#define SIMDJSON_IMPLEMENTATION_H

#include <optional>
#include <string>
#include <atomic>
#include <vector>
#include "simdjson/document.h"

namespace simdjson {

/**
 * An implementation of simdjson for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on first use to the most advanced implementation supported by the host.
 */
class implementation {
public:
  /**
   * The name of this implementation.
   *
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &description() const { return _description; }

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `instruction_set` values
   */
  virtual uint32_t required_instruction_sets() const { return _required_instruction_sets; };

  /**
   * @private For internal implementation use
   *
   * Run a full document parse (ensure_capacity, stage1 and stage2).
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code parse(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Run a full document parse (ensure_capacity, stage1 and stage2).
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param dst the buffer to write the minified document to. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param dst_len the number of bytes written. Output only.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 1 of the document parser.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @param streaming whether this is being called by parser::parse_many.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage1(const uint8_t *buf, size_t len, dom::parser &parser, bool streaming) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser for parser::parse_many.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @param next_json the next structural index. Start this at 0 the first time, and it will be updated to the next value to pass each time.
   * @return the error code, SUCCESS if there was no error, or SUCCESS_AND_HAS_MORE if there was no error and stage2 can be called again.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser, size_t &next_json) const noexcept = 0;

protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  really_inline implementation(
    std::string_view name,
    std::string_view description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }

private:
  /**
   * The name of this implementation.
   */
  const std::string _name;

  /**
   * The description of this implementation.
   */
  const std::string _description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into simdjson.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into simdjson */
  really_inline available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const implementation * const *begin() const noexcept;
  /** STL const end() iterator */
  const implementation * const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl = simdjson::available_implementations["westmere"];
   *     if (!impl) { exit(1); }
   *     simdjson::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const implementation * operator[](const std::string_view &name) const noexcept {
    for (const implementation * impl : *this) {
      if (impl->name() == name) { return impl; }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl = simdjson::available_implementation::detect_best_supported();
   *     simdjson::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  const std::string &name() const noexcept final { return set_best()->name(); }
  const std::string &description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept final {
    return set_best()->parse(buf, len, parser);
  }
  WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final {
    return set_best()->minify(buf, len, dst, dst_len);
  }
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, dom::parser &parser, bool streaming) const noexcept final {
    return set_best()->stage1(buf, len, parser, streaming);
  }
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser) const noexcept final {
    return set_best()->stage2(buf, len, parser);
  }
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, dom::parser &parser, size_t &next_json) const noexcept final {
    return set_best()->stage2(buf, len, parser, next_json);
  }

  really_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}
private:
  const implementation *set_best() const noexcept;
};

inline const detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;

template<typename T>
class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

  operator const T*() const { return ptr.load(); }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr.load(); }

  operator T*() { return ptr.load(); }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr.load(); }
  T* operator=(T *_ptr) { return ptr = _ptr; }

private:
  std::atomic<T*> ptr;
};

} // namespace [simdjson::]internal

/**
 * The list of available implementations compiled into simdjson.
 */
inline const internal::available_implementation_list available_implementations;

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  *
  * @hideinitializer
  */
inline internal::atomic_ptr<const implementation> active_implementation = &internal::detect_best_supported_implementation_on_first_use_singleton;

} // namespace simdjson

#endif // SIMDJSON_IMPLEMENTATION_H
