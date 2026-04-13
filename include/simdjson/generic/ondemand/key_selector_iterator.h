#ifndef SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_ITERATOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_ITERATOR_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/key_selector.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/value.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <array>
#include <cstddef>
#include <utility>

#if SIMDJSON_SUPPORTS_CONCEPTS

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * Forward iterator over selector matches in an object.
 *
 * Walks the JSON object once, yielding each (selector_index, value) pair whose
 * key matches one of the selector's keys, in JSON order. Duplicate matches of
 * the same key are silently skipped; iteration ends when every selector key
 * has matched OR the object ends.
 *
 *   for (auto [i, val] : obj.select<sel_t>()) {
 *     switch (i) { case 0: ...; case 1: ...; }
 *   }
 */
template <typename Selector>
class selector_iterator {
public:
    // Yield ondemand::value directly (not simdjson_result<value>); the caller
    // uses iterator state (error()) to check for errors after iteration.
    using value_type = std::pair<std::size_t, value>;

    struct end_sentinel {};

    simdjson_inline selector_iterator() noexcept = default;

    simdjson_inline explicit selector_iterator(object obj) noexcept
      : obj_{std::move(obj)} {
        auto begin_res = obj_.begin();
        if (begin_res.error()) { done_ = true; last_error_ = begin_res.error(); return; }
        it_ = begin_res.value();
        advance();
    }

    simdjson_inline value_type operator*() noexcept {
        return { current_index_, std::move(current_value_) };
    }

    simdjson_inline selector_iterator& operator++() noexcept { advance(); return *this; }

    simdjson_inline bool operator==(end_sentinel) const noexcept { return done_; }
    simdjson_inline bool operator!=(end_sentinel) const noexcept { return !done_; }

    /** Error code set if iteration was terminated by an error. */
    simdjson_inline error_code error() const noexcept { return last_error_; }
    /** Number of unique selector-key matches produced so far. */
    simdjson_inline std::size_t matched_count() const noexcept { return matched_; }

private:
    object obj_{};
    object_iterator it_{};
    std::array<bool, Selector::size()> seen_{};
    std::size_t matched_{0};
    std::size_t current_index_{Selector::size()};
    value current_value_{};
    error_code last_error_{SUCCESS};
    bool done_{false};
    bool primed_{false};

    simdjson_inline void advance() noexcept {
        if (done_) return;
        if (primed_) { ++it_; primed_ = false; }
        if (matched_ >= Selector::size()) { done_ = true; return; }

        object_iterator end{};
        while (it_ != end) {
            auto f_res = *it_;
            if (f_res.error()) { last_error_ = f_res.error(); done_ = true; return; }
            field f = f_res.value_unsafe();
            std::size_t idx = Selector::match_raw(f.key());
            if (idx < Selector::size() && !seen_[idx]) {
                seen_[idx] = true;
                ++matched_;
                current_index_ = idx;
                current_value_ = std::move(f.value());
                primed_ = true;
                return;
            }
            ++it_;
        }
        done_ = true;
    }
};

/**
 * Range adapter returned by object::select<Selector>(). Satisfies the range-for
 * loop requirements (begin() / end()).
 */
template <typename Selector>
class selector_range {
public:
    simdjson_inline explicit selector_range(object obj) noexcept
      : obj_{std::move(obj)} {}

    simdjson_inline selector_iterator<Selector> begin() noexcept {
        return selector_iterator<Selector>{std::move(obj_)};
    }
    simdjson_inline typename selector_iterator<Selector>::end_sentinel end() const noexcept {
        return {};
    }

private:
    object obj_;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS

#endif // SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_ITERATOR_H
