# Fuzzing

[Fuzzing](https://en.wikipedia.org/wiki/Fuzzing) is efficient for finding bugs. Here are a few bugs in simdjson found by fuzzing:

 - [#353](https://github.com/simdjson/simdjson/issues/353)
 - [#351](https://github.com/simdjson/simdjson/issues/351)
 - [#345](https://github.com/simdjson/simdjson/issues/345)
 - [oss-fuzz 18714](https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=18714&sort=-opened&q=proj%3Asimdjson&can=1)

The simdjson library tries to follow [fuzzing best practises](https://google.github.io/oss-fuzz/advanced-topics/ideal-integration/#summary).

The simdjson library is continuously fuzzed on [oss-fuzz](https://github.com/google/oss-fuzz). In case a bug is found, the offending input is minimized and tested for reproducibility. A report with the details is automatically filed, and the contact persons at simdjson are notified via email. An issue is opened at the oss-fuzz bugtracker with restricted view access. When the bug is fixed, the issue is automatically closed.

Bugs are automatically made visible to the public after a period of time. An example of a bug that was found, fixed and closed can be seen here: [oss-fuzz 18714](https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=18714).


## Currently open bugs


You can find the currently opened bugs, if any at [bugs.chromium.org](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-opened&q=proj%3Asimdjson&can=2): make sure not to miss the "Open Issues" selector. Bugs that are fixed by follow-up commits are automatically closed.

## Integration with oss-fuzz

Changes to the integration with oss-fuzz are made by making pull requests against the oss-fuzz github repo. An example can be seen at [oss-fuzz pull request 3013](https://github.com/google/oss-fuzz/pull/3013).

As little code as possible is kept at oss-fuzz since it is inconvenient to change. The [oss-fuzz build script](https://github.com/google/oss-fuzz/blob/b96dd54183f727a5d90c786e0fb01ec986c74d30/projects/simdjson/build.sh#L18) invokes [the script from the simdjson repo](https://github.com/simdjson/simdjson/blob/master/fuzz/ossfuzz.sh).



## Fuzzing as a CI job

There is a CI job which builds and runs the fuzzers. This is aimed to catch the "easy to fuzz" bugs quickly, without having to wait until pull requests are merged and eventually built and run by oss-fuzz.

The CI job does the following
 - builds several variants (with/without avx, with/without sanitizers, a fast fuzzer)
 - downloads the stored corpus
 - runs the fastest fuzzer build for 30 seconds, to grow the corpus
 - runs each build variant for 10 seconds on each fuzzer
 - using a reproduce build (uninstrumented), executes all the test cases in the corpus through valgrind
 - minimizes the corpus and upload it (if on the master branch)
 - store the corpus and valgrind output as artifacts

The job is available under the actions tab, here is a [direct link](https://github.com/simdjson/simdjson/actions?query=workflow%3A%22Run+fuzzers+on+stored+corpus+and+test+it+with+valgrind%22).

The corpus will grow over time and easy to find bugs will be detected already during the pull request stage. Also, it will keep the fuzzer builds from bit rot.

## Corpus

The simdjson library does not benefit from a corpus as much as other projects, because the library is very fast and explores the input space very well. With that said, it is still beneficial to have one. The CI job stores the corpus on bintray between runs, and is available at [bintray](https://dl.bintray.com/pauldreik/simdjson-fuzz-corpus/corpus/corpus.tar).

One can also grab the corpus as an artifact from the github actions job. Pick a run, then go to artifacts and download.

## Fuzzing coverage

The code coverage from fuzzing is most easily viewed on the [oss-fuzz status panel](https://oss-fuzz.com/fuzzer-stats). Viewing the coverage does not require login, but the direct link is not easy to find. Substitute the date in the URL to get a more recent link:
[https://storage.googleapis.com/oss-fuzz-coverage/simdjson/reports/20200411/linux/src/simdjson/report.html](https://storage.googleapis.com/oss-fuzz-coverage/simdjson/reports/20200411/linux/src/simdjson/report.html)


## Running the fuzzers locally

This has only been tested on Linux (Debian and Ubuntu are known to work).

Make sure you have clang and cmake installed.
The easiest way to get started is to run the following, standing in the root of the checked out repo:
```
fuzz/build_like_ossfuzz.sh
```

Then invoke a fuzzer as shown by the following example:
```
mkdir -p out/parser
build/fuzz/fuzz_parser out/parser/
```

You can also use the more extensive fuzzer build script to get a variation of builds by using
```
fuzz/build_fuzzer_variants.sh
```

It is also possible to run the full oss-fuzz setup by following [these oss-fuzz instructions](https://google.github.io/oss-fuzz/getting-started/new-project-guide/#testing-locally) with PROJECT_NAME set to simdjson. You will need rights to run docker.

## Reproducing
To reproduce a test case, build the fuzzers, then invoke it with the testcase as a command line argument:
```
build/fuzz/fuzz_parser my_testcase.json
```
