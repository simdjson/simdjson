# Plan: `iterate_unpadded` for the On-Demand API

This is a design/execution plan for adding an *unpadded* entry point to the
On-Demand API, analogous to `dom::parser::parse_unpadded`. It is a planning
document for discussion, not a description of shipped behaviour.

## Goal

Let users run the On-Demand API on a buffer that has **no** trailing
`SIMDJSON_PADDING` bytes, without the library copying the whole input into a
padded buffer first. The library must never read past `buf + len`.

The On-Demand `iterate(...)` entry points currently *require* padding: they call
`json.has_sufficient_padding()` and return `INSUFFICIENT_PADDING` otherwise.

## Background: how the DOM case was solved

`dom::parser::parse_unpadded` (see the "Parsing without padding (DOM)" note in
[basics.md](basics.md)) parses in place with **zero penalty on the padded path**. That was possible
because DOM parsing has a single chokepoint — `tape_builder` — which we templated
on a compile-time `bool UNPADDED`. `parse_document` selects the instantiation once
per document, so the padded build compiles with no extra branch (verified: same
instruction count as before). The only places that over-read for unpadded input
turned out to be:

- string unescaping (`parse_string`) — handled with a bounds-safe finisher;
- floats — `is_made_of_eight_digits_fast` reads 8 bytes, so a number whose digits
  reach the final bytes over-reads; handled by parsing near-the-end numbers from a
  space-padded copy of the tail (like `visit_root_number`);
- malformed/truncated non-root atoms at the buffer end — handled with the
  length-aware atom validators.

Thorough testing (exact-size heap buffers under AddressSanitizer + a randomized
fuzz) was essential: the float and atom over-reads were **not** caught by
string-only coverage.

## Why On-Demand is harder than DOM

The DOM "free lunch" (compile-time gating at one chokepoint) does **not** transfer:

- On-Demand parsing is **lazy** and **interleaved with user code**. Values are
  parsed when the user calls `get_double()`, `get_string()`, iterates, or calls
  `find_field`/`operator[]`. There is no single chokepoint.
- The over-read-prone code is spread across the non-templated class API
  (`value`, `array`, `object`, `document`, `value_iterator`, `json_iterator`).
  Templating all of it on `UNPADDED` is impractical.
- The "unpadded" state must persist for the **document's whole lifetime**, not for
  one call.

## Survey of On-Demand read sites

Already safe (no work needed):

- **Stage 1** — identical to DOM; tail-copies the last block.
- **Root scalars** — already parsed bounded: `peek_root_length()` +
  `json_iterator::copy_to_buffer(json, max_len, tmpbuf)` and length-aware checks
  (`numberparsing::check_if_integer(json, max_len)`,
  `raw_json_string_is_quote_terminated(json, max_len)`). The bounded-primitive
  pattern we need already exists in the code.

Over-read sites for unpadded input (the work), with file references:

1. **Non-root numbers** — `value_iterator-inl.h` (~567–621):
   `numberparsing::parse_double/parse_unsigned/parse_integer/parse_number(
   peek_non_root_scalar(...))` have no length bound (the float 8-byte over-read).
2. **String unescaping** — `get_string` / `raw_json_string::unescape` →
   `json_iterator::unescape` → shared `parse_string`. The quote-*termination*
   check is already bounded (`raw_json_string_is_quote_terminated(json, max_len)`);
   the unescape pass is not.
3. **Key matching** — `raw_json_string::unsafe_is_equal(std::string_view)`
   (`raw_json_string-inl.h:74`) reads `raw()[target.size()]` and
   `memcmp(raw(), target.data(), target.size())`. **A new class of over-read not
   present in DOM**: `find_field`/`operator[]` with a search key longer than a
   near-the-end document key reads past the buffer.
4. **Atoms** — `is_null`/true/false; mirror the DOM fix (length-aware validators).
5. **To audit** — `peek`/`advance`/document-end & trailing-content scans;
   `get<T>` reflection paths; `document_stream`/`parse_many`.

## Design options (the decision to make)

- **(A) Copy into a padded buffer at `iterate_unpadded`.** Copy the input once
  into the parser's internal buffer, then iterate normally. Zero parsing-code
  changes, zero padded-path penalty, lowest risk. Cost: an `O(n)` copy that
  touches the whole input — which partly defeats On-Demand's "don't read what you
  don't use" benefit.
- **(B) True in-place + a runtime `unpadded` flag.** Store a flag on
  `json_iterator`; when set, route the non-root scalar/string/key paths through
  the bounded primitives that root scalars already use. No copy. Cost: a small
  per-access predicted branch on the padded path — i.e. a non-zero
  instruction-count penalty (the same kind we eliminated for DOM, but here it
  cannot be removed without (C)).
- **(C) In-place + compile-time gating.** Zero penalty, but requires templating
  the entire On-Demand class API on `UNPADDED`. Impractical.

**Open question for discussion:** for On-Demand, do we accept a one-time `O(n)`
copy (A) to keep the padded path byte-for-byte unchanged, or a small per-access
runtime cost (B) to stay copy-free? DOM's zero-penalty approach (C) is not
realistically available here.

## Proposed execution plan (assuming option B, in place)

1. **Entry point & flag.** Add `parser::iterate_unpadded(...)` overloads that skip
   `has_sufficient_padding()` and set a `bool unpadded` on the `json_iterator`
   (persisted for the document's lifetime).
2. **Non-root numbers.** When `unpadded`, route the non-root number getters
   through the bounded path root scalars already use (`copy_to_buffer` into a
   stack `tmpbuf`, parse from there). Reuses proven code.
3. **String unescape.** When `unpadded`, give the unescape path the buffer end and
   finish with a bounds-safe routine — reuse `parse_string_safe` (factor it into a
   shared location, or mirror it).
4. **Key matching.** Add a length-bounded `unsafe_is_equal` variant (the max
   readable length is known from `remaining_input_length`/`peek_length`); use it
   when `unpadded`. This is the genuinely new piece.
5. **Atoms.** Mirror the DOM fix: length-aware validators when `unpadded`.
6. **Audit the rest** — peek/advance/document-end/trailing scans; defer
   `document_stream`/`parse_many` and `get<T>` reflection if possible.
7. **Tests.** New ASAN test on the DOM lesson: parse from **exact-size heap
   buffers** and exercise every *access* pattern near the end —
   `get_double/uint64/int64/string/bool/is_null`, array/object iteration, and
   especially `find_field`/`operator[]` with keys shorter, equal, and **longer**
   than the document's near-the-end keys; plus a randomized fuzz comparing
   `iterate_unpadded` against padded `iterate`. The DOM experience shows the test
   must explicitly hit numbers, atoms, strings **and keys**.
8. **Performance.** Extend an On-Demand benchmark with an unpadded flag (like the
   DOM `parse -u`) to quantify the option-(B) penalty and inform the A-vs-B call.

## Risks

- On-Demand has more read sites and more `SIMDJSON_PADDING` assumptions than DOM
  (the survey already found key-matching as a brand-new one); expect the
  exact-buffer ASAN fuzz to surface more, as it did for DOM.
- The option-(B) runtime-flag penalty is unavoidable without the impractical (C).
- `document_stream`/`parse_many` and the C++26 reflection `get<T>` paths add
  scope; propose deferring them.
