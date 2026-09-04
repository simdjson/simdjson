# Core security audit

## Audit target

- Branch: `francisco/cyber_security`
- Upstream baseline: `origin/master` at `afd7d10935dade64e8a94859a34bbc636540a58f`
- Baseline date: 2026-09-03
- Scope: stage 1 and stage 2 parsing, DOM and On-Demand APIs, padded and
  unpadded inputs, document streams and their worker threads, JSON
  Pointer/JSONPath traversal, allocation arithmetic, memory-mapped inputs, and
  serialization/resource-exhaustion behavior.

This is a source and dynamic security review, not a proof of correctness. It
focuses on memory safety, integer overflow, lifetime/threading hazards, parser
integrity, and denial of service when input or query strings are attacker
controlled.

## Executive summary

The review found six High-severity issues, eight Medium-severity issues, and
three Low-severity hardening defects in the audited scope. The branch fixes all
findings in the table below and adds focused regressions for the reproducible
findings plus sanitizer and fuzz coverage. The highest-risk defects were
dangling ownership in lazy parsing inputs, unsafe active stream moves,
zero-depth parser stack writes, and streaming views whose length could
underflow after the first batch.

No known direct code-execution primitive was demonstrated. Several defects do
permit out-of-bounds access or use-after-free and should therefore be treated as
security vulnerabilities even where the observed reproducer only crashes.
The ratings describe impact after the vulnerable API path is reached, not pure
remote reachability from JSON bytes alone: SJ-SEC-03 requires a temporary-string
construction pattern, SJ-SEC-04 requires moving an active threaded stream, and
SJ-SEC-05/06 require dangerous caller-supplied parser configuration values.
SJ-SEC-16 requires passing a temporary input owner or invoking a lazy stream
API on a temporary parser, and SJ-SEC-17 requires an input at the approximately
4 GiB maximum supported size.

## Fixed findings

