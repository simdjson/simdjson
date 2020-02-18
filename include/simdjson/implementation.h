// Declaration order requires we get to document.h before implementation.h no matter what
#include "simdjson/document.h"

#ifndef SIMDJSON_IMPLEMENTATION_H
#define SIMDJSON_IMPLEMENTATION_H

#include <optional>
#include <string>
#include <atomic>
#include <vector>

namespace simdjson {

/**
 * An implementation of simdjson for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on startup to the most advanced implementation supported by the host.
 */
class implementation {

/*
 * Active implementation management
 */
public:
  /**
   * Get the current active implementation.
   *
   * All simdjson parsing will be done using this implementation.
   *
   *     const implementation &impl = simdjson::implementation::active();
   *     cout << "simdjson is optimized for " << impl.name() << "(" << impl.description() << ")" << endl;
   *
   * @return the active implementation.
   */
  static const implementation & active() noexcept { return *_active.load(); }

  /**
   * Set the current active implementation.
   *
   * All simdjson parsing will be done using this implementation.
   *
   *     auto impl = simdjson::implementation::from_string("westmere");
   *     if (!impl) { exit(1); }
   *     simdjson::implementation::use(*impl);
   *
   * @param arch the implementation to use
   */
  static void use(const implementation &arch) noexcept { _active = &arch; }

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     auto impl = simdjson::implementation::from_string("westmere");
   *     if (!impl) { exit(1); }
   *     simdjson::implementation::use(*impl);
   *
   * @param arch the implementation string, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  static const implementation * from_string(const std::string &arch) noexcept;

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation &impl = simdjson::implementation::detect();
   *     simdjson::implementation::use(impl);
   *
   * @return the most advanced supported implementation for the current host.
   */
  static const implementation & detect() noexcept;

  /**
   * Get the list of available implementations supported by simdjson.
   *
   * @return available implementations
   */
  static const std::vector<const implementation *> &available_implementations() noexcept {
    return _available_implementations;
  }

private:
  /**
   * The active implementation.
   *
   * Automatically initialized to the most advanced implementation supported by this hardware.
   */
  static std::atomic<const implementation *> _active;

  /**
   * Known implementations.
   */
  static const std::vector<const implementation *> _available_implementations;

/*
 * Implementation interface
 */
public:
  /**
   * The name of this implementation.
   *
   *     const implementation &impl = simdjson::implementation::active();
   *     cout << "simdjson is optimized for " << impl.name() << "(" << impl.description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  const std::string &name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation &impl = simdjson::implementation::active();
   *     cout << "simdjson is optimized for " << impl.name() << "(" << impl.description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  const std::string &description() const { return _description; }

  /**
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `instruction_set` values
   */
  uint32_t required_instruction_sets() const { return _required_instruction_sets; };

  /**
   * Run a full document parse (init_parse, stage1 and stage2).
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code parse(const uint8_t *buf, size_t len, document::parser &parser) const noexcept = 0;

  /**
   * Stage 1 of the document parser.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @param streaming whether this is being called by a JsonStream parser.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage1(const uint8_t *buf, size_t len, document::parser &parser, bool streaming) const noexcept = 0;

  /**
   * Stage 2 of the document parser.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @return the error code, or SUCCESS if there was no error.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, document::parser &parser) const noexcept = 0;

  /**
   * Stage 2 of the document parser for JsonStream.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len the length of the json document.
   * @param parser the parser with the buffers to use. *MUST* have allocated up to at least len capacity.
   * @param next_json the next structural index. Start this at 0 the first time, and it will be updated to the next value to pass each time.
   * @return the error code, SUCCESS if there was no error, or SUCCESS_AND_HAS_MORE if there was no error and stage2 can be called again.
   */
  WARN_UNUSED virtual error_code stage2(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json) const noexcept = 0;

protected:
  really_inline implementation(
    const std::string &name,
    const std::string &description,
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

} // namespace simdjson

#endif // SIMDJSON_IMPLEMENTATION_H
