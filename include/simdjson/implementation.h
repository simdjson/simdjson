#ifndef SIMDJSON_IMPLEMENTATION_H
#define SIMDJSON_IMPLEMENTATION_H

#include "simdjson/common_defs.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include <string>
#include <atomic>
#include <vector>

namespace simdjson {

/**
 * Validate the UTF-8 string.
 *
 * @param buf the string to validate.
 * @param len the length of the string in bytes.
 * @return true if the string is valid UTF-8.
 */
SIMDJSON_WARN_UNUSED bool validate_utf8(const char * buf, size_t len) noexcept;


/**
 * Validate the UTF-8 string.
 *
 * @param sv the string_view to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_really_inline SIMDJSON_WARN_UNUSED bool validate_utf8(const std::string_view sv) noexcept {
  return validate_utf8(sv.data(), sv.size());
}

/**
 * Validate the UTF-8 string.
 *
 * @param p the string to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_really_inline SIMDJSON_WARN_UNUSED bool validate_utf8(const std::string& s) noexcept {
  return validate_utf8(s.data(), s.size());
}

namespace dom {
  class document;
} // namespace dom

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
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @param capacity The largest document that will be passed to the parser.
   * @param max_depth The maximum JSON object/array nesting this parser is expected to handle.
   * @param dst The place to put the resulting parser implementation.
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_depth,
    std::unique_ptr<internal::dom_parser_implementation> &dst
  ) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Minify the input string assuming that it represents a JSON string, does not parse or validate.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to minify.
   * @param len the length of the json document.
   * @param dst the buffer to write the minified document to. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param dst_len the number of bytes written. Output only.
   * @return the error code, or SUCCESS if there was no error.
   */
  SIMDJSON_WARN_UNUSED virtual error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept = 0;
  
  
  /**   
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  SIMDJSON_WARN_UNUSED virtual bool validate_utf8(const char *buf, size_t len) const noexcept = 0;

protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  simdjson_really_inline implementation(
    std::string_view name,
    std::string_view description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
  virtual ~implementation()=default;

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
  simdjson_really_inline available_implementation_list() {}
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
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }

private:
  std::atomic<T*> ptr;
};

} // namespace internal

/**
 * The list of available implementations compiled into simdjson.
 */
extern SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations;

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation;

} // namespace simdjson

#endif // SIMDJSON_IMPLEMENTATION_H