| ID | Severity | Area | Impact | Resolution |
|---|---|---|---|---|
| SJ-SEC-01 | Medium | fallback UTF-8 / `parse_unpadded` | Truncated 2-, 3-, or 4-byte UTF-8 at an exact buffer boundary caused a bounded read beyond `buf + len`. | Check remaining bytes before every continuation-byte access; add guard-page, sanitizer, and differential fuzz coverage. |
| SJ-SEC-02 | High | On-Demand `document_stream::iterator::source()` | After the first batch, a relative structural offset was mixed with an absolute document offset. Unsigned length underflow created a view far beyond the input and its trailing-space scan could immediately read out of bounds. | Add `batch_start` to the next structural offset and verify scalar source text and bounds over many batches. |
| SJ-SEC-03 | High | C++17 `padded_input` string lifetime | Constructing from a temporary `basic_string` could select a non-owning view, leaving a dangling pointer immediately after construction. | Add owning rvalue overloads for `std::string`, const temporaries, and allocator-specialized strings; retain zero-copy behavior for safe lvalues. |
| SJ-SEC-04 | High | threaded DOM and On-Demand stream moves | Default moves transferred a live worker whose owner/parser pointers still referenced the moved-from object, creating a use-after-move/use-after-free race. | Quiesce both workers, then transfer all state with custom move operations. Regress move construction and assignment over an already-active destination. |
| SJ-SEC-05 | High | parser `max_depth == 0` | Zero-length stage-2 stacks were accepted and subsequently indexed at element zero, permitting a heap out-of-bounds write. | Reject zero and unrepresentable depths before mutation/allocation in DOM and On-Demand paths. |
| SJ-SEC-06 | High (32-bit) | parser allocation arithmetic | Structural-index roundup/addition and byte-count arithmetic could wrap on 32-bit targets, allowing a recorded capacity larger than its backing allocation. | Validate roundup, sentinel addition, element counts, and stack element sizes before allocation. Add conditional 32-bit regressions. |
| SJ-SEC-07 | Medium (32-bit POSIX with wider `off_t`) | `padded_memory_map` | Converting a large `off_t` to `size_t`, then adding padding, could truncate or wrap mapping lengths and unsafe fixed-map addresses. | Reject non-positive sizes and any file larger than `SIZE_MAX - SIMDJSON_PADDING` before conversion. |
| SJ-SEC-08 | Medium | DOM/On-Demand path recursion | Attacker-controlled deeply nested JSON Pointer/JSONPath queries could exhaust the thread stack when parser depth was raised. | Make DOM pointer and wildcard traversal iterative. Bound the inherently recursive single-pass On-Demand traversal to 64 segments and document `DEPTH_ERROR`. |
| SJ-SEC-09 | Medium | streaming scalar boundaries | An unquoted scalar touching the end of a partial window was treated as complete, so values such as `100007` could be emitted as `1`. | Defer ambiguous root scalars to the next window while preserving incomplete-container and unclosed-string boundary handling. |
| SJ-SEC-10 | Medium | On-Demand wildcard array selectors | Decimal index accumulation wrapped `size_t`, allowing a very large selector to address an unintended small index. | Use checked multiply/add and return `INDEX_OUT_OF_BOUNDS`. |
| SJ-SEC-11 | Medium | JSON Pointer validation | On-Demand dispatch indexed empty null-data `string_view` inputs, while trailing/invalid `~` escapes could be read past their logical token or hidden behind a failed prefix. | Handle empty pointers before indexing and validate every RFC 6901 token up front. DOM token comparison now avoids allocation. |
| SJ-SEC-12 | Medium | On-Demand parser allocation failure | The string buffer and development position stack were used without checking whether their nothrow allocations succeeded. | Return `MEMALLOC`, clean partial state, and leave capacity invalid on failure. |
| SJ-SEC-13 | Low | `padded_string(size_t)` failure state | Allocation failure left a null data pointer paired with the requested nonzero logical size. | Reset logical size to zero when allocation fails. |
| SJ-SEC-14 | Low | stream worker creation | `std::thread` resource failure escaped a `noexcept` path and terminated the process. | When language exceptions are available, catch creation failure and fall back to synchronous streaming, independently of simdjson's throwing-API mode. True `-fno-exceptions` builds preserve historical threading but cannot recover a constructor failure. |
| SJ-SEC-15 | Low | stage-2 safe string parsing | Bounds logic formed a pointer beyond the permitted one-past-the-end value before comparing it, which is undefined behavior. | Compare pointer distance before advancing. |
| SJ-SEC-16 | High | lazy DOM/On-Demand object lifetime | Stream calls and On-Demand document calls could accept temporary strings, padded strings, result owners, padded inputs, or temporary memory maps. Stream methods also accepted a temporary parser receiver, and iterators could be extracted from temporary streams/results. The returned lazy object retained a pointer/view after its owner was destroyed, enabling use-after-free. | Delete unsafe input rvalue overloads; ref-qualify owner-to-view, stream-producing, and iterator-producing methods; and add compile-time coverage for input, parser, and stream value categories. |
| SJ-SEC-17 | Medium | stage-1 maximum-size arithmetic | At the approximately 4 GiB supported limit, a trailing escape could wrap the fallback scanner's cursor and make an unclosed string appear complete. Maximum structural counts also wrapped sentinel indexes, while more than `INT_MAX` unmatched delimiters overflowed signed nesting counters. These could corrupt parser bookkeeping or invoke undefined behavior. | Use `size_t` for scanner/sentinel arithmetic, explicitly bound escape advancement, widen nesting counters, and narrow only proven in-range offsets. |

The stricter whole-pointer validation intentionally changes error precedence for
malformed unreachable suffixes: for example, `/missing/~2` now reports
`INVALID_JSON_POINTER` instead of stopping at `NO_SUCH_FIELD`.

## Defense-in-depth additions

