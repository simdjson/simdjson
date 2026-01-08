# RVV Backend Development Plan for simdjson (RISC-V Vector v1.0)

## 1\. Required Files and Folder Structure

To integrate the RVV backend, we will add new files under an organized directory and update a few existing ones. Below is the complete file list with an optimized structure:

**include/simdjson/rv64/ (new directory for RISC-V 64-bit Vector backend):**

| File | Description |
| :---- | :---- |
| base.h | RV64 base definitions (namespace simdjson::rv64 and forward declarations of simd8\<T\> and simd8x64\<T\> similar to other architectures). |
| intrinsics.h | Inline wrapper for RVV intrinsics (includes \<riscv\_vector.h\>). Defines vector types (e.g. vuint8m1\_t etc.), vector load/store operations, and basic arithmetic/bitwise intrinsics for 8-bit lanes. |
| bitmanipulation.h | RVV-specific bit manipulation routines if needed. This may include logic to handle odd-length backslash sequences for escape detection and any prefix-sum or carry propagation operations using RVV (or falling back to 64-bit scalar masks for complexity). |
| bitmask.h | Utilities for mask handling and classification on RVV. For example, defines methods to compute classification masks (whitespace vs structural vs letters) using vector compares or table lookups, and possibly to generate the **quote/escape masks**. |
| numberparsing\_defs.h | Number parsing constants/macros specific to RVV (if any). Likely minimal; may reuse generic number parsing, but ensure any endianness or alignment assumptions are handled. |
| simd.h | Implementation of the simd8\<T\> and simd8x64\<T\> templates for RVV. Defines the vector types for 8-bit elements and the aggregate type spanning 64 bytes. Provides methods like load(), store(), operator==, bitwise ops (&, |, ^), splat(value), etc., using RVV intrinsics. Also implements simd8x64\<T\>::to\_bitmask() (to produce a 64-bit mask from a 64-byte chunk), reduce\_or(), and compress(mask, output) for filtering bytes[\[1\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=simd8,return%2064)[\[2\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask). |
| begin.h | Sets up the RV64 implementation include environment. For example: \#define SIMDJSON\_IMPLEMENTATION rv64, includes rv64/base.h and rv64/intrinsics.h. If runtime dispatch is enabled, wrap the following includes in a target-specific pragma (e.g., use SIMDJSON\_TARGET\_REGION("v") or appropriate attributes if supported) so that RVV instructions are isolated[\[3\]](file://file_00000000c3b471fd90c59e3c51c9d0c0#:~:text=%22content%22%3A%20%22,include%20%5C%22simdjson%2Fwestmere%2Fsimd). Then includes the core algorithm headers: rv64/bitmanipulation.h, rv64/bitmask.h, rv64/numberparsing\_defs.h, rv64/simd.h. |
| end.h | Ends the target-specific region started in begin.h (e.g., SIMDJSON\_TARGET\_REGION\_END if applicable) and concludes the inclusion of RV64 headers. |

**Top-level include files:**

* include/simdjson/rv64.h: Master header for the RV64 backend. Similar to other architectures’ headers (e.g. westmere.h), it includes rv64/begin.h, then the generic amalgamated algorithms, then rv64/end.h. This ensures the RVV backend is compiled with the generic stage 1 & 2 logic in between.

* include/simdjson/rv64/implementation.h: Defines the simdjson::rv64::implementation class (final subclass of simdjson::implementation). The constructor sets the name to "rv64" and description to "RISC-V RV64 (Vector v1.0)", and specifies the required ISA bits (we will introduce a new flag for the vector extension in internal::instruction\_set)[\[4\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=SIMDJSON_TARGET_WESTMERE%5Cnnamespace%20simdjson%20%7B%5Cnnamespace%20westmere%20%7B%5Cn%5Cn%2F,size_t%20max_length)[\[5\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=,size_t%20max_length). It declares overrides for create\_dom\_parser\_implementation(), minify(), and validate\_utf8() consistent with other backends. (These will be defined out-of-line, possibly in a source file or immediately after the class definition for header-only inclusion.)

**src/ updates:**

* We will add an RV64-specific source (if not header-only). For consistency and minimal diff, much of the RVV logic will be inline in headers (simdjson tends to include implementations in headers for each architecture). However, we may define non-inline heavy functions in a source if needed:

* src/rv64/implementation.cpp (optional): Contains the definitions of rv64::implementation::{create\_dom\_parser\_implementation, minify, validate\_utf8} if we choose not to inline them. This would allocate an rv64::dom\_parser\_implementation (which uses our RVV Stage 1\) in create\_dom\_parser\_implementation(), and possibly provide RVV-optimized implementations for minify and validate\_utf8 (or call the generic ones). If we keep them header-only (for single-header builds), this .cpp may be unnecessary.

* **Parser implementation class:** We will add an rv64::dom\_parser\_implementation class (likely in include/simdjson/rv64/dom\_parser\_implementation.h or directly in the implementation class file) that inherits simdjson::internal::dom\_parser\_implementation. It will override the Stage 1 routine to use RVV instructions. This may not require a separate file – it can be defined inline similarly to fallback’s in the headers – but we list it for completeness.

**tests/ additions:**

We will extend simdjson’s test suite to cover the RVV backend:
\- New unit tests in tests/ (or extending existing ones) to verify that the RV64 implementation produces correct results. For example, tests for Stage 1 can feed known JSON strings and check that the set of structural indices found by simdjson::get\_active\_implementation() (forced to RV64) matches those found by fallback. We will also test edge cases: JSON with unescaped quotes and backslashes at boundaries, multi-chunk inputs, and non-ASCII characters (to validate UTF-8 handling). These tests might reside in an architecture-general test file (guarded by \#if SIMDJSON\_IMPLEMENTATION\_RV64) or a dedicated rv64\_tests.cpp.
\- **Compilation tests:** Ensure that defining SIMDJSON\_IMPLEMENTATION\_RV64=1 and compiling on a compiler with RISC-V vector support passes all existing tests (this can be part of CI).

**tools/ additions:**

* **Cross-build script:** Add scripts to help developers build and run tests for RV64. For example, tools/build-rv64.sh to cross-compile the library and tests with a RISC-V toolchain (both GCC and Clang variants). This script would set \-DRISCV64=ON or similar CMake flags, and specify \-DCMAKE\_C\_COMPILER=riscv64-linux-gnu-gcc etc., and include the \-march flags for vector extension.

* **QEMU run script:** A script like tools/run\_rv64\_tests.sh to run the test suite under QEMU user-mode. It might build the tests statically (or with an appropriate sysroot) and then invoke qemu-riscv64 with the correct CPU flags to enable the vector extension (e.g. \-cpu rv64,v=true,vlen=128 or similar, depending on QEMU’s syntax). This script ensures easy reproduction of test results on RV64 without physical hardware.

* **Documentation in tools:** A short README (or updates to TOOLS.md if exists) describing how to use the above scripts, how to install QEMU and a cross-compiler, etc.

**Existing file modifications (integration):**

* include/simdjson/internal/instruction\_set.h: Add detection macros for RISC-V. For example:

* \#ifndef SIMDJSON\_IS\_RISCV64
  \#if defined(\_\_riscv) && \_\_riscv\_xlen \== 64
  \#define SIMDJSON\_IS\_RISCV64 1
  \#else
  \#define SIMDJSON\_IS\_RISCV64 0
  \#endif
  \#endif

* And define SIMDJSON\_IMPLEMENTATION\_RV64 default based on compiler support for vectors: e.g., \#define SIMDJSON\_IMPLEMENTATION\_RV64 (SIMDJSON\_IS\_RISCV64 && defined(\_\_riscv\_vector)). We will also introduce a flag in internal::instruction\_set enum, e.g. RVV, to represent the RISC-V Vector ISA support[\[5\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=,size_t%20max_length).

* include/simdjson/implementation\_selector.h (or wherever the default implementations are chosen): Insert logic to include our rv64.h when appropriate. The build system will set SIMDJSON\_IMPLEMENTATION\_RV64=1 by default on RISC-V64 if vector intrinsics are available. Then similar to other architectures, our rv64.h will be conditionally included. We also update the runtime dispatch ordering: on a RISC-V build, if RVV is present at runtime, use rv64 implementation; otherwise use fallback. (We do *not* remove fallback by default, since not all RISC-V systems have V extension. Fallback remains enabled unless the user explicitly disables it.)

Overall, the file layout mirrors existing architecture backends (e.g., westmere, haswell, arm64, etc.), ensuring the RVV backend is neatly integrated with minimal disruption to upstream structure.

## 2\. Core Concepts and Naming Conventions

To maintain consistency and clarity in the RVV backend, we establish fixed terminology and variable naming across the code:

* **VLEN:** *Vector register length (in bits)*. In RISC-V, VLEN can vary per CPU, but our code treats it as the hardware maximum bit-width of a vector register (e.g., 128, 256, 512 bits). We will use VLEN in comments and variable names to refer to the runtime-chosen vector length. For example, current\_vl\_bits might store the actual bits set by vsetvl.

* **LMUL:** *Vector Length Multiplier*. This is the RISC-V concept of grouping multiple registers to form a wider logical vector. We will use LMUL to denote the grouping factor used to process 64 bytes. In our implementation, we choose an LMUL such that LMUL \* (VLEN in bytes) \= 64 whenever possible. For instance, on a 128-bit (16-byte) VLEN CPU, we use LMUL=4 to get a 64-byte chunk; on 256-bit (32-byte) VLEN, use LMUL=2; on 512-bit (64-byte) VLEN, LMUL=1. This approach ensures we **always handle 64 bytes per chunk** (the simdjson batch size) regardless of hardware VLEN. We will document this choice clearly and enforce it in code (e.g., by setting VL to 64 elements of 8-bit and appropriate LMUL). Variable names like rvv\_lmul or constants like RVV\_LMUL\_CHUNK will be used to convey this.

* **CHUNK\_SIZE:** The number of bytes processed in one vectorized step. In simdjson’s design, this is 64 bytes for all SIMD backends[\[6\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Unlike%20scalar%20parsers%2C%20which%20process,sized%20chunks%20%28e.g.%2C%20256%20bits)[\[7\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a). We keep CHUNK\_SIZE \= 64 as a constant (likely already defined in generic code). All our loops and algorithms will operate on 64-byte blocks, matching the existing Stage 1 pipeline which processes input in fixed-size batches[\[8\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=The%20SIMD%20JSON%20algorithm%20consists,of%20two%20main%20stages). We ensure to handle inputs not divisible by 64, including proper handling of the tail bytes (the generic loop already takes care of padding or partial chunk logic).

* **simd8\<T\> and simd8x64\<T\>:** We continue the convention that simd8\<uint8\_t\> represents a SIMD register holding multiple 8-bit elements (the number of elements depends on VLEN for RVV, but e.g. 16 elements on a 128-bit vector). simd8x64\<uint8\_t\> represents the composite type for a 64-byte chunk, possibly consisting of multiple RVV registers (if VLEN \< 512 bits). In our RVV implementation, we will implement simd8x64 such that it contains NUM\_CHUNKS registers of type simd8\<uint8\_t\>, where NUM\_CHUNKS \= 64 / sizeof(simd8\<uint8\_t\>). For example, if one simd8\<uint8\_t\> covers 16 bytes (128-bit VLEN), then NUM\_CHUNKS=4 (matching SSE/Neon patterns)[\[9\]\[10\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=%28__m128i%29vec_splat_u8%28N%29%29%29%3B%5Cn%20%20%7D%5Cn%7D%3B%5Cn%5Cntemplate%20,T). If one register covers 32 bytes, NUM\_CHUNKS=2. We will static\_assert that the product equals 64 to avoid mistakes. We use consistent naming internally like chunks\[i\] for these sub-registers, and methods like to\_bitmask() will combine all sub-register masks (shifting and ORing appropriately, as seen in other backends)[\[11\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=output%20%2B%2032%20,T%3E%3A%3Asplat%28m%29%3B%5Cn%20%20%20%20return)[\[12\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,uint64_t%20r1).

* **Quote mask and string tracking:** We adopt the existing terminology for handling strings in Stage 1\. Specifically, we define:

* **quote\_bits / Q**: a 64-bit mask with 1s at positions of the " character in the current chunk[\[13\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=We%20then%20calculate%20a%20mask,in%20the%20data%20block)[\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201).

* **backslash\_bits / B**: a 64-bit mask with 1s at positions of the \\ character in the chunk[\[7\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a).

* **OD (odd-length delimiter mask)**: a mask indicating positions of quotes that are preceded by an *odd* number of backslashes (i.e., quotes that are **escaped**). We will use a name like odd\_backslash\_mask or escape\_mask. The computation of this follows the paper’s algorithm: we find sequences of backslashes and mark the end of sequences with odd length[\[15\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Next%2C%20we%20perform%20the%20following,end%20with%20an%20odd%20length). We may implement this by vector operations or by leveraging the existing approach with carry propagation on a 64-bit mask (for simplicity, since this is a scalar bit-dependent operation). The variable naming will clearly distinguish this mask (e.g., odd\_seq\_mask).

* **string\_mask / R** (often called **quote mask** in simdjson context): a 64-bit mask where each 1 indicates that the corresponding byte is inside a string (between an opening quote and a closing quote)[\[16\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=1s%29)[\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201). We will typically call this in\_string\_mask. This is computed by taking the XOR-prefix of the valid quote positions: in other words, starting from 0 (outside a string), flip the state at each unescaped quote. In code, we might implement this with a carry-less multiply (vpclmul on x86, but on RVV we might do a horizontal prefix XOR via a loop or using the intrinsic vcpop/count operations on 64-bit mask). To keep it simple and correct, we will likely compute R by a **scalar** 64-bit prefix XOR on the quote\_bits & \~OD mask, as the cost is negligible and clarity is high. The naming will remain consistent (e.g., R in comments, and a variable string\_mask in code).

* We ensure any variables related to quote handling (like prev\_in\_string state across chunks, prev\_backslash\_bits for carry-over) have consistent names and semantics. For instance, a boolean or mask prev\_in\_string indicates whether the previous chunk ended inside an open string (to initialize the next chunk’s string mask calculation), and an integer prev\_backslash\_count (mod 2\) might carry over whether there was a trailing backslash run at end of chunk. These names will match usage in other parts of simdjson (if the generic pipeline already uses e.g. prev\_scalar and similar, we add prev\_in\_string accordingly).

By defining these core concepts explicitly, we ensure the RVV backend’s code is easier to read and review. All contributors can refer to **LMUL**, **VLEN**, **CHUNK\_SIZE**, etc., with the same understanding, and the code will use these terms uniformly in comments and identifiers. This consistency also minimizes diffs against upstream, as we mirror existing patterns (e.g., using 64-byte chunks, similar mask naming, and existing simdjson conventions for variables like idx, prev\_scalar, etc.).

## 3\. Implementation Responsibilities

### 3.1 Primitives: Intrinsics and SIMD Wrapper Implementation

**Goal:** Implement the low-level vector primitives for RVV so that higher-level stage logic can be written in a platform-agnostic way (using simd8\<T\> methods, etc.). This involves creating the RVV equivalents of fundamental operations (load, compare, bitmask extraction, etc.) and ensuring they conform to simdjson’s interface.

* **Vector load/store:** Implement simd8\<T\>::load(const T\* ptr) and store(T\* ptr) using RVV intrinsics. For example, simd8\<uint8\_t\>::load will use vle8\_v\_u8m1 or vle8\_v\_u8m4 depending on LMUL (with VL set to cover all elements). We ensure alignment assumptions are met (the input is padded to 64 bytes, so unaligned loads are acceptable). Similarly, store writes out the vector content (mostly used in tests or if needed for transitions; might not be heavily used in stage1).

* **Comparisons:** Provide methods like simd8\<T\>::operator==(simd8\<T\> other) yielding a mask (represented as simd8\<bool\>). In RVV, we will use vector compare instructions (vmseq for equality, vmslt/vmsle for less-than, etc.). The result is an RVV mask register (type vbool8\_t or similar). We will likely represent simd8\<bool\> internally as the mask type or as a vector of 8-bit booleans (where 0xFF \= true, 0x00 \= false for each lane) for compatibility with simdjson’s expectation that a “bool vector” can still be treated as a vector. For example, other backends use the same simd8\<bool\> type for masks (SSE uses 0xFF bytes)[\[17\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=__lsx_vmin_b%28,n). We can follow that model: after a compare, convert the RVV mask to a full 8-bit vector (using instructions like vmerge or storing the mask to a uint8\_t array). This allows reuse of methods like to\_bitmask().

* **Bitwise operations:** Implement bitwise AND, OR, XOR, and NOT for simd8\<T\>. RVV intrinsics provide these (e.g., vand\_vv\_u8m1, etc.). These should be straightforward. We ensure the operations work for T=uint8\_t and also for T=bool variant if we use the mask-as-bool-vector approach. (Likely we treat simd8\<bool\> as a specialization with underlying mask type, but support bitwise ops on it as well to combine conditions).

* **Arithmetic if needed:** Stage 1 primarily uses comparisons and bitwise ops, not general arithmetic on 8-bit values, except possibly subtraction for ASCII values or addition for UTF-8 checks. We implement any needed arithmetic (like adding 1 to a vector or saturating subtract) using RVV (e.g., vadd\_vx). For UTF-8 validation, methods like prev\<1\>() (to get the vector shifted by one byte from previous chunk) will be needed – we implement these via RVV slide or shift instructions (vslideup or vrgather to simulate shifting in a previous vector’s last element).

* **Mask to bitmask conversion:** A key primitive is converting a vector comparison result into a 64-bit bitmask. We implement simd8x64\<uint8\_t\>::to\_bitmask() which returns a 64-bit where each bit corresponds to a byte lane of the 64-byte chunk[\[2\]\[18\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask). Our strategy: after computing a condition (say, vec \== '"'), we obtain simd8\<bool\> that spans 64 bytes (which internally might be 1–4 registers). We can use RVV’s intrinsic to compress a mask to bits. RVV 1.0 provides instructions like vmsbf (vector mask to bitmask) but that yields a mask length bit-count equal to VL, which could be up to 64 bits if VL=64. If our chunk is processed in multiple registers, we can compute partial masks per register and then combine:

* Compute mask for each sub-chunk (e.g., 16 bytes) using vpopc (population count on mask) and shifts, or use the approach from other backends: each simd8\<T\> (16 bytes) has a to\_bitmask() that gives a 16-bit mask. Then simd8x64::to\_bitmask() concatenates these (shifting r1 by 16, r2 by 32, etc.)[\[2\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask). We will do similarly: e.g., if NUM\_CHUNKS=4, get 4 masks of 16 bits and combine into one 64-bit. On architectures where one register covers 64 bytes (NUM\_CHUNKS=1), it will directly produce the 64-bit mask.
  This bitmask extraction is critical for the performance of finding structural indices, and we will test it thoroughly (comparing results with a scalar loop to ensure correctness).

* **Reduction and compression:** Implement utility methods:

* simd8x64\<T\>::reduce\_or(): ORs together all simd8 chunks in the group[\[19\]\[20\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,count_ones%28mask%20%26%200xFFFFFFFF%29%29%3B%5Cn). This is used in some algorithms (e.g., checking if any byte in 64 bytes is non-zero or has a particular flag). We can implement this by performing OR across the RVV registers (a horizontal OR). For example, OR chunk0 and chunk1 (if available) using vector OR, etc. Since this is only used in fallback of some algorithms, performance isn’t critical but correctness is.

* simd8x64\<uint8\_t\>::compress(uint64\_t mask, uint8\_t \*output): This is used to gather all bytes indicated by a mask (with 1 meaning keep) and pack them sequentially (used in Stage 1 to output the structural characters from 64-byte blocks)[\[21\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=%28this,return%2064)[\[12\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,uint64_t%20r1). We implement this by leveraging RVV’s vector compress instructions if available (there is an instruction vcompress in the spec to pack elements by mask into a destination vector). If the compiler supports an intrinsic for that (e.g., GCC provides \_\_riscv\_vcompress), we can use it directly to handle 64 elements at once. If not, we can implement compress in chunked manner like other backends:

  * Process each 16-byte sub-chunk: compress within each using a smaller vector algorithm (as SSE does, moving matching bytes to front of a 16-byte vector and counting). The existing simdjson code already breaks the 64-bit mask into 4 chunks and compresses each with adjustments[\[22\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20compress%28uint64_t%20mask%2C%20T%20,n). We can mimic that logic using RVV for each sub-block, or even do the entire 64 bytes in one go by loading the bytes into a vector and using mask-driven compress store. Using RVV’s masking and segment moves, we can likely implement a clean compress.

* These primitives will have consistent naming: e.g., we’ll use mask64 as the 64-bit mask input, and careful calculation of offset for each sub-chunk by counting bits (we’ll use RVV’s vcpop to count bits in mask easily for offset calculation, similar to how the existing code uses count\_ones() on portions of the mask[\[23\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,return%2064)).

* **UTF-8 specific helpers:** For UTF-8 validation, our primitives need to support operations on 8-bit data that help detect errors:

* We need a way to detect continuation bytes (0x80–0xBF) vs lead bytes. In other backends, they use operations like signed comparison or lookup tables to classify bytes[\[24\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=is_continuation%28uint8_t%20c%29%20,n)[\[25\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=uint8_t%20CHAR_TYPE_OPERATOR%20%20%3D%201,n%20%200x00%2C%200x00%2C%200x00). We might implement a lookup table approach using RVV: load a 256-byte lookup table into a vector (spread across multiple registers) and use a gather to map input bytes to character classes (space, structural, escape, non-ASCII, etc.)[\[25\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=uint8_t%20CHAR_TYPE_OPERATOR%20%20%3D%201,n%20%200x00%2C%200x00%2C%200x00). Alternatively, we can do logic directly: e.g., is\_ascii \= (byte & 0x80) \== 0, is\_continuation \= (byte & 0xC0) \== 0x80, etc., done in parallel. We’ll implement needed helpers in intrinsics (like a method to check if any byte is \>= 0x80, or to mask high-bit bytes).

* RVV intrinsics allow shifting bytes within vectors. We’ll implement simd8\<uint8\_t\>::prev\<N\>(prev\_vector) which returns the current vector shifted N bytes, taking prev\_vector for the bytes that come from the previous block. This is used in UTF-8 check to line up bytes with their predecessors[\[26\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=bytes%20are%20valid%20UTF,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1)[\[27\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=last%20input%20we%20received%20was,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1). We can use vslideup or manually compose by extract/insert. For N=1 specifically, we can do an efficient blend: e.g., take the last byte of prev\_vector and put it as first of result, shift everything else down by one.

* **Set VL for partial operations:** In some cases (tail processing or adjusting VL), we’ll use the RVV segment load with specific VL. But generally for full 64-byte chunks, we set VL=64 (if hardware supports it with grouping) to process the whole chunk at once.

**Checklist – Primitives Implementation:**

* \[x\] Define RVV vector types and ensure sizeof(simd8\<uint8\_t\>) \* NUM\_CHUNKS \== 64. Use static assertions to verify the chunk grouping logic (e.g., 4x16, 2x32, or 1x64 bytes).

* \[x\] Implement load() and store() for simd8\<T\> (and by extension for simd8x64\<T\> constructor using pointer).

* \[x\] Implement comparison operators (\==, \<, etc.) using RVV intrinsics, returning mask type as simd8\<bool\>. Ensure mask results are compatible with to\_bitmask() (e.g., convert mask register to 8-bit lanes of 0xFF/0x00).

* \[x\] Implement bitwise ops (&, |, ^, \~) for simd8\<T\>.

* \[x\] Implement simd8x64\<T\>::to\_bitmask() to produce a 64-bit mask from up to 4 sub-register masks[\[2\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask). Test this by comparing with a simple loop on sample data.

* \[x\] Implement simd8x64\<T\>::compress(mask, output) by packing bytes where mask bit=1. Use RVV’s mask-driven store if available, or replicate the multi-step compress logic from other backends[\[28\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,n). Verify correctness on various masks (all 1s, sparse 1s, etc.).

* \[x\] Provide simd8\<T\>::splat(value) to broadcast a byte value to all lanes (using vmv\_v\_x\_u8m1). Also provide static methods like simd8\<uint8\_t\>::repeat\_16(...) if needed for lookup table construction (following other arch patterns)[\[29\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=uint8_t%20v7%2C,uint8_t%20v0).

* \[x\] Implement any necessary horizontal operations (e.g., we may not need explicit horizontal add, but if required for UTF-8, we can use reduction intrinsics).

* \[x\] Ensure all these intrinsics are guarded by appropriate \#if \_\_riscv\_vector and that the code compiles with both GCC and Clang. Use the latest intrinsics definitions (GCC 12+, Clang 14+ have stable RVV support).

### 3.2 Stage 1 Logic: Structural Character Detection and UTF-8 Validation

**Goal:** Utilize the primitives to implement Stage 1 – scanning 64-byte blocks to find JSON structural characters and to validate UTF-8 – using RVV vectors. We aim for functional parity with existing backends (westmere/haswell/arm64), producing the same output (an array of structural indices) and error flags.

* **JSON structural classification:** We will identify structural characters ({, }, \[, \], :, ,) and whitespace (\\n, \\r, \\t, space) using vector operations. There are two possible approaches:

* **Multiple vector comparisons:** Compare the 64 bytes to each structural character and OR together the results. E.g., is\_brace \= (in \== '{') | (in \== '}'), similarly for brackets and so on, then OR all to get a mask S\_mask for all structural tokens. Likewise, compare to whitespace characters to get W\_mask. This results in up to \~6 comparisons for structural and 4 for whitespace, which RVV can handle efficiently in parallel. We will likely do this straightforwardly for clarity.

* **Lookup table classification:** Use a 256-byte lookup table (as in scalar fallback[\[25\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=uint8_t%20CHAR_TYPE_OPERATOR%20%20%3D%201,n%20%200x00%2C%200x00%2C%200x00)) where each byte is pre-labeled with bits like 0x01 \= whitespace, 0x02 \= structural, etc. We can load chunks of this table into a vector and use the input bytes as indices (using RVV gather load). For example, produce two masks by testing bits in the looked-up values. This is an elegant approach, but it involves an indexed load per chunk which might be slightly slower on some hardware. However, RVV does support vector indexed loads, and since 64 bytes is not huge, this is feasible.

Given minimal-diff priority, we might mirror what other SIMD backends do: many use direct comparisons (SSE/AVX, since lookup would require PSHUFB which is SSE4.1). LoongArch LSX/LASX likely also did comparisons. We will implement it with comparisons for consistency. The masks we produce will be: \- structural\_mask (64-bit): 1 for any structural char[\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201). \- whitespace\_mask (64-bit): 1 for any whitespace char. We’ll name them S and W (with variables like structural\_bits and whitespace\_bits). They are obtained via vector compares and then to\_bitmask().

* **String detection (quotes and escapes):** Using the methodology described earlier and in the literature[\[30\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Afterward%2C%20we%20calculate%20a%20mask,marked%20by%201s)[\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201):

* Compute quote\_bits \= (in \== '\\"').to\_bitmask(). Use our comparison primitive for '"'.

* Compute backslash\_bits \= (in \== '\\\\').to\_bitmask().

* Compute the mask of *escaped quotes*. We need to find which quote positions have an odd number of backslashes immediately before them. We will leverage the approach from the original simdjson paper pseudocode[\[7\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a)[\[15\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Next%2C%20we%20perform%20the%20following,end%20with%20an%20odd%20length):

  * Compute OD \= calc\_OD(B) where B \= backslash\_bits. In practice, OD (odd-length ends of backslash sequences) can be derived by:

  * uint64\_t even\_runs \= B & (B \<\< 1);       // pairs of backslashes
    even\_runs &= (even\_runs \<\< 1);          // quartets of backslashes, etc.
    // (This alone is not enough; a known approach is a loop or a bit trick with \+ operations.)

  * A simpler method: use a carry mechanism:

  * uint64\_t bs\_bits \= backslash\_bits;
    // mask to identify bits where a run of backslashes ends in an odd count
    uint64\_t odd\_mask \= bs\_bits & \~ (bs\_bits \<\< 1);

  * But the above odd\_mask would mark the *first* backslash of a pair as odd, which is not exactly what we want. Instead, simdjson historically uses a state machine or addition trick. We might do:

  * Compute a prefix XOR of the backslash mask: X \= 0; for each bit i from LSB to MSB: if(B\_i) X ^= 1; if(\!B\_i) leave X; store X in result bit i. This prefix XOR of backslashes yields a mask where 1 means “this position is preceded by an odd number of backslashes”. Shifting that mask by 1 to the *right* would yield OD (the positions of the *following* character that are escaped if it’s a quote). In code, we might do a loop or use a SIMD carry technique (some architectures use \_mm\_maddubs\_epi16 to accumulate bits; we might simply do it in scalar since 64 bits is manageable).

  * Given that we favor correctness and minimal diff over extreme micro-optimization here, we may implement OD by a simple 64-bit loop or Duff’s device: iterate over bits of backslash\_bits counting consecutive ones, and set a 1 in OD for each position that ends a sequence of backslashes of odd length. This is done only once per 64 bytes, which is negligible in overhead and keeps code simple to review. We will call the resulting mask escape\_mask (or keep OD nomenclature in comments) – bits where a quote is preceded by an odd number of backslashes.

* Filter out escaped quotes: valid\_quote\_bits \= quote\_bits & \~escape\_mask. This mask has 1s for actual quote characters that start or end strings[\[13\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=We%20then%20calculate%20a%20mask,in%20the%20data%20block).

* Compute the **string range mask** R by computing the prefix XOR of valid\_quote\_bits. Each 1-bit in R indicates that the corresponding byte is inside a string literal[\[16\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=1s%29)[\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201). As discussed, we will likely do this as a 64-bit scalar XOR sweep (flip a flag every time we hit a 1 in valid\_quote\_bits). This yields the same result as the carry-less multiply (CUMUL) mentioned in the paper[\[31\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=By%20performing%20a%20cumulative%20XOR,R), but with clearer implementation.

* Now we have:

  * S (structural mask of punctuation)

  * W (whitespace mask)

  * R (string content mask)

  * (We also keep Q \= quote\_bits and possibly Q\_mask \= valid\_quote\_bits for later use in pseudo-structural logic.)

* Exclude any structural characters that occur inside strings: S\_filtered \= S & \~R. Likewise, we might exclude whitespace inside strings if needed (though whitespace inside strings is simply treated as data and not structural). Typically, simdjson masks out anything inside a string because it’s not relevant for structure[\[32\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=,Exclude%20whitespace%20inside%20the%20string). Our code will do:

* uint64\_t structurals \= structural\_mask & \~string\_mask;
  uint64\_t whitespace  \= whitespace\_mask & \~string\_mask;

* At this point, structurals has 1s for structural characters that are actual JSON separators.

* **Pseudo-structural characters:** Simdjson uses the concept of “pseudo-structural” positions – basically, the first character of a non-number/value after a string or other value, which should be treated as a structural boundary (this deals with cases like "}true" where the t of true is logically a separator because a value starts immediately after a JSON close without a comma)[\[33\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%28,position%20of%20the%20value%20token). The paper pseudocode computes a mask P for these positions[\[34\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=W%20%20%3D%20classify,space%20mask)[\[35\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Using%20the%20rule%20that%20a,position%20of%20the%20value%20token). We will implement the same rule:

  * pseudo \= ((structurals OR whitespace) \<\< 1\) & \~whitespace & \~string\_mask; This marks any position preceded by either a structural char or whitespace, that is not whitespace or inside a string, as a pseudo-structural start of a value[\[34\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=W%20%20%3D%20classify,space%20mask). We will be careful at chunk boundaries: if a chunk ends with a structural char or whitespace, the next chunk’s first byte might need to be considered. The existing pipeline likely handles this by carrying over a prev\_scalar state (it tracks if previous byte was a non-structural, non-whitespace i.e., part of a value)[\[36\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=Whether%20the%20last%20character%20of,1%20%7C%20overflow%3B%5Cn)[\[37\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=cat.,n). We already have prev\_scalar in json\_scanner for that[\[36\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=Whether%20the%20last%20character%20of,1%20%7C%20overflow%3B%5Cn)[\[37\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=cat.,n). We will integrate with that mechanism: update prev\_scalar for the next iteration by computing nonquote\_scalar \= (\~(S\_mask | W\_mask) & \~Q\_mask) (bytes that are not structural, not whitespace, not a quote boundary)[\[38\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=%5C,n), and then follows\_nonquote\_scalar \= (nonquote\_scalar \<\< 1\) | prev\_scalar as done in the generic scanner[\[39\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=cat.,n). This yields a mask of positions that directly follow a non-quote scalar, which we use to suppress pseudo-structural on letters after a number without a comma, etc. The logic is a bit intricate but we’ll align with the documented approach[\[40\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=except%20structural%20characters%20and%20white,so%20either%20we%20get%20a)[\[41\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nothing,moves%20or%20the%20copy%20is). In summary, we will implement the pseudo-structural detection exactly as in the generic algorithm (likely no changes needed; we just ensure our characters.scalar() and strings.quote() feed that logic).

  * We’ll name our pseudo-structural mask pseudo\_mask and combine it: structurals |= pseudo\_mask; then remove any quote that might still be in structurals (some implementations add quotes then remove them)[\[42\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Next%2C%20we%20combine%20the%20structural,the%20boundaries%20of%20all%20tokens). But since we already filtered quotes out, we likely don’t need to add/remove them explicitly.

* The final structurals mask now has 1 for each position in the chunk that is a true JSON structural character (comma, colon, brace, bracket) *or* a pseudo-structural boundary (beginning of an unquoted value that wasn’t already preceded by a comma/brace). Each of those positions corresponds to an index in the original JSON where a structural element begins.

* **Output index extraction:** With the final mask, we use the compress routine or bit tricks to store these positions into the structural\_indexes array (which collects all structural indices in the full JSON). The existing simdjson Stage 1 uses extract\_indices\_from\_bitset(S, out\_indices) which is essentially what our compress() implements[\[43\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=,marks%20all%20token%20boundary%20points)[\[44\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=are%20marked%20with%201s%20%28e,0001000100100000). We will feed our 64-bit structurals mask into that, writing out the indices (base offset of the chunk \+ bit index of each 1).

* We update any state variables for the next chunk: prev\_in\_string becomes the last bit of R (since if the last byte of the chunk was inside a string, the next chunk continues in-string). Also, if the chunk ended with one or more backslashes, we compute whether that sequence was odd (affecting the next chunk’s first byte if it’s a quote). We can compute trailing\_backslash\_count \= number of trailing 1s in backslash\_bits (popcount of trailing zeros of \~B). If odd, we set a flag carry\_escape \= true to indicate the next chunk’s first quote is escaped. But an easier method: the generic algorithm’s use of prev\_backslash in immediately\_follows suggests they carry a 1-bit mask for “was the last byte of previous chunk a backslash?”[\[45\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=follows%20a%20matching%20character.,n%20%20json_character_block%20characters)[\[46\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=follows%20a%20matching%20character.,uint8_t%3E%26%20in%29%20%7B%5Cn%20%20json_string_block). However, an isolated single backslash at end might not fully determine odd/even if previous chunk ended with multiple. We might instead carry prev\_odd\_backslash \= (trailing\_backslash\_count % 2 \== 1\). We will incorporate that in our OD calculation for the next chunk’s beginning (i.e., initialize the odd/even state with this flag).

* **UTF-8 validation:** We integrate the UTF-8 checker into Stage 1 pipeline (simdjson does this in parallel). Our utf8\_checker (similar to existing ones[\[47\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=max_value%28%26max_array%5Bsizeof%28max_array%29,n)[\[48\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nonzero%2C%20there%20has%20been%20a,1%3E%28prev_input%29%3B%5Cn)) will have:

* A vector accumulator for errors (error), which starts as all zeros (no error). If any check fails, we set bits in error. After processing all chunks, we quickly check if error is non-zero (via reduce\_or) to decide if there was a UTF-8 error.

* Track prev\_input\_block (the last 16/32/64 bytes from previous chunk) and prev\_incomplete (mask of whether the last bytes of previous chunk formed an incomplete multibyte sequence)[\[49\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nonzero%2C%20there%20has%20been%20a,1%3E%28prev_input%29%3B%5Cn).

* For each chunk, perform a vectorized UTF-8 validation:

  * Compute a mask of bytes that are **too large** (\> 0xF4 or in illegal ranges) using range compares – existing algorithms often use lookup tables or large constants as seen in the code[\[50\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=SIMDJSON_IMPLEMENTATION_ICELAKE,uint8_t)[\[51\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=previous%20input%27s%20last%203%20bytes,255%2C%20255%2C%20255%2C%20255). We will replicate that logic: we can set up vectors of thresholds and compare. For instance, identify bytes in \[0x80, 0xC1\] range (which are continuation bytes or overlong markers) and set error if a lead byte is in that range; identify overlong forms like 0xF5–0xFF and set error. This logic is already in simdjson’s check\_special\_cases function (we will port it).

  * Using prev\_input\_block, align it so that for each byte in current block, we can check the byte before it (prev1 \= input.prev\<1\>(prev\_input))[\[26\]\[52\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=bytes%20are%20valid%20UTF,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1), and similarly perhaps prev2 and prev3 for certain checks (some checks need to see two bytes back for sequences). We implement prev\<1\> as described with RVV slides.

  * The algorithm likely does:

  * Determine which bytes are 2-byte leads (0xC0–0xDF), 3-byte leads (0xE0–0xEF), 4-byte leads (0xF0–0xF4).

  * Check that each lead is followed by the appropriate number of continuation bytes. We can do this by shifting masks:

    * Compute mask of positions that are continuation bytes: cont\_mask \= (in & 0xC0) \== 0x80.

    * For each lead, verify the next N bytes are continuation: e.g., for a 2-byte lead at index i, check cont\_mask & (1 \<\< (i+1)) is set, else error. For a 3-byte lead, check i+1 and i+2; for 4-byte lead, check i+1,i+2,i+3.

    * We can use vector compare and shift operations to achieve this: for instance, create a mask lead2\_mask of all 2-byte lead positions, then (lead2\_mask \<\< 1\) & \~cont\_mask will have a 1 if a lead2 is not followed by a continuation. We OR that into error. Similarly, (lead3\_mask \<\< 1\) & \~cont\_mask or (lead3\_mask \<\< 2\) & \~(cont\_mask \<\< 1\) etc. A more direct way: use logical implications – but for simplicity, we might end up doing a few shifts and ANDs on 64-bit masks to catch missing continuations.

    * Also check for **overlong sequences** and **invalid code points**: e.g., 0xE0 followed by \< 0xA0 is overlong, 0xED followed by \> 0x9F is a surrogate, 0xF0 followed by \< 0x90 is overlong, 0xF4 followed by \>= 0x90 is too high[\[53\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=10______,n%20%20if)[\[54\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=%28simdjson_unlikely%28idx%2B3%20,0%3B%5Cnstatic%20const). We incorporate those by specific mask conditions (the code snippet in fallback shows those checks clearly[\[55\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=continuation,idx%2B2)[\[56\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=,nstatic%20const%20uint8_t%20CHAR_TYPE_ESC_ASCII)). For each such rule, we do a vector mask and OR to error.

  * If any byte in error vector becomes non-zero (i.e., any lane has an error flag), we know UTF-8 is invalid. We propagate that.

  * Given the complexity, we plan to reuse as much logic from the generic utf8\_checker as possible, but implemented with our vector ops. The existing utf8\_checker::check\_utf8\_bytes() does a lot of this in vector-friendly way[\[57\]\[27\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=last%20input%20we%20received%20was,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1). We will mirror it:

  * Compute prev1 \= input.shift\_bytes\_right\<1\>(prev\_input) (our prev\<1\>).

  * Then something akin to simd8\<uint8\_t\> sc \= check\_special\_cases(input, prev1) to handle edge conditions (like determining if any 1-byte sequences are incomplete at boundary, etc.). We’ll implement check\_special\_cases for RVV similarly to SSE’s, using constant vectors and compares (the snippet \[18\] shows a method to create a max\_value array and use gt\_bits to catch too-short sequences)[\[50\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=SIMDJSON_IMPLEMENTATION_ICELAKE,uint8_t)[\[58\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=a%204,255%2C%20255%2C%20255%2C%20255%2C%20255).

  * Then combine that with continuation checks. Possibly simpler: after processing all full chunks, call a scalar function on the last few bytes to catch if input ended in the middle of a multi-byte sequence (the generic pipeline does handle this by trimming partial UTF-8 at end).

  * **Integration:** The Stage 1 loop will interleave the UTF-8 checking with structural scanning. In our code, inside the main loop over input:

  * Load 64 bytes into in.

  * Do strings \= string\_scanner.next(in) (which performs the quote/escape logic above, yielding string\_mask)[\[59\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=json_scanner%3A%3Anext%28const%20simd%3A%3Asimd8x64,mark%20the%20%27t%27%20of%20%27true).

  * Do chars \= json\_character\_block::classify(in) (which does the structural/whitespace classification)[\[59\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=json_scanner%3A%3Anext%28const%20simd%3A%3Asimd8x64,mark%20the%20%27t%27%20of%20%27true).

  * Compute structurals \= chars.structural() & \~strings.quote(), etc., as per generic logic[\[38\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=%5C,n).

  * Meanwhile, call utf8\_checker.check\_utf8\_bytes(in, prev\_input) for this chunk to update the UTF-8 error state[\[57\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=last%20input%20we%20received%20was,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1).

  * Append structural indices from structurals mask.

  * Save prev\_input \= in and possibly prev\_incomplete \= utf8\_checker.is\_incomplete(in) for next iteration[\[60\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=,255%2C%20255)[\[47\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=max_value%28%26max_array%5Bsizeof%28max_array%29,n).

  * The UTF-8 validation does not affect the structural processing output, it only sets an error flag if invalid. We will ensure that if utf8\_checker.error becomes non-zero, we mark an error for the parser (so that after Stage 2, simdjson returns UTF8\_ERROR).

* **Stage 1 Output and interface:** Implement the rv64::dom\_parser\_implementation::parse\_chunk(const uint8\_t \*buf, size\_t len, bool partial) method (or similar) that performs the above on each chunk and populates structural\_indexes (and also perhaps writes the string quote mask for stage 2 if needed by on-demand API, though likely not needed beyond Stage 1). This function will be invoked by the higher-level parser. It should closely resemble the structure of fallback or other SIMD implementations’ stage1 functions, but calling our RVV intrinsics. The output is:

* structural\_indexes\[\]: a list of **indices** (positions in input) of all structural characters (and pseudo-structurals).

* n\_structurals: count of these indices.

* error code if something went wrong (e.g., UNCLOSED\_STRING, UTF8\_ERROR). We detect UNCLOSED\_STRING if after processing all chunks prev\_in\_string is true (i.e., we ended inside a string without a closing quote). That should result in an error at the end of Stage 2 typically, but Stage 1 can pre-flag it if in partial mode.

* The partial mode flag (for streaming) should be handled: if partial is true, it means this chunk might not end a document, so we tolerate an unclosed string and carry state. If partial is false (end of document), we will throw an error if still in string or if UTF-8 sequence incomplete. We’ll follow existing patterns for this (checked in finish() routines)[\[61\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=,n%20%20bit_indexer)[\[53\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=10______,n%20%20if).

**Checklist – Stage 1 Implementation:**

* \[x\] Implement json\_character\_block::classify(simd8x64\<uint8\_t\> in) for RVV, returning a structure with two 64-bit masks: one for structural positions and one for scalar (non-structural non-whitespace) positions. (This could reuse the generic table or logic, but we ensure it uses our vector results.)

* \[x\] Implement json\_string\_scanner.next(simd8x64\<uint8\_t\> in) which returns a json\_string\_block containing at least the quote\_mask (string content mask) for the chunk[\[59\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=json_scanner%3A%3Anext%28const%20simd%3A%3Asimd8x64,mark%20the%20%27t%27%20of%20%27true). Under the hood, this uses our quote/escape logic: compute quote\_bits, escape\_mask (OD), and update the running state. This likely involves storing a few state variables in the json\_string\_scanner object (like prev\_quote or prev\_odd\_backslash). Verify that strings spanning chunks are handled (if prev\_in\_string was true, XOR prefix should start from 1 rather than 0 for the new chunk).

* \[x\] Integrate the above in a loop over input bytes. Use our to\_bitmask() to get bitmasks for each condition and combine them properly. Compare results on sample JSON against fallback to ensure identical structural index output.

* \[x\] Maintain and update state between chunks: prev\_scalar (already in generic code) and our prev\_in\_string / prev\_odd\_backslash. Ensure that at chunk boundaries, the first byte of the new chunk is treated correctly (if we were in a string, its in\_string\_mask continues as 1; if an odd backslash carried over, treat the first quote as escaped by flipping the first bit of escape\_mask). We can achieve this by initializing an “overflow” variable for follows() logic (like the generic immediately\_follows uses overflow to carry a bit beyond 64-bit boundary[\[62\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=in.eq%28%27%5C,n%20%20json_character_block%20characters)). We likely extend that concept: carry an overflow\_backslash and overflow\_string flag between iterations.

* \[x\] Implement UTF-8 check in parallel: in the chunk loop, call a helper that uses RVV to set error\_mask bits for any UTF-8 error in the chunk. Use mask logic for continuation as described, or replicate the known algorithm from the paper (which has been implemented in simdjson)[\[63\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=quote%EC%A0%9C%EA%B1%B0%20R%20%20%3D%20clmul,space%20mask)[\[31\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=By%20performing%20a%20cumulative%20XOR,R). Test this on a variety of UTF-8 inputs: all-ASCII (should pass), valid multi-byte (Chinese characters, emoji), and known invalid sequences (overlong encodings, isolated continuation bytes, boundary cut sequences) to ensure the error\_mask is set when it should.

* \[x\] Handle end-of-input conditions: after the loop, if partial is false (end of doc), check prev\_in\_string. If true, set UNCLOSED\_STRING error. Check utf8\_checker.prev\_incomplete (if the last bytes indicated an incomplete multi-byte) – if true, set UTF8\_ERROR (unless partial). If partial is true (streaming mode), carry those states to the parser for next invocation rather than error.

* \[x\] Ensure the function returns the correct error code (or simdjson error enum) and count of structurals.

* \[x\] Verify the Stage 1 output by running all Stage 1 unit tests (there are likely tests in tests/\*.cpp that check structural counts, etc.) and comparing with known good output (fallback or another backend) on sample JSON files of various sizes.

### 3.3 Stage 2 (Optional, Stretch Goal): Document Parsing

Stage 2 involves building the DOM (or triggering callbacks in On-Demand) from the structural indices and the input text. It is marked as an optional stretch goal for RVV – meaning we might initially use the existing fallback Stage 2, but we outline potential optimizations.

* **Approach:** Stage 2 is less about data-parallel operations and more about branching and state machines (walking through the array of structural indices, identifying values, numbers, etc.). As such, many backends do not heavily vectorize Stage 2\. We can **reuse the generic Stage 2** (internal::dom\_parser\_implementation from simdjson’s generic code) to parse the document using the structural indices from our Stage 1\. This yields minimal code duplication and ensures correctness (since Stage 2 logic is complex and well-tested in simdjson generically). Initially, we will do exactly this: our rv64::implementation::create\_dom\_parser\_implementation() will instantiate an rv64::dom\_parser\_implementation that uses RVV for Stage 1 but calls the base class’s Stage 2 logic for actual parsing. This means after Stage 1, the processing of tokens (converting numbers from text to binary, etc.) will happen in scalar code (or with some small SIMD usage that’s already in generic code).

* **Potential Stage 2 vectorization (future):** If we were to optimize Stage 2, one area is number parsing (converting strings of digits to integer or double). There have been SIMD approaches to parse 8 or 16 digits at once (e.g., using 64-bit multiplication on SSE). RISC-V vectors could potentially parse more digits in parallel. Another is streaming string copying (for string values, copying from input to the output DOM – but simdjson often does this lazily or with pointers into the original padded string, so minimal copying occurs). These optimizations are complex and would require significant new code, so for now we mark them as *stretch*: they can be attempted once Stage 1 is stable and if performance analysis shows Stage 2 as a bottleneck.

* **Integration:** Our rv64::dom\_parser\_implementation class can simply inherit the generic internal::dom\_parser\_implementation and override the stage1() method to call our RVV routine. It can rely on the base class for parse\_document() which calls stage1 then stage2. If needed, we might override the stage2’s parse\_value() to use an RVV optimized number parsing in the future, but initially we won’t.

* **Testing Stage 2:** Because we plan to use generic Stage 2, we expect full JSON parsing (DOM building and On-Demand queries) to just work with our Stage 1 output. We will run the entire test suite (which includes DOM/ondemand parsing tests) with RVV enabled in QEMU. This will validate that Stage 2 combined with our Stage 1 produces correct final results for all JSON examples.

**Checklist – Stage 2 (if attempted):**

* \[ \] **(Now)** Use generic Stage 2: Ensure that after Stage 1, the dom\_parser\_implementation continues with the same code path as fallback (just using our filled structural\_indexes). Confirm that DOM parsing tests pass, implying Stage 2 was compatible.

* \[ \] **(Future)** If optimizing: implement vectorized routines for number parsing (e.g., 8 digits at a time conversion using 64-bit RVV operations), string copying (maybe use wider loads/stores), and boolean/null value comparisons (though these are trivial). Make sure to guard these with extensive tests (including tricky cases like very large numbers, numbers near overflow, etc.).

* \[ \] Keep Stage 2 diff minimal: possibly confine any new Stage 2 logic to rv64/numberparsing\_defs.h or similar, and include it only for RVV, so upstream can review it in isolation.

*(Since Stage 2 optimization is optional, our development plan will prioritize Stage 1 and integration. We note this section for completeness but it may be deferred until Stage 1 is merged and validated.)*

## 4\. Runtime Integration

With the RVV backend implemented, we must integrate it into simdjson’s runtime dispatch mechanism and ensure it coexists with other implementations.

* **Registration in get\_available\_implementations():** simdjson has a global list of compiled SIMD implementations[\[64\]](file://file_00000000258471fd8ebd57e18c428e24#:~:text=internal%3A%3Aavailable_implementation_list%26%20get_available_implementations%28%29%3B%5Cn%5Cn%2F,nextern%20SIMDJSON_DLLIMPORTEXPORT). We will add RV64 to this list. Specifically, in the internals (likely in implementation.cpp or the header that defines available\_implementation\_list), we will append our rv64::implementation. For example, there may be code like:

* \#if SIMDJSON\_IMPLEMENTATION\_RV64
  available\_implementations\_list.add\_internal( internal::get\_rv64\_singleton() );
  \#endif

* Where get\_rv64\_singleton() returns a pointer to a static rv64::implementation instance[\[65\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=,define)[\[66\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=ppc64%3A%3Aimplementation,only). We will create a function similar to others for RV64. This ensures that if the binary is compiled with RVV support, the list of available impls includes "rv64".

* **CPU detection:** At startup, simdjson will choose the **active implementation** by checking which compiled implementation is supported by the running CPU[\[67\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=to%5Cnget%20maximum%20performance.%5Cn%5CnRuntime%20CPU%20Detection%5Cn,). We need to implement rv64::implementation::supported\_by\_runtime\_system() to return true if and only if the current CPU actually has the RISC-V Vector extension. On other architectures, this might check CPUID bits. For RISC-V, detection is tricky but possible:

* On Linux, starting with kernel 5.11+, the presence of the vector extension can be checked via getauxval(AT\_HWCAP) or AT\_HWCAP2) where a bit (likely HWCAP2\_RVV) would be set if V is available. We should use this if available. Alternatively, RISC-V Linux introduced a PR\_SET\_FP\_MODE/PR\_SET\_VL prctl to enable vector usage[\[68\]](https://docs.kernel.org/arch/riscv/vector.html#:~:text=Two%20new%20prctl,usage%20guideline%20for%20these). If a process can execute a vector instruction without crashing, it’s enabled. But we prefer not to trap an illegal instruction for detection.

* Easiest: we can attempt to read the **CPU ISA string** from /proc/cpuinfo (which on RISC-V contains a line like isa: rv64imafdcv if V is present). As a fallback, doing so and searching for v might suffice (ensuring we distinguish 'v' extension from, say, a 'v' in some vendor name). This is a bit hacky but can work in user-space.

* Ideally, use standardized feature macros: if we compiled with vector support (\_\_riscv\_vector), we assume the target CPU *might* have it. But at runtime on a different CPU lacking V, the code would crash if we call it. So dispatch must be correct.

* We will implement a conservative approach: call a small function that attempts to execute a harmless vector instruction inside a SIGILL handler to detect support. However, given the complexity, a simpler approach is to rely on the OS: we can use \<sys/auxv.h\> on Linux:

* \#ifdef RISCV\_HWCAP\_CHECK
    unsigned long hwcap \= getauxval(AT\_HWCAP2);
    if (hwcap & (1UL \<\< SOME\_RVV\_BIT)) return true;
  \#endif

* (We need to find out the correct HWCAP bit for RVV; assuming it exists in the running environment).

* For the scope of our plan, we assume QEMU or target environment *does* have V (since we compile for it). We will document that supported\_by\_runtime\_system() needs to be robust. In the interim, we might have it simply return true on RISC-V builds (meaning “if it’s compiled in, assume available”), and rely on the user to disable RV64 if running on a non-V system. This matches how ARM64 is handled (Neon is mandatory on ARM64 so no runtime check needed). But RISC-V V is not mandatory, so a runtime check is ideal if possible.

* **Optional fallback dispatch:** Since RVV might not be present on all CPUs, we *keep the fallback implementation compiled* (unless user disables it). The dispatch logic will then choose either rv64 or fallback at runtime. Concretely:

  * If active\_implementation is not already set, simdjson will iterate: it will check icelake (not present on RISC-V, skip), haswell (skip), ..., arm64 (skip), ppc64, lsx, lasx (skip), then rv64 (if compiled). It will call rv64\_impl-\>supported\_by\_runtime\_system(). If we implement that to verify CPU features, it will return true only if V is present. If true, active\_implementation becomes rv64. If false (no vector), it moves on to fallback. Thus, on a non-V machine, fallback will be chosen safely[\[69\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=on%20any%2064,nThe)[\[70\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=they%20call%20%60parse%28%29%60%20or%20%60load%28%29%60.,name).

  * We will test this by simulating the scenario. For example, we can temporarily force supported\_by\_runtime\_system() to false and see that get\_active\_implementation()-\>name() yields "fallback". Conversely, on a V-enabled QEMU/hardware, it should yield "rv64". We’ll add a runtime check in our QEMU test to print the detected implementation to ensure it works.

* **Conditional compilation:** The build system defines SIMDJSON\_IMPLEMENTATION\_RV64 based on compiler support. If someone compiles simdjson on RISC-V *without* vector (e.g., \-march=rv64gc without v), our code will not include RV64 at all (the macro will be 0). Then only fallback is available (so it still works, just slower). If compiled with \-march=rv64gcv, then our code is included. We’ll document this behavior. The user can also manually set \-DSIMDJSON\_IMPLEMENTATION\_RV64=0 at compile time to exclude our backend (similar to how they can disable others)[\[71\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=going%20to%20run%20and%20want,nolder%20processors.%20_Note%3A_%20for%20POWER9).

* **Minimize overhead:** If both RV64 and fallback are compiled in, the binary size increases. We ensure to wrap our RVV code in proper ifdefs so that if SIMDJSON\_IMPLEMENTATION\_RV64=0 the overhead is zero. Also, we avoid static constructors or heavy init: our rv64::implementation instance will be static but minimal initialization (just populating name/description).

* **Name and description:** We will set the implementation name as "rv64", and description string to something like "RISC-V 64-bit (RVV 1.0)". This will appear in logs or simdjson::get\_active\_implementation()-\>description() output. We ensure the naming follows the style of others (lowercase code, descriptive text)[\[4\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=SIMDJSON_TARGET_WESTMERE%5Cnnamespace%20simdjson%20%7B%5Cnnamespace%20westmere%20%7B%5Cn%5Cn%2F,size_t%20max_length). We will also add this to documentation (see Documentation section).

* **Integration points in code:** Files likely to update:

* simdjson/implementation.h: Add an extern const implementation \*available\_rv64 if needed (some implementations declare extern singletons).

* simdjson/internal/instruction\_set.h: As discussed, macros for compile-time enabling and maybe a placeholder for RVV in instruction\_set:: flags enum (if they use an enum bitfield for features like SSE4.2, AVX2, etc., we add RVV).

* src/implementation.cpp or equivalent: Register the singleton in the list and handle detection. Possibly update the logic that prints supported implementations in README.

* **Dispatch testing:** We will test a few scenarios:

* On a known RVV system, ensure get\_active\_implementation() returns RV64 and parsing works.

* Force fallback at runtime to test fallback still works (e.g., by temporarily making supported\_by\_runtime\_system() false or by using an environment variable if simdjson supports forcing an implementation via SIMDJSON\_FORCE\_IMPLEMENTATION or similar).

* If possible, test that if we disable fallback at compile time (\-DSIMDJSON\_IMPLEMENTATION\_FALLBACK=0) on a V-enabled CPU, it runs on RV64 and fails gracefully if run on a non-V CPU (this last scenario is unusual and probably not recommended, but we can note it). Usually, one wouldn’t disable fallback unless they *know* all target CPUs have vectors.

**Checklist – Runtime Integration:**

* \[x\] Add RV64 to compile-time selection macros in instruction\_set.h[\[72\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=SIMDJSON_IMPLEMENTATION_PPC64%5Cn,define), default ON for riscv64 if vector intrinsics available.

* \[x\] Define a new instruction\_set::RVV flag (bit mask) and include it in rv64::implementation constructor (for informational purposes)[\[4\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=SIMDJSON_TARGET_WESTMERE%5Cnnamespace%20simdjson%20%7B%5Cnnamespace%20westmere%20%7B%5Cn%5Cn%2F,size_t%20max_length).

* \[x\] Implement rv64::implementation::supported\_by\_runtime\_system(). Try using OS feature detection (HWCAP or /proc/cpuinfo). At minimum, implement a conservative version that returns true on RISC-V builds, and **document** that if running on a CPU without V, the user should disable RV64 at compile-time to avoid illegal instructions. (We will strive to improve this with proper checks when possible).

* \[x\] Register the rv64 implementation in the global list. Ensure that if both RV64 and fallback are available, the logic picks RV64 if supported. (Follow the pattern where e.g., on x86 if AVX2 is supported it picks haswell over fallback).

* \[x\] Update simdjson::get\_available\_implementations() to include "rv64" in the map of names to impl pointers[\[73\]](file://file_00000000258471fd8ebd57e18c428e24#:~:text=simdjson%3A%3Aget_available_implementations%28%29%5B%5C,n%20%20%20%2A%20%40return). This allows users to manually select it via name if desired.

* \[x\] Verify simdjson::get\_active\_implementation()-\>name() returns "rv64" on a V-enabled run, and that simdjson::active\_implementation-\>description() is correct (e.g., “RISC-V64 (RVV 1.0)”).

* \[x\] Ensure that building simdjson on non-RISC-V or on RISC-V without vector does not include or compile our RVV code (the macros and CMake checks will handle this). Test by building with \-DSIMDJSON\_IMPLEMENTATION\_RV64=0 and see that no RVV symbols are present.

* \[x\] Confirm that fallback still works by running the test suite on a configuration where RV64 is either not compiled or not chosen (for example, compile with RV64 but run under QEMU with vector disabled). The library should seamlessly fall back to the generic implementation with correct results.

## 5\. Build System Integration

To support the RVV backend, we must update the build configuration (CMake and any scripts) to compile the new files and enable cross-building/testing.

* **CMakeLists updates:**

* **Enable RVV backend:** Modify CMake to detect RISC-V architecture. We can use CMake’s CMAKE\_SYSTEM\_PROCESSOR or compiler predefined macros. For example:

* if(CMAKE\_SYSTEM\_PROCESSOR STREQUAL "riscv64" OR CMAKE\_C\_COMPILER\_TARGET MATCHES "riscv64")
    option(SIMDJSON\_ENABLE\_RV64 "Enable RISC-V RVV backend" ON)
  endif()

* By default, if building natively on RISC-V64, we turn it ON (assuming the compiler supports \<riscv\_vector.h\>). If cross-compiling, user can set this option.

* **Set compile flags:** Ensure the compiler is invoked with the correct flags for vector intrinsics. For GCC, this means adding \-march=rv64gcv (RV64GC \+ Vector extension) and possibly \-mabi=lp64d (if not default). For Clang, similarly \-march=rv64gcv. We might create a CMake variable SIMDJSON\_RVV\_FLAGS that defaults to these. We should also include \-O3 (which is default in release) and ensure the compiler is new enough. We may add a check: try compiling a small snippet with \<riscv\_vector.h\> in CMake to verify support, otherwise warn that RVV is not supported by the compiler.

* **Target include and source files:** Add our new files to the library target. For instance:

* if(SIMDJSON\_ENABLE\_RV64)
    target\_sources(simdjson PRIVATE
      include/simdjson/rv64.h
      include/simdjson/rv64/base.h ... \[all RV64 headers\] ...
      )
  endif()

* Since simdjson might amalgamate sources, we have to ensure these headers are packaged in releases. The single-header generation script (tools like release.py) likely pick up all included headers; we should double-check that our rv64.h will be included when appropriate. (We might need to add references in singleheader/simdjson.cpp generation if that process isn’t fully automatic. But likely, including rv64.h in simdjson.h or similar will cause it to be amalgamated when enabled.)

* **Define macro for compilation:** The CMake should add \-DSIMDJSON\_IMPLEMENTATION\_RV64=1 when building for RVV (or rely on our default in code). It should also define any internal macro like \_\_riscv\_vector if not automatically defined (usually \_\_riscv\_vector is defined by the compiler when \-march includes v, so we’re fine).

* **Testing flags:** If we compile the tests for RV64, we ensure they also have \-march=rv64gcv so that the tests can link against the RVV code and possibly use \<riscv\_vector.h\> in any direct usage (though tests likely don’t use intrinsics directly).

* **Cross-compilation support:**

* Provide a toolchain file or instructions to cross-compile from x86\_64 to riscv64. Possibly update the README/BUILDING docs with an example:

* cmake \-B build-rv64 \-DCMAKE\_C\_COMPILER=riscv64-linux-gnu-gcc \-DCMAKE\_CXX\_COMPILER=riscv64-linux-gnu-g++ \-DSIMDJSON\_ENABLE\_RV64=ON \-DCMAKE\_SYSTEM\_PROCESSOR=riscv64 .
  cmake \--build build-rv64

* We might add a convenience script as mentioned in Tools (build-rv64.sh) which sets these variables and also sets up the \-march flags.

* Ensure that when cross-building, simdjson’s tests and tools are not run on the host (CMake’s CTest should skip or allow emulation). We can integrate the QEMU runner in CTest by adding a custom test command prefix (like set(CTEST\_RUNNER "qemu-riscv64 \-cpu rv64,v=true \-E LD\_LIBRARY\_PATH") and have CTest use it). However, that might be beyond scope; likely the developer will run tests manually under QEMU. We at least ensure the cross-compiled binaries (e.g., simdjson\_tests) can be executed with QEMU by linking statically or providing the correct dynamic loader.

* We could consider linking the test binary statically (to avoid needing a riscv64 libc on host). We could add in CMake an option SIMDJSON\_RVV\_TEST\_STATIC=ON to link tests with \-static. We need to caution that static linking on glibc for RISC-V might require \-static \-pthread etc. Alternatively, instruct the developer to install a riscv64 sysroot and use QEMU’s \-L option. For simplicity, building a static test runner is a good approach (since it’s just for internal use).

* **CI Integration (if upstream has CI for multiple arch):** We should extend continuous integration to cover RV64 if possible. This might involve using QEMU in CI (for example, GitHub Actions can run QEMU for RISC-V Linux). If not feasible, we at least set up a nightly or manual test. We will add a job that cross-compiles the library with RVV and runs the basic tests under QEMU. This ensures no regressions on each commit for RV64 backend. This step might be something to coordinate with maintainers (not fully in our control in the planning document, but we can mention it as a recommendation).

* **CMake Options for developers:** Document new options:

* SIMDJSON\_ENABLE\_RV64 (default ON if appropriate, otherwise OFF).

* Possibly SIMDJSON\_FORCE\_RV64 if we want to force using RVV and exclude fallback (not likely needed).

* Ensure existing flags like SIMDJSON\_IMPLEMENTATION\_ICELAKE etc. are complemented by SIMDJSON\_IMPLEMENTATION\_RV64. And that disabling RV64 via cmake (setting to 0\) excludes it from build.

* **Build Testing Matrix:**

* Test build on:

  * Native RISC-V (if hardware or QEMU user emulator used for compilation – this is slow, cross-compiling is preferred).

  * Cross-compile on x86 host targeting riscv64 (this will be our main method).

  * GCC and Clang: verify both compilers can build the RVV code without errors or warnings. (GCC 12+ and Clang 14+ should, but we test the minimum version simdjson supports – if simdjson requires C++17, we ensure vector intrinsics are supported in those compilers).

  * Also test building without RVV (e.g., using an older compiler or with RV64 option off) to ensure our code ifdef’s out cleanly.

* Add compile-time checks to avoid unsupported compilers: e.g., if \_\_riscv\_vector is not defined but SIMDJSON\_IMPLEMENTATION\_RV64=1, throw a \#error "RVV intrinsics not supported by this compiler". This prevents confusing errors if someone forces the flag without proper compiler.

**Checklist – Build System:**

* \[x\] Update CMake to detect riscv64 and include RV64 backend source.

* \[x\] Append necessary compiler flags (\-march=rv64gcv etc.) when building RV64 code. Possibly integrate with CMake’s CheckCXXCompilerFlag to ensure the compiler accepts the flag.

* \[x\] Add new files to target sources/install manifests (so they get installed with library headers).

* \[x\] Provide cross-compilation instructions or script for RV64.

* \[x\] (Optional) Integrate a QEMU test run in the build system (perhaps as a custom CTest command or a script usage).

* \[x\] Test cross-build: using the script, compile the library and run simdjson\_tests under QEMU to ensure everything links and executes properly.

* \[x\] Update any pkg-config or vcpkg integration if needed to account for RISC-V (likely not needed beyond what we do, since pkg-config just lists compiler flags which will include our march if the library was compiled with it).

* \[x\] Ensure single-header generation (make dist or similar) includes RV64. The release script might need an update: e.g., it might have a list of impl to amalgamate. We add "RV64" there so that simdjson.h single-header contains our code when enabled.

* \[x\] Document in BUILDING.md: list required toolchain versions (e.g., “RISC-V Vector extension support requires GCC 12+ or Clang 14+; make sure to compile with \-march=rv64gcv.”). And mention how to run tests via QEMU.

## 6\. Test Coverage

We aim for comprehensive test coverage to ensure the RVV backend is correct and robust. This includes unit tests, integration tests, and platform-specific checks:

* **Compilation & Continuous Integration:**

* Ensure the RVV code compiles without errors or warnings on all targeted compilers. Treat warnings as errors in CI to catch any misuse of intrinsics or type mismatches early. This is part of CI build jobs for RVV (as discussed, possibly via QEMU or cross-compile artifact check).

* If possible, add an automated test job that runs at least the basic tests under QEMU. This might be slower, so perhaps not for every PR but at least manually or nightly. However, for development we will manually run the test suite under QEMU to validate functionality.

* **Unit tests (Stage 1 logic):**
  We will add targeted tests for the critical algorithms:

* **Quote mask generation:** Construct small JSON inputs that stress string masking. For example: "\\"hello\\"" (simple string), "\\"esca\\\\\\"ped\\"" (escaped quote inside string), strings that span across a 64-byte boundary (we can create a 100+ byte string with no closing quote in the first chunk to simulate carry). Verify that the quote mask produced by Stage 1 correctly marks inside-string regions and that the structural indices exclude any commas inside strings, etc. We can compare the results of ondemand::parser.iterate() on such JSON using our RVV backend vs using fallback to ensure the structural indices (and ultimately the parse result) match.

* **Backslash escapes:** Test cases with sequences of backslashes of various lengths at chunk boundaries, e.g. "foo\\\\\\"bar" (even number of backslashes \+ quote, should count as literal quote) vs "foo\\\\\\"bar with an odd count causing an escaped quote. Also test a JSON like "abc\\ (a backslash at end of input) to ensure UNCLOSED\_STRING is flagged.

* **UTF-8 validation:** Prepare a set of byte arrays with known validity/invalidity:

  * Valid 2-byte, 3-byte, 4-byte sequences (including edge like U+07FF, U+0800, U+FFFF, U+10000, U+10FFFF).

  * Invalid sequences: isolated continuation (0x80), starts with 0xC0 0x20 (missing continuation), 0xE0 0x9F (overlong), 0xED 0xA0 (surrogate), 0xF0 0x8F (overlong 4-byte), 0xF4 0x90 (too high)[\[53\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=10______,n%20%20if)[\[54\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=%28simdjson_unlikely%28idx%2B3%20,0%3B%5Cnstatic%20const), etc. We will feed these into simdjson::validate\_utf8() if available, or simply into ondemand::parser.iterate() and expect an error. Our RVV validate\_utf8() override (if implemented) should catch them. We compare with fallback’s validation to ensure we detect the same errors.

  * Also test boundary conditions: a multi-byte character cut off at the end of input (in partial mode, should not error, just wait, but in final mode should error). And extremely long strings of ASCII vs strings containing high Unicode, to ensure performance isn’t explosive (we might measure, though functional test is enough).

* **Structural index correctness:** For a variety of JSON files (from simple to complex):

  * Run Stage 1 (we might expose a debug method to just run stage1 and return the structural index array) and verify the indices point to the right characters (we can cross-check with known positions or by re-parsing with fallback).

  * Use existing tests – many in simdjson\_tests verify that parsing results are correct. If our Stage 1 had errors, it would likely show up as parse failures or mis-parsed values. We expect the comprehensive tests (for DOM and OnDemand) to cover most scenarios: objects, arrays, nested structures, string escapes, numbers, etc. By running them under the RV64 implementation, we implicitly test Stage 1 thoroughly.

  * We should especially test large files (e.g., twitter.json or repeat of it, and a unicode JSON) to ensure no segmentations faults or out-of-bound writes. The library’s own tests (like parse\_many NDJSON tests, large file parsing, etc.) will cover this.

* **Smoke tests (tools):**

* Use the command-line tools provided by simdjson (if any, e.g., json2json or benchmark tools) to do quick sanity tests. For example, run the minify or validate tool on some JSON under QEMU. Ensure they produce expected output.

* If a ondemand sample is available, run it using RVV to make sure the On-Demand API yields correct values for queries (this indirectly tests Stage 1 as well).

* **Block model coverage:**

* We want to be sure our handling of chunk boundaries is correct. We can write a test that feeds a long JSON to the parser in pieces (simulate streaming). For example, use ondemand::parser.iterate\_many or similar if available, or manually feed the parser first N bytes with partial flag, then next bytes, etc. Ensure that the result is same as feeding it whole. This will test if our prev\_in\_string and prev\_scalar carry logic works when a JSON token spans chunks. If any issues (like missing comma detection across boundary), it will manifest as a parse error or incorrect parsed value.

* Test specifically the edge case of 64-byte boundary: e.g., craft a 64-byte JSON (excluding padding) that ends with a quote " as the 64th byte, and continues the string in the next chunk – ensure no off-by-one in quote toggling. Similarly for a number that spans boundary (though typically numbers are short, but we can create a 100-digit number that splits).

* **Performance tests (non-mandatory for correctness but to gauge success):**

* Run the simdjson benchmark (if available) on a sample JSON with RVV vs fallback under QEMU. Note that QEMU does not accurately reflect performance, but if QEMU supports the vector instructions, we can at least see that the vector code path is being exercised (maybe QEMU prints something or at least we know by the active impl). The actual speedup should ideally be tested on real hardware later. But performance tuning is secondary to correctness and minimal diff for now.

* One micro-benchmark: measure Stage 1 throughput by timing simdjson::dom::parser::parse() on a large file with RVV and comparing to fallback. Under QEMU it will be slow anyway, but a relative difference might be visible if vector operations are emulated somewhat faster than pure scalar. This is optional and more for curiosity – the main check is that we don’t introduce any *catastrophic* slowdowns or infinite loops (e.g., if our algorithm has an issue that causes it to spin or mis-handle something, the tests would hang or time out).

* **Robustness tests:**

* Fuzz test Stage 1: feed random bytes (invalid JSON) and ensure it does not crash and properly returns an error (likely a UTF8 error or something). The simdjson test suite might already do some pseudo-fuzz. We might manually attempt a few random inputs of size \~128 bytes to see if any assertion triggers or if results differ between fallback and RVV. If differences, investigate – likely our UTF-8 or structural logic might misclassify in some corner, which these random inputs could reveal.

* Test memory alignment assumptions: our code loads 64 bytes – simdjson ensures an SIMD-friendly padding, but let’s verify that if input length is not multiple of 64, our code safely reads within bounds. Typically, simdjson pads input with extra bytes (maybe zeros)[\[74\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=%5Cn%5Cnclass%20json_structural_indexer%20%7B%5Cnpublic%3A%5Cn%20%20%2F,n%20%20simdjson_inline%20json_structural_indexer%28uint32_t), so reading up to next 64 boundary is safe. We rely on that. We should test parsing a non-padded buffer (simulate user forgetting to pad) – simdjson should normally copy it into an padded buffer internally. Just to be safe, run parser on an odd-length input and under tools like valgrind (possibly via QEMU’s memcheck if available) to ensure no invalid reads/writes. This is more of a general simdjson guarantee but we validate it remains true with our backend.

**Checklist – Test Coverage:**

* \[x\] Write new tests or reuse existing to specifically verify quote mask and escape handling (strings with quotes and backslashes). Ensure structural indexes skip quoted content correctly.

* \[x\] Verify through tests that an unterminated string is detected: e.g., "{"key": "value} (missing closing quote or brace) yields UNCLOSED\_STRING or TAPE\_ERROR appropriately, in both streaming partial mode and final mode.

* \[x\] Add cases for UTF-8 validity: create small test cases for each invalid pattern and check that ondemand::parser.iterate() returns an error. We can use ASSERT\_ERROR(doc, UTF8\_ERROR) in the test (simdjson test macros) to validate. Also test a fully valid multi-lingual JSON (with Chinese, emoji, etc.) parses successfully.

* \[x\] Run the full test suite (simdjson\_tests) with RVV backend active and ensure **all tests pass**. This is the ultimate confirmation of correctness since simdjson’s suite is thorough (covering DOM, OnDemand, parser, scalar fallback comparisons, etc.).

* \[x\] Test on large inputs: e.g., parse a 3MB JSON under QEMU to ensure no issues with memory or performance (it will be slow under emulation, but should complete and produce the correct number of elements).

* \[x\] Monitor for any assertion or bounds errors (compile with ASan/UBSan if possible for RISC-V, or at least valgrind). Fix any out-of-bounds memory issues if found.

* \[x\] Confirm that with RVV disabled (fallback only), all tests still pass – to ensure our integration didn’t break anything for other platforms. (This is just running tests normally on x86 for instance).

By covering these test points, we’ll have high confidence in the correctness and stability of the RVV backend.

## 7\. Documentation Updates

We will update documentation to reflect the addition of the RISC-V backend, and provide guidance on building and using it.

* **BUILDING.md (or equivalent build docs):**

* Add a section “**Building on RISC-V (RV64) with Vector Extension**”. Explain that simdjson now supports RISC-V Vector acceleration. Include steps to compile:

  * “Requires a compiler with RVV intrinsics support (GCC 12+, Clang 14+, or newer).”

  * “If compiling natively on RISC-V with vector extension, no special flags needed if your compiler’s default march includes V; otherwise specify \-march=rv64gcv. The CMake build will try to add this automatically if SIMDJSON\_ENABLE\_RV64 is ON.”

  * “For cross-compiling from x86\_64: install a riscv64 toolchain, and use \-DCMAKE\_C\_COMPILER\_TARGET=riscv64-linux-gnu \-DSIMDJSON\_ENABLE\_RV64=ON. Also ensure to set \-DCMAKE\_CXX\_FLAGS="--target=riscv64-linux-gnu \-march=rv64gcv \-mabi=lp64d" if CMake doesn’t do it. (Our provided script does this.)”

  * Mention any known compiler issues or if a certain flag (like \-menable-experimental-extensions for older compilers) is needed (should not for stable ones).

  * Note that if the target CPU does *not* have the vector extension, the binary will fall back to scalar automatically **if fallback is compiled**. We should warn: “Do not disable fallback unless you are sure all deployment targets support V; otherwise, you’ll get illegal instructions on CPUs without V.”

  * Outline how to run tests with QEMU if not on real hardware. For example: “You can use QEMU user mode to run the tests: qemu-riscv64 \-cpu rv64,v=true build/simdjson\_tests (assuming your QEMU supports RVV). The v=true flag enables the vector extension in QEMU’s emulated CPU. If using QEMU 7.0 or later, specify \-cpu rv64gcv or similar. See QEMU documentation for enabling experimental extensions.”

  * If applicable, mention that Linux kernel 5.14+ might require a prctl to use vectors. In practice, as of kernel 6.x, by default the first use of a vector instruction enables the context (with some overhead). It's worth mentioning: “On some Linux systems, the vector unit may be disabled by default for new processes to save context switch cost. The simdjson library will attempt to enable it by executing vector instructions. If you experience an illegal instruction despite a V-enabled CPU, ensure your OS allows vector usage (e.g., on Linux, upgrade to a kernel with RVV support or use prctl(PR\_SET\_VL, ... )).”

  * Provide an example to verify it’s using RVV: e.g., get\_active\_implementation()-\>name() returns "rv64".

* Ensure BUILDING.md lists RISC-V as a supported platform now, alongside x86, ARM, etc.

* Document the new CMake option SIMDJSON\_ENABLE\_RV64 and how to force disable it if needed.

* **README.md:**

* In the overview of supported architectures (if present), add “**RISC-V 64-bit (RVV 1.0)**” to the list. For example: currently README or docs list icelake, haswell, westmere, arm64, ppc64, loongarch (LSX/LASX), fallback[\[75\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=different%20CPU%20architectures%2C%20often%20with,endian%2C%20POWER%20systems.%5Cn%2A%20lasx%3A%20Loongson). We add bullet: “\* rv64: 64-bit RISC-V with the Vector extension v1.0[\[75\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=different%20CPU%20architectures%2C%20often%20with,endian%2C%20POWER%20systems.%5Cn%2A%20lasx%3A%20Loongson).” and perhaps note parenthetically that it is tested on QEMU or on specific hardware if applicable (for now, likely just QEMU).

* If README has a section on *runtime dispatch*, update it to include RISC-V: explain that on RISC-V, simdjson will include two implementations by default (rv64 and fallback) and will choose rv64 on CPUs with V, or fallback otherwise (similar to how PPC does)[\[69\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=on%20any%2064,nThe). Encourage not to compile out fallback unless targeting a specific device.

* Update any performance numbers or charts if any exist. (If not available for RISC-V yet, we might skip, or we can mention “performance on a prototype vector hardware or QEMU indicates significant speedups over scalar, but official benchmarks will come later when hardware is widely available.”)

* Acknowledge contributors or that this is an initial implementation (if relevant).

* Possibly mention that RVV support is new and any feedback or testing on real hardware is welcome (if upstream likes to invite community testing).

* **DESIGN.md (or developer documentation):**

* Simdjson might have a design document describing how stage 1 works and how implementations differ. We will add a subsection for RISC-V:

  * Outline that RISC-V’s vector model is dynamic and how we chose a static 64-byte processing approach using LMUL (this is important for others reading code to know why we did that).

  * Note any differences: e.g., “Unlike other SIMD implementations with fixed register sizes, the RV64 backend can adapt to different hardware vector lengths. We currently assume at least 128-bit and use LMUL to reach 512-bit total width. If VLEN \> 512, we limit processing to 64 bytes at a time for consistency and to reuse the existing algorithm.”

  * Mention that we rely on portable intrinsics and no assembly, and that the approach mirrors the one used for LSX/LASX (if that’s a useful comparison).

  * Document any limitations: “We have not (yet) vectorized stage 2, so the performance gains are mainly in stage 1 indexing. In deeply nested or small JSON documents, this is less of a factor, but in large documents it helps.”

  * If there are known *caveats*, e.g., if some edge case is handled slightly differently – ideally none. Perhaps mention we do scalar prefix XOR for quotes instead of CLMUL (which RVV lacks an equivalent for 64-bit) but it’s fine.

  * Summarize how we do UTF-8 check with RVV and that it mirrors the algorithm from existing code.

  * If portability: note that the code assumes little-endian (RISC-V can be big-endian but it’s extremely rare; our intrinsics usage should technically work either way because operations are per-byte, but we haven’t tested big-endian).

  * Also mention kernel context enabling: i.e., that first vector use might incur a system call to allocate vector register context if OS does lazy enable. It’s an OS detail but worth noting for completeness.

* Add references to the RISC-V spec or intrinsics doc as needed, perhaps for maintainers to find more info (like linking to the official intrinsics documentation).

* **tools/README.md (or similar for scripts):**

* Document the usage of build-rv64.sh and run\_rv64\_tests.sh: what prerequisites (QEMU installed, a riscv64 cross toolchain, etc.), and example output.

* If we integrated QEMU in CTest, explain how to invoke it (maybe via an env var or a CTest label).

* Instruct that QEMU 7.2+ is recommended because it fully supports V 1.0. Earlier QEMU might require vext\_spec=0.7.1 or similar which could be problematic (we saw that in the StackOverflow snippet above). We’ll say “Ensure you use a QEMU version that supports the ratified 1.0 spec (QEMU 8.0 or later should suffice).”

* Provide troubleshooting tips: if tests fail due to illegal instruction, check that QEMU CPU was launched with vector support (the script should handle it). If something fails, one can run with SIMDJSON\_FORCE\_IMPLEMENTATION=fallback to compare behaviors, etc.

* **API documentation (if any):**

* The public API doesn’t change except possibly the output of simdjson::get\_available\_implementations(). If that is documented somewhere (like in a table of arch in the docs website), update it to include rv64.

* If there’s a mention of simdjson::ARM64, etc., add simdjson::RV64 accordingly for completeness.

* **Changelog/Release notes:**

* Mark that “Added support for RISC-V Vector extension (thanks to ... )” in the upcoming release notes, so users are aware.

After these doc updates, users and contributors will know about the new backend and how to use it effectively.

**Checklist – Documentation:**

* \[x\] Add RISC-V backend info to README (supported architectures list)[\[75\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=different%20CPU%20architectures%2C%20often%20with,endian%2C%20POWER%20systems.%5Cn%2A%20lasx%3A%20Loongson) and any introductory text.

* \[x\] Update the runtime dispatch discussion in docs to include RV64 (e.g., note that on RISC-V builds, the library will dispatch between rv64 and fallback)[\[69\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=on%20any%2064,nThe).

* \[x\] Expand BUILDING.md with RISC-V build instructions, including cross-compile and QEMU usage.

* \[x\] Document new CMake option(s) in BUILDING.md or a separate “CMake Options” section, including default values.

* \[x\] Update DESIGN/internals documentation: describe how the RVV implementation works (especially any deviations from the others). This helps future maintainers understand our code decisions. Possibly include a brief example pseudocode snippet like in the Medium article[\[7\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a)[\[63\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=quote%EC%A0%9C%EA%B1%B0%20R%20%20%3D%20clmul,space%20mask), but adapted to our approach (with comments).

* \[x\] Verify that doxygen or Sphinx documentation generation (if any) picks up our rv64::implementation class and any new functions. If some symbol needs to be marked for docs, do so.

* \[x\] Tools/README: Document usage of any new scripts and mention QEMU as needed.

* \[x\] In any list of contributors or acknowledgments, add attribution if necessary (if this was a community contribution, etc.).

* \[x\] Proofread all added documentation to ensure accuracy and clarity for both end-users and developers. Use consistent terminology (e.g., refer to it as “RISC-V RV64 (Vector)” in user docs).

* \[x\] Double-check that documentation changes do not conflict with previous statements. For example, if an old doc said “Supported platforms: x86, ARM, PPC, LoongArch”, we now include RISC-V.

* \[x\] Mention testing status: perhaps note “Tested on QEMU and known to work on prototype hardware; passes full test suite.” This gives users confidence, but also transparency that as of writing, widespread hardware testing may be pending if that’s the case.

## 8\. Review and Upstream Support

To facilitate a smooth code review and acceptance upstream, we will prepare review materials and adhere to project guidelines:

* **Review checklist for our implementation:** Before submitting the PR, we ensure:

* Code style matches simdjson (naming conventions, use of simdjson\_inline macros, no trailing whitespace, etc.).

* Sufficient comments are added where the logic might be non-obvious (especially in the tricky parts of Stage 1). We’ll comment our use of LMUL and any unusual steps, referencing the algorithm/paper when relevant for clarity.

* No dead code or leftover debug prints. All \#ifdef logic is correct (for example, no RVV code compiled when it shouldn’t).

* The implementation passes all tests (we will include logs of running the test suite under QEMU as evidence).

* The performance overhead for existing platforms is nil (we didn’t inadvertently change common code in a way that slows others).

* We’ve run static analysis (if available) or at least sanity checks on our code (like ensuring all shifts are within range, no undefined behavior).

* The single-header mode works (generate simdjson.cpp single amalgamation and compile it with RVV flags to see it compiles).

* **Upstream constraints:**

* *Portability:* Upstream will want to ensure our code doesn’t break other platforms. We already accounted for compile-time gating. We should explicitly mention in the PR that “If \_\_riscv\_vector is not defined, our code does nothing, so there’s no impact on x86/ARM/etc.” and maybe show snippet of our ifdefs.

* *Dependencies:* We only rely on the standard \<riscv\_vector.h\>, which is part of GCC/Clang, so no external dependency. We should confirm the minimum compiler versions to upstream so they can update any documentation if needed (e.g., if they claim support for GCC 8 for everything except we need GCC 12 for RVV, that’s fine as long as building on RISC-V presumably uses a new compiler).

* *Testing and CI:* Upstream maintainers might not have RISC-V hardware readily. We’ll provide our QEMU test evidence and possibly help set up CI (maybe in a GitHub Actions Ubuntu runner we can install qemu-user and run our tests as an emulated job). If needed, we can volunteer to monitor issues related to RISC-V backend given our familiarity.

* *Performance caveat:* We note that without hardware it’s hard to provide real benchmarks. We could point out theoretical potential (e.g., “Processing 64 bytes per iteration instead of scalar \~8 bytes gives \~8x throughput in stage1 in ideal conditions; overall parsing maybe \~2-4x faster than scalar, to be confirmed on hardware”). This sets expectations. Also mention that we focused on minimal diff and clarity, so there might be room for further optimizations in the future (like stage2 vectorization or handling \>64-byte vectors), but we chose not to complicate initial implementation.

* *Portability (big-endian etc.):* If upstream cares about big-endian support (they test on s390x for fallback, for example), we should state that RVV backend is untested on big-endian RISC-V (if such exists) and likely would need adjustments if ever needed. Given RISC-V BE is rare, this is low priority.

* *Compliance:* Affirm that we adhere to the official RVV spec 1.0. We should mention the spec version and that we assume V extension version \>=1.0 (since earlier draft 0.7.1 had slightly different intrinsics names, but our code is for the finalized spec). So if someone tries on an older toolchain targeting 0.7.1, it might not compile – but that’s fine because 1.0 is ratified and tools have caught up. We can note it just in case.

* *Memory usage:* The RVV backend doesn’t allocate anything dynamic. It only uses a small amount of stack (a few masks and 64-byte temporary maybe for compress output). This is similar to other backends. So no concerns there. If there is any static large table (like the 256-byte char classification table), we ensure it’s in .rodata and maybe mark it constexpr static so it might even be optimized out or placed in cacheable memory.

* *Thread-safety:* Our code should be thread-safe (simdjson’s stage1 is stateless except the parser buffers). We use only local variables in scanning and static const tables. Nothing we did introduces global mutable state except the active\_implementation which is existing. So we pass this.

* *Upstream maintenance:* We provide comments and perhaps a brief README for our code to help future devs. Also commit to addressing future bug reports if any (implicitly by being part of the project).

* Provide test logs: We might attach (in PR comments or as part of commit message) the results of running simdjson\_tests on QEMU with summary “All tests passed (X assertions in Y tests)” to reassure maintainers.

* **Review process support:**

* Prepare a **PR summary** that breaks down the changes by category (similar to this plan) so reviewers can see that the change, while large (new files), is well-organized: e.g., "Added rv64 directory with 8 headers analogous to westmere, updated CMake, etc.". Summarize testing done.

* Offer to split the PR if needed (for instance, maybe first adding the build system and stubs, then actual implementation) if that helps review. But since this is a feature addition, one PR is reasonable.

* Make sure commit history is clean – probably squash into one commit for adding RVV support.

* Tag any relevant issues (the GitHub issue \#1868 we saw[\[76\]](https://github.com/simdjson/simdjson/issues/1868#:~:text=Hi%21)). We can mention “Closes \#1868” in PR so it auto-closes the feature request.

* Highlight that diff is mostly addition of new files; existing files changes are minimal (just adding hooks), indicating low risk to existing functionality.

* **Portability caveats (again):** Emphasize in review notes that we keep fallback on by default to avoid any risk on non-V systems. Also note that some OS-specific code might be needed for perfect runtime detect (we might ask maintainers if they want a certain approach; perhaps they have a CPU feature detection utility in simdjson already we could use, but likely not for RISC-V yet). They might accept a simpler solution initially given RISC-V is niche, with a TODO to improve detection later.

* **Future support:** Note that as RISC-V ecosystem grows, simdjson may see contributions for 32-bit RISC-V or other extensions. Our code is for RV64 specifically (we define SIMDJSON\_IS\_RISCV64 so 32-bit is excluded). We can mention that a 32-bit with V extension could theoretically be supported with minor tweaks (mostly adjusting pointer sizes and using same logic), but we did not implement or test that (since 32-bit RISC-V Linux with vector is not common). Upstream might be okay with focusing on 64-bit only (just as they only support 64-bit x86/ARM).

**Checklist – Review & Upstream:**

* \[x\] Conduct internal review using the above checklist to catch any issues before PR.

* \[x\] Ensure all style guidelines are met (use clang-format if available with project style).

* \[x\] Create a detailed PR description with context, linking references (e.g., academic paper[\[8\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=The%20SIMD%20JSON%20algorithm%20consists,of%20two%20main%20stages) or spec if needed) to justify algorithm decisions. Possibly cite the Medium article or paper in the PR for background.

* \[x\] Attach test results (e.g., “Tested on QEMU 8.0 (VLEN=128) – all tests pass; also tested fallback on actual riscv64 board without V – parsed correctly with fallback”).

* \[x\] Double-check that no public API changes (so no documentation of API needed beyond what we did).

* \[x\] Respond promptly to code review comments. If maintainers suggest changes (like refactoring something for consistency), be prepared to implement them (for example, they might ask that we unify some of our code with the generic templates more). Given minimal diff was a goal, we likely duplicated a bit of logic (like quote handling) that maybe could be abstracted – if they request it, we’ll work on it.

* \[x\] Provide performance numbers if requested using any data we have (even if just synthetic).

* \[x\] After merging, help with any follow-up bug reports from real hardware testing. Possibly coordinate with any community members who have an early RISC-V vector machine (like those from RISE or Alibaba) to run real benchmarks – we can mention in PR we welcome that.

By following these steps and providing thorough information, we expect the review process to proceed smoothly and the RVV backend to be merged with confidence from the maintainers and community.

---

[\[1\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=simd8,return%2064) [\[2\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask) [\[4\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=SIMDJSON_TARGET_WESTMERE%5Cnnamespace%20simdjson%20%7B%5Cnnamespace%20westmere%20%7B%5Cn%5Cn%2F,size_t%20max_length) [\[5\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=,size_t%20max_length) [\[9\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=%28__m128i%29vec_splat_u8%28N%29%29%29%3B%5Cn%20%20%7D%5Cn%7D%3B%5Cn%5Cntemplate%20,T) [\[10\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=%28__m128i%29vec_splat_u8%28N%29%29%29%3B%5Cn%20%20%7D%5Cn%7D%3B%5Cn%5Cntemplate%20,T) [\[11\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=output%20%2B%2032%20,T%3E%3A%3Asplat%28m%29%3B%5Cn%20%20%20%20return) [\[12\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,uint64_t%20r1) [\[18\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20r0%20%3D%20uint32_t%28this,chunks%5B3%5D%20%3D%3D%20mask%29%5Cn%20.to_bitmask) [\[19\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,count_ones%28mask%20%26%200xFFFFFFFF%29%29%3B%5Cn) [\[20\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,count_ones%28mask%20%26%200xFFFFFFFF%29%29%3B%5Cn) [\[21\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=%28this,return%2064) [\[22\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=uint64_t%20compress%28uint64_t%20mask%2C%20T%20,n) [\[23\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,return%2064) [\[28\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=this,n) [\[45\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=follows%20a%20matching%20character.,n%20%20json_character_block%20characters) [\[65\]](file://file_000000007ed871fdac4d325c16299ccc#:~:text=,define) simdjson\_20260107\_1741\_02\_singleheader.json

[file://file\_000000007ed871fdac4d325c16299ccc](file://file_000000007ed871fdac4d325c16299ccc)

[\[3\]](file://file_00000000c3b471fd90c59e3c51c9d0c0#:~:text=%22content%22%3A%20%22,include%20%5C%22simdjson%2Fwestmere%2Fsimd) simdjson\_20260107\_1653\_04\_include.json

[file://file\_00000000c3b471fd90c59e3c51c9d0c0](file://file_00000000c3b471fd90c59e3c51c9d0c0)

[\[6\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Unlike%20scalar%20parsers%2C%20which%20process,sized%20chunks%20%28e.g.%2C%20256%20bits) [\[7\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a) [\[8\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=The%20SIMD%20JSON%20algorithm%20consists,of%20two%20main%20stages) [\[13\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=We%20then%20calculate%20a%20mask,in%20the%20data%20block) [\[14\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%2A%20Q%3A%20Quotation%20mark%20%28%60,positions%20are%20marked%20as%201) [\[15\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Next%2C%20we%20perform%20the%20following,end%20with%20an%20odd%20length) [\[16\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=1s%29) [\[30\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Afterward%2C%20we%20calculate%20a%20mask,marked%20by%201s) [\[31\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=By%20performing%20a%20cumulative%20XOR,R) [\[32\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=,Exclude%20whitespace%20inside%20the%20string) [\[33\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=%28,position%20of%20the%20value%20token) [\[34\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=W%20%20%3D%20classify,space%20mask) [\[35\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Using%20the%20rule%20that%20a,position%20of%20the%20value%20token) [\[42\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=Next%2C%20we%20combine%20the%20structural,the%20boundaries%20of%20all%20tokens) [\[43\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=,marks%20all%20token%20boundary%20points) [\[44\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=are%20marked%20with%201s%20%28e,0001000100100000) [\[63\]](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a#:~:text=quote%EC%A0%9C%EA%B1%B0%20R%20%20%3D%20clmul,space%20mask) SIMD JSON: Unlocking Maximum Performance for JSON Deserialization | by Donghyung Ko | Medium

[https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a](https://koko8624.medium.com/simd-json-unlocking-maximum-performance-for-json-deserialization-6189a199590a)

[\[17\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=__lsx_vmin_b%28,n) [\[26\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=bytes%20are%20valid%20UTF,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1) [\[27\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=last%20input%20we%20received%20was,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1) [\[29\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=uint8_t%20v7%2C,uint8_t%20v0) [\[36\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=Whether%20the%20last%20character%20of,1%20%7C%20overflow%3B%5Cn) [\[37\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=cat.,n) [\[38\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=%5C,n) [\[39\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=cat.,n) [\[40\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=except%20structural%20characters%20and%20white,so%20either%20we%20get%20a) [\[41\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nothing,moves%20or%20the%20copy%20is) [\[46\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=follows%20a%20matching%20character.,uint8_t%3E%26%20in%29%20%7B%5Cn%20%20json_string_block) [\[47\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=max_value%28%26max_array%5Bsizeof%28max_array%29,n) [\[48\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nonzero%2C%20there%20has%20been%20a,1%3E%28prev_input%29%3B%5Cn) [\[49\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=nonzero%2C%20there%20has%20been%20a,1%3E%28prev_input%29%3B%5Cn) [\[50\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=SIMDJSON_IMPLEMENTATION_ICELAKE,uint8_t) [\[51\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=previous%20input%27s%20last%203%20bytes,255%2C%20255%2C%20255%2C%20255) [\[52\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=bytes%20are%20valid%20UTF,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1) [\[57\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=last%20input%20we%20received%20was,uint8_t%3E%20sc%20%3D%20check_special_cases%28input%2C%20prev1) [\[58\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=a%204,255%2C%20255%2C%20255%2C%20255%2C%20255) [\[59\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=json_scanner%3A%3Anext%28const%20simd%3A%3Asimd8x64,mark%20the%20%27t%27%20of%20%27true) [\[60\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=,255%2C%20255) [\[62\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=in.eq%28%27%5C,n%20%20json_character_block%20characters) [\[66\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=ppc64%3A%3Aimplementation,only) [\[72\]](file://file_00000000f43871fdacdd538e26ffa9e4#:~:text=SIMDJSON_IMPLEMENTATION_PPC64%5Cn,define) simdjson\_20260107\_1653\_02\_singleheader.json

[file://file\_00000000f43871fdacdd538e26ffa9e4](file://file_00000000f43871fdacdd538e26ffa9e4)

[\[24\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=is_continuation%28uint8_t%20c%29%20,n) [\[25\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=uint8_t%20CHAR_TYPE_OPERATOR%20%20%3D%201,n%20%200x00%2C%200x00%2C%200x00) [\[53\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=10______,n%20%20if) [\[54\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=%28simdjson_unlikely%28idx%2B3%20,0%3B%5Cnstatic%20const) [\[55\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=continuation,idx%2B2) [\[56\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=,nstatic%20const%20uint8_t%20CHAR_TYPE_ESC_ASCII) [\[61\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=,n%20%20bit_indexer) [\[74\]](file://file_0000000048b871fda4e60983752e23f5#:~:text=%5Cn%5Cnclass%20json_structural_indexer%20%7B%5Cnpublic%3A%5Cn%20%20%2F,n%20%20simdjson_inline%20json_structural_indexer%28uint32_t) simdjson\_20260107\_1653\_08\_src.json

[file://file\_0000000048b871fda4e60983752e23f5](file://file_0000000048b871fda4e60983752e23f5)

[\[64\]](file://file_00000000258471fd8ebd57e18c428e24#:~:text=internal%3A%3Aavailable_implementation_list%26%20get_available_implementations%28%29%3B%5Cn%5Cn%2F,nextern%20SIMDJSON_DLLIMPORTEXPORT) [\[73\]](file://file_00000000258471fd8ebd57e18c428e24#:~:text=simdjson%3A%3Aget_available_implementations%28%29%5B%5C,n%20%20%20%2A%20%40return) simdjson\_20260107\_1741\_04\_include.json

[file://file\_00000000258471fd8ebd57e18c428e24](file://file_00000000258471fd8ebd57e18c428e24)

[\[67\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=to%5Cnget%20maximum%20performance.%5Cn%5CnRuntime%20CPU%20Detection%5Cn,) [\[69\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=on%20any%2064,nThe) [\[70\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=they%20call%20%60parse%28%29%60%20or%20%60load%28%29%60.,name) [\[71\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=going%20to%20run%20and%20want,nolder%20processors.%20_Note%3A_%20for%20POWER9) [\[75\]](file://file_0000000067b471fdb67a0e935d137f0a#:~:text=different%20CPU%20architectures%2C%20often%20with,endian%2C%20POWER%20systems.%5Cn%2A%20lasx%3A%20Loongson) simdjson\_20260107\_1653\_07\_doc.json

[file://file\_0000000067b471fdb67a0e935d137f0a](file://file_0000000067b471fdb67a0e935d137f0a)

[\[68\]](https://docs.kernel.org/arch/riscv/vector.html#:~:text=Two%20new%20prctl,usage%20guideline%20for%20these) Vector Extension Support for RISC-V Linux

[https://docs.kernel.org/arch/riscv/vector.html](https://docs.kernel.org/arch/riscv/vector.html)

[\[76\]](https://github.com/simdjson/simdjson/issues/1868#:~:text=Hi%21) RISC-V 64 \- Vector Support · Issue \#1868 · simdjson/simdjson · GitHub

[https://github.com/simdjson/simdjson/issues/1868](https://github.com/simdjson/simdjson/issues/1868)
