#ifndef SIMDJSON_IMPLEMENTATION_H
#define SIMDJSON_IMPLEMENTATION_H

#include "simdjson/internal/atomic_ptr.h"
#include "simdjson/internal/dom_parser_implementation.h"

#include <memory>

namespace simdjson {

/**
 * Validate the UTF-8 string.
 *
 * @param buf the string to validate.
 * @param len the length of the string in bytes.
 * @return true if the string is valid UTF-8.
 */
simdjson_warn_unused bool validate_utf8(const char * buf, size_t len) noexcept;
/**
 * Validate the UTF-8 string.
 *
 * @param sv the string_view to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_inline simdjson_warn_unused bool validate_utf8(const std::string_view sv) noexcept {
  return validate_utf8(sv.data(), sv.size());
}

/**
 * Validate the UTF-8 string.
 *
 * @param p the string to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_inline simdjson_warn_unused bool validate_utf8(const std::string& s) noexcept {
  return validate_utf8(s.data(), s.size());
}

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
   *     const implementation *impl = simdjson::get_active_implementation();
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64".
   */
  virtual std::string name() const { return std::string(_name); }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdjson::get_active_implementation();
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the description of the implementation, e.g. "Intel/AMD AVX2", "Intel/AMD SSE4.2", "ARM NEON".
   */
  virtual std::string description() const { return std::string(_description); }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   * @return true if the implementation can be safely used on the current system (determined at runtime).
   */
  bool supported_by_runtime_system() const;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values.
   */
  virtual uint32_t required_instruction_sets() const { return _required_instruction_sets; }

  /**
   * @private For internal implementation use
   *
   *     const implementation *impl = simdjson::get_active_implementation();
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @param capacity The largest document that will be passed to the parser.
   * @param max_depth The maximum JSON object/array nesting this parser is expected to handle.
   * @param dst The place to put the resulting parser implementation.
   * @return the error code, or SUCCESS if there was no error.
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
  simdjson_warn_unused virtual error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept = 0;


  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  simdjson_warn_unused virtual bool validate_utf8(const char *buf, size_t len) const noexcept = 0;

protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  simdjson_inline implementation(
    std::string_view name,
    std::string_view description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
protected:
  ~implementation() = default;

private:
  /**
   * The name of this implementation.
   */
  std::string_view _name;

  /**
   * The description of this implementation.
   */
  std::string_view _description;

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
  simdjson_inline available_implementation_list() {}
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
   *     const implementation *impl = simdjson::get_available_implementations()["westmere"];
   *     if (!impl) { exit(1); }
   *     if (!imp->supported_by_runtime_system()) { exit(1); }
   *     simdjson::get_active_implementation() = impl;
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
   *     simdjson::get_active_implementation() = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

} // namespace internal

/**
 * The list of available implementations compiled into simdjson.
 */
extern SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list& get_available_implementations();

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation>& get_active_implementation();

} // namespace simdjson

#endif // SIMDJSON_IMPLEMENTATION_H