- Added `fuzz_unpadded`, which gives each supported runtime implementation an
  exact-size input and compares its result with padded parsing. The oracle uses
  `abort()` so Release builds retain it.
- Added failed-state, overflow, active-worker move, exact batch-boundary,
  malformed-path, deep-path, and guard-page regressions. Allocation-failure
  injection was not available for every nothrow allocation site.
- Added compile-time lifetime tests covering lazy DOM/On-Demand document and
  stream APIs, alternate stream formats, raw iteration, padded owners, result
  wrappers, mapped-file views, temporary parser receivers, and temporary stream
  iterator extraction.
- Regenerated the distributed single-header sources and archive so fixes are not
  limited to modular builds.

## Residual risks

These items were confirmed or identified during review but are not patched in
this branch because they are broader API/resource-policy decisions rather than
unconditional memory corruptions at supported defaults:

1. **Serialization recursion at extreme configured depth (Low).** DOM
   serialization and parts of `FracturedJson` recurse with document nesting.
   The default parser depth is bounded, but an application that deliberately
   raises it to hundreds of thousands can exhaust its stack while serializing.
   Applications accepting untrusted JSON should retain a conservative maximum
   depth or serialize on a bounded worker process.
2. **Pretty-print output amplification (Low/Medium).** Deep input formatted by
   `FracturedJson` can produce quadratic indentation/output growth. Callers
   should cap input depth and output size when formatting untrusted documents.
3. **Allocation failure in `noexcept` convenience APIs (Low).** JSONPath
   conversion and wildcard result collection allocate standard strings/vectors
   inside `noexcept` APIs. Severe memory pressure can therefore terminate rather
   than return `MEMALLOC`. Removing this behavior requires an API-wide error and
   allocator design change.
4. **Non-owning padded views remain contract-sensitive (Informational).** APIs
   that explicitly accept `padded_string_view` still rely on the caller keeping
   the storage alive and genuinely padded. Direct temporary-owner conversion
   traps are rejected, but explicit views and lvalue-backed views cannot be made
   lifetime-safe by the parser.
5. **On-Demand query compatibility (Informational).** The new 64-segment query
   limit is independent of parser `max_depth`. Queries at the limit succeed;
   longer recursive Pointer/Path queries return `DEPTH_ERROR`.

## Validation

Completed on the macOS audit host:

- native arm64 Clang 22 AddressSanitizer + UndefinedBehaviorSanitizer: all 112
  applicable CTest tests passed with threads enabled;
- independent x86_64 AppleClang UndefinedBehaviorSanitizer: all 121 CTest tests
  passed, including the x86 implementation-selection negative test;
- focused guard-page, exact-EOF, allocation-boundary, active-worker move,
  stream-boundary, malformed/deep query, and ownership regressions passed;
- `fuzz_unpadded` seed smoke tests passed under both sanitizer configurations;
- exception-interface-disabled and true `-fno-exceptions` threaded stream
  smoke tests passed;
- the regenerated single-header sources compiled with both sanitizers, and the
  demo parsed the Twitter corpus and streamed all 793 Amazon documents;
- `git diff --check` passed.

The arm64 cross-target initially registered one x86-only implementation-
selection test because CMake itself was hosted under Rosetta; that inapplicable
test was excluded from the 112-test arm64 result and passed normally in the
x86_64 build.

Platform and campaign limitations: the 32-bit arithmetic and `off_t`
regressions are conditional and could not execute on this 64-bit host. There
was no dynamic allocation of an approximately 4 GiB buffer for the fallback
cursor-boundary regression; reaching it also requires the fallback
implementation and successful very-large parser allocations. That fix was
reviewed statically and compiled.
There was no injected On-Demand allocator failure or `std::thread` constructor
failure. The active-worker move tests are timing stress tests, and no
ThreadSanitizer run was completed. There was also no Windows/MSVC dynamic run
or long-duration coverage-guided fuzz campaign; fuzzing here was a sanitizer
seed smoke test.
