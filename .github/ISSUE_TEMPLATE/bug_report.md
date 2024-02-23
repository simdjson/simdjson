---
name: Bug report
about: Create a report to help us improve
title: ''
labels: bug (unverified)
assignees: ''

---

Before submitting an issue, please ensure that you have read the documentation:

* Basics is an overview of how to use simdjson and its APIs: https://github.com/simdjson/simdjson/blob/master/doc/basics.md
* Performance shows some more advanced scenarios and how to tune for them: https://github.com/simdjson/simdjson/blob/master/doc/performance.md
* Contributing: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md
* We follow the [JSON specification as described by RFC 8259](https://www.rfc-editor.org/rfc/rfc8259.txt) (T. Bray, 2017). If you wish to support features that are not part of RFC 8259, then you should not refer to your issue as a bug.


**Describe the bug**
A clear and concise description of what the bug is. A bug is a failure to build with normal compiler settings or a misbehaviour: when running the code, you get a result that differs from the expected result from our documentation.

A compiler or static-analyzer warning is not a bug. It is possible with tools such as Visual Studio to require that rarely enabled warnings are considered errors. Do not report such cases as bugs. We do accept pull requests if you want to silence warnings issued by code analyzers, however.

We are committed to providing good documentation. We accept the lack of documentation or a misleading documentation as a bug (a 'documentation bug').

An unexpected poor software performance can be accepted as a bug (a 'performance bug').

We accept the identification of an issue by a sanitizer or some checker tool (e.g., valgrind) as a bug, but you must first ensure that it is not a false positive.

We recommend that you run your tests using different optimization levels. In particular, we recommend your run tests with the simdjson library and you code compiled in debug mode. The simdjson then sets the SIMDJSON_DEVELOPMENT_CHECKS macro to 1, and this triggers additional checks on your code and on the internals of the library. If possible, we recommend that you run tests with sanitizers (e.g., see [No more leaks with sanitize flags in gcc and clang](https://lemire.me/blog/2016/04/20/no-more-leaks-with-sanitize-flags-in-gcc-and-clang/)).  You can compile the library with sanitizers for debugging purposes (e.g., set SIMDJSON_SANITIZE to ON using CMake), but you should also turn on sanitizers on your own code. You may also use tools like valgrind or the commercial equivalent.

Before reporting a bug, please ensure that you have read our documentation.

**To Reproduce**
Steps to reproduce the behaviour: provide a code sample if possible. Please provide a complete test with data. Remember that a bug is either a failure to build or an unexpected result when running the code.

If we cannot reproduce the issue, then we cannot address it. Note that a stack trace from your own program is not enough. A sample of your source code is insufficient: please provide a complete test for us to reproduce the issue. Please reduce the issue: use as small and as simple an example of the bug as possible.

It should be possible to trigger the bug by using solely simdjson with our default build setup. If you can only observe the bug within some specific context, with some other software, please reduce the issue first.

**simjson release**

Unless you plan to contribute to simdjson, you should only work from releases. Please be mindful that our main branch may have additional features, bugs and documentation items.

It is fine to report bugs against our main branch, but if that is what you are doing, please be explicit.

**Configuration (please complete the following information if relevant)**
 - OS: [e.g. Ubuntu 16.04.6 LTS]
 - Compiler* [e.g. Apple clang version 11.0.3 (clang-1103.0.32.59) x86_64-apple-darwin19.4.0]
 - Version [e.g. 22]
 - Optimization setting (e.g., -O3)

We support up-to-date 64-bit ARM and x64 FreeBSD, macOS, Windows and Linux systems. Please ensure that your configuration is supported before labelling the issue as a bug.

* We do not support unreleased or experimental compilers. If you encounter an issue with a
pre-release version of a compiler, do not report it as a bug to simdjson. However, we always
invite contributions either in the form an analysis or of a code contribution.

**Indicate whether you are willing or able to provide a bug fix as a pull request**

If you plan to contribute to simdjson, please read our guide:
* CONTRIBUTING guide: https://github.com/simdjson/simdjson/blob/master/CONTRIBUTING.md and our
* HACKING guide: https://github.com/simdjson/simdjson/blob/master/HACKING.md
