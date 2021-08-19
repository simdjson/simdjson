# Fuzzing

[Fuzzing](https://en.wikipedia.org/wiki/Fuzzing) is efficient for finding bugs. Here are a few bugs in simdjson found by fuzzing:

 - [#353](https://github.com/simdjson/simdjson/issues/353)
 - [#351](https://github.com/simdjson/simdjson/issues/351)
 - [#345](https://github.com/simdjson/simdjson/issues/345)
 - [oss-fuzz 18714](https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=18714&sort=-opened&q=proj%3Asimdjson&can=1)

The simdjson library tries to follow [fuzzing best practises](https://google.github.io/oss-fuzz/advanced-topics/ideal-integration/#summary).

There is both "normal" fuzzers just feeding the api with fuzz data, as well as **differential** fuzzers. The differential fuzzers feed the same data to the multiple implementations (haswell, westmere and fallback) and ensure the same results are achieved. This makes sure the user will always get the same answer regardless of which implementation is in use.

The fuzzers are used in several ways.

* local fuzzing - for developers testing their changes before pushing and/or during development of the fuzzers themselves.
* CI fuzzing - for weeding out those easy to find bugs in pull requests, before they are merged.
* oss-fuzz - heavy duty 24/7 fuzzing provided by the google driven oss-fuzz project

## Local fuzzing
Just invoke fuzz/quick_check.sh, it will download the latest corpus (kept up to date by the CI fuzzers) and run the fuzzers for a short time. In case you want to run the fuzzers for longer, modify the timeout value in the script or invoke the fuzzer directly.

This requires linux with clang and cmake installed (recent Debian and Ubuntu are known to work fine).

It is also possible to run the full oss-fuzz setup by following [these oss-fuzz instructions](https://google.github.io/oss-fuzz/getting-started/new-project-guide/#testing-locally) with PROJECT_NAME set to simdjson. You will need rights to run docker.

## Fuzzing as a CI job - x64

There is a CI job which builds and runs the fuzzers. This is aimed to catch the "easy to fuzz" bugs quickly, without having to wait until pull requests are merged and eventually built and run by oss-fuzz.

The CI job does the following
 - builds a fast fuzzer, with full optimization but less checks which is good at rapidly exploring the input space
 - builds a heavily sanitized fuzzer, which is good at detecting errors
 - downloads the stored corpus
 - runs the fast fuzzer build for a while, to grow the corpus
 - runs the sanitizer fuzzer for a while, using the input found by the fast fuzzer
 - using a reproduce build (uninstrumented), executes a subset of the test cases in the corpus through valgrind
 - minimizes the corpus and uploads it (if on the master branch)
 - stores the corpus and valgrind output as artifacts

The job is available under the actions tab, here is a [direct link](https://github.com/simdjson/simdjson/actions?query=workflow%3A%22Fuzz+and+run+valgrind%22).

The corpus will grow over time and easy to find bugs will be detected already during the pull request stage. Also, it will keep the fuzzer builds from bit rot.

## Fuzzing as a CI job - arm64
There is also a job running the fuzzers on arm64 (see .drone.yml) to make sure also the arm specific parts are fuzzed. This does not update the corpus, it just reuses what the x64 job finds.

## Fuzzing as a CI job - power
There is a fuzzing job similar to the arm64 one. It takes the corpus from the x64 fuzzer as a starting point and fuzzes it for a short while. See the "short fuzz on the power arch" github action job.

## Fuzzing on oss-fuzz
The simdjson library is continuously fuzzed on [oss-fuzz](https://github.com/google/oss-fuzz). In case a bug is found, the offending input is minimized and tested for reproducibility. A report with the details is automatically filed, and the contact persons at simdjson are notified via email. An issue is opened at the oss-fuzz bugtracker with restricted view access. When the bug is fixed, the issue is automatically closed.

Bugs are automatically made visible to the public after a period of time. An example of a bug that was found, fixed and closed can be seen here: [oss-fuzz 18714](https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=18714).


## Currently open bugs

You can find the currently open bugs (if any) at [bugs.chromium.org](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-opened&q=proj%3Asimdjson&can=2): make sure not to miss the "Open Issues" selector. Bugs that are fixed by follow-up commits are automatically closed.

## Integration with oss-fuzz

Changes to the integration with oss-fuzz are made by making pull requests against the oss-fuzz github repo. An example can be seen at [oss-fuzz pull request 3013](https://github.com/google/oss-fuzz/pull/3013).

As little code as possible is kept at oss-fuzz since it is inconvenient to change. The [oss-fuzz build script](https://github.com/google/oss-fuzz/blob/b96dd54183f727a5d90c786e0fb01ec986c74d30/projects/simdjson/build.sh#L18) invokes [the script from the simdjson repo](https://github.com/simdjson/simdjson/blob/master/fuzz/ossfuzz.sh).


## Corpus

The simdjson library does not benefit from a corpus as much as other projects, because the library is very fast and explores the input space very well. With that said, it is still beneficial to have one. The CI job stores the corpus on a remote server between runs, and is available at [www.pauldreik.se](https://readonly:readonly@www.pauldreik.se/fuzzdata/index.php?project=simdjson).

One can also grab the corpus as an artifact from the github actions job if you are logged in at github. Pick a run, then go to artifacts and download.

## Fuzzing coverage

The code coverage from fuzzing is most easily viewed on the [oss-fuzz status panel](https://oss-fuzz.com/fuzzer-stats). Viewing the coverage does not require login, but the direct link is not easy to find. Substitute the date in the URL to get a more recent link:
[https://storage.googleapis.com/oss-fuzz-coverage/simdjson/reports/20200411/linux/src/simdjson/report.html](https://storage.googleapis.com/oss-fuzz-coverage/simdjson/reports/20200411/linux/src/simdjson/report.html)

Keeping the coverage up is a never ending job. See [issue 368](https://github.com/simdjson/simdjson/issues/368)

## Reproducing
To reproduce a test case, use the local build instruction. Then invoke the fuzzer (the fuzz_parser is shown as an example below) with the testcase as a command line argument:
```shell
fuzz/build_fuzzer_variants.sh
build-sanitizers/fuzz/fuzz_parser my_testcase.json
```
In case this does not reproduce the bug, you may want to proceed with reproducing using the oss-fuzz tools. See the instructions [here](https://google.github.io/oss-fuzz/advanced-topics/reproducing/).

# Minimizing and cleansing crashes
If a crashing case is found, it is useful to minimize it and cleanse it (replace irrelevant bytes with spaces).

```shell
build-sanitizers-O0/fuzz/fuzz_ndjson out/ndjson
# ...crashes and writes the crash-... file
# minimize it:
build-sanitizers-O0/fuzz/fuzz_ndjson crash-xxxxxxx -minimize_crash=1 -exact_artifact_path=minimized_crash -max_total_time=100

# replace irrelevant parts with space:
build-sanitizers-O0/fuzz/fuzz_ndjson minimized_crash -cleanse_crash=1 -exact_artifact_path=cleansed_crash

# use/share cleansed_crash

```
