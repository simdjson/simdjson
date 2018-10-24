To simplify the engineering, we make some assumptions that can be lifted with some effort:

- We support UTF-8 (and thus ASCII), nothing else (no Latin, no UTF-16).
- We assume AVX2 support which is available in all recent mainstream x86 processors produced by AMD and Intel. No support for non-x86 processors is included.
- We only support GNU GCC and LLVM Clang at this time. There is no support for Microsoft Visual Studio at this time.
- This library cannot parse JSON document of size 16MB or more.
- We expect the input memory pointer to 256-bit aligned and to be padded (e.g., with spaces) so that it can be read entirely in blocks of 256 bits. In practice, this means that users should allocate the memory where the JSON bytes are located using the `allocate_aligned_buffer` function or the equivalent.
