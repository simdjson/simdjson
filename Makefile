REFERENCE_VERSION = master

.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist
COREDEPSINCLUDE = -Idependencies/json/single_include -Idependencies/rapidjson/include -Idependencies/sajson/include -Idependencies/cJSON  -Idependencies/jsmn
EXTRADEPSINCLUDE =  -Idependencies/jsoncppdist -Idependencies/json11 -Idependencies/fastjson/src -Idependencies/fastjson/include -Idependencies/gason/src -Idependencies/ujson4c/3rdparty -Idependencies/ujson4c/src
# users can provide their own additional flags with make EXTRAFLAGS=something
architecture:=$(shell arch)

####
# If you want to specify your own target architecture,
# then define ARCHFLAGS. Otherwise, we set good default.
# E.g., type ' ARCHFLAGS="-march=westmere" make parse '
###

CXXFLAGS = $(ARCHFLAGS) -std=c++17  -pthread -Wall -Wextra -Wshadow -Ibenchmark/linux
CFLAGS =  $(ARCHFLAGS) -Idependencies/ujson4c/3rdparty -Idependencies/ujson4c/src $(EXTRAFLAGS)

# This is a convenience flag
ifdef SANITIZEGOLD
    SANITIZE = 1
    LINKER = gold
endif

ifdef LINKER
	CXXFLAGS += -fuse-ld=$(LINKER)
	CFLAGS += -fuse-ld=$(LINKER)
endif


# SANITIZE *implies* DEBUG
ifeq ($(MEMSANITIZE),1)
	CXXFLAGS += -g3 -O0  -fsanitize=memory -fno-omit-frame-pointer -fsanitize=undefined
	CFLAGS += -g3 -O0  -fsanitize=memory -fno-omit-frame-pointer -fsanitize=undefined
else
ifeq ($(SANITIZE),1)
	CXXFLAGS += -g3 -O0  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
	CFLAGS += -g3 -O0  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
else
ifeq ($(DEBUG),1)
	CXXFLAGS += -g3 -O0
	CFLAGS += -g3 -O0
else
# we opt for  -O3 for regular builds
	CXXFLAGS += -O3
	CFLAGS += -O3
endif # ifeq ($(DEBUG),1)
endif # ifeq ($(SANITIZE),1)
endif # ifeq ($(MEMSANITIZE),1)

# Headers and sources
SRCHEADERS_GENERIC=src/generic/atomparsing.h src/generic/numberparsing.h src/generic/json_scanner.h src/generic/json_string_scanner.h src/generic/json_structural_indexer.h src/generic/json_minifier.h src/generic/buf_block_reader.h src/generic/stage2_build_tape.h src/generic/stringparsing.h src/generic/stage2_streaming_build_tape.h src/generic/utf8_fastvalidate_algorithm.h src/generic/utf8_lookup_algorithm.h src/generic/utf8_lookup2_algorithm.h src/generic/utf8_range_algorithm.h src/generic/utf8_zwegner_algorithm.h
SRCHEADERS_ARM64=      src/arm64/bitmanipulation.h    src/arm64/bitmask.h    src/arm64/intrinsics.h    src/arm64/numberparsing.h    src/arm64/simd.h    src/arm64/stage1_find_marks.h    src/arm64/stage2_build_tape.h    src/arm64/stringparsing.h
SRCHEADERS_HASWELL=  src/haswell/bitmanipulation.h  src/haswell/bitmask.h  src/haswell/intrinsics.h  src/haswell/numberparsing.h  src/haswell/simd.h  src/haswell/stage1_find_marks.h  src/haswell/stage2_build_tape.h  src/haswell/stringparsing.h
SRCHEADERS_FALLBACK=  src/fallback/bitmanipulation.h src/fallback/implementation.h src/fallback/numberparsing.h src/fallback/stage1_find_marks.h src/fallback/stage2_build_tape.h src/fallback/stringparsing.h
SRCHEADERS_WESTMERE=src/westmere/bitmanipulation.h src/westmere/bitmask.h src/westmere/intrinsics.h src/westmere/numberparsing.h src/westmere/simd.h src/westmere/stage1_find_marks.h src/westmere/stage2_build_tape.h src/westmere/stringparsing.h
SRCHEADERS_SRC=src/isadetection.h src/jsoncharutils.h src/simdprune_tables.h src/implementation.cpp src/stage1_find_marks.cpp src/stage2_build_tape.cpp src/document_parser_callbacks.h
SRCHEADERS=$(SRCHEADERS_SRC) $(SRCHEADERS_GENERIC) $(SRCHEADERS_ARM64) $(SRCHEADERS_HASWELL) $(SRCHEADERS_WESTMERE) $(SRCHEADERS_FALLBACK)

INCLUDEHEADERS=include/simdjson.h include/simdjson/common_defs.h include/simdjson/internal/jsonformatutils.h include/simdjson/jsonioutil.h include/simdjson/jsonparser.h include/simdjson/padded_string.h include/simdjson/inline/padded_string.h include/simdjson/document.h include/simdjson/inline/document.h include/simdjson/parsedjson_iterator.h include/simdjson/inline/parsedjson_iterator.h include/simdjson/document_stream.h include/simdjson/inline/document_stream.h include/simdjson/implementation.h include/simdjson/parsedjson.h include/simdjson/portability.h include/simdjson/error.h include/simdjson/inline/error.h include/simdjson/simdjson.h include/simdjson/simdjson_version.h

ifeq ($(SIMDJSON_TEST_AMALGAMATED_HEADERS),1)
	HEADERS=singleheader/simdjson.h
	LIBFILES=singleheader/simdjson.cpp
	CXXFLAGS += -Isingleheader
else
	HEADERS=$(INCLUDEHEADERS) $(SRCHEADERS)
	LIBFILES=src/simdjson.cpp
	CXXFLAGS += -Isrc -Iinclude
endif

# We put EXTRAFLAGS after all other CXXFLAGS so they can override if necessary
CXXFLAGS += $(EXTRAFLAGS)

FEATURE_JSON_FILES=jsonexamples/generated/0-structurals-full.json jsonexamples/generated/0-structurals-miss.json jsonexamples/generated/0-structurals.json jsonexamples/generated/15-structurals-full.json jsonexamples/generated/15-structurals-miss.json jsonexamples/generated/15-structurals.json jsonexamples/generated/23-structurals-full.json jsonexamples/generated/23-structurals-miss.json jsonexamples/generated/23-structurals.json jsonexamples/generated/7-structurals-full.json jsonexamples/generated/7-structurals-miss.json jsonexamples/generated/7-structurals.json jsonexamples/generated/escape-full.json jsonexamples/generated/escape-miss.json jsonexamples/generated/escape.json jsonexamples/generated/utf-8-full.json jsonexamples/generated/utf-8-miss.json jsonexamples/generated/utf-8.json

RAPIDJSON_INCLUDE:=dependencies/rapidjson/include
SAJSON_INCLUDE:=dependencies/sajson/include
JSON11_INCLUDE:=dependencies/json11/json11.hpp
FASTJSON_INCLUDE:=dependencies/include/fastjson/fastjson.h
GASON_INCLUDE:=dependencies/gason/src/gason.h
UJSON4C_INCLUDE:=dependencies/ujson4c/src/ujdecode.c
CJSON_INCLUDE:=dependencies/cJSON/cJSON.h
JSMN_INCLUDE:=dependencies/jsmn/jsmn.h
JSON_INCLUDE:=dependencies/json/single_include/nlohmann/json.hpp

EXTRAOBJECTS=ujdecode.o

MAINEXECUTABLES=parse minify json2json jsonstats statisticalmodel jsonpointer get_corpus_benchmark
TESTEXECUTABLES=jsoncheck jsoncheck_westmere jsoncheck_fallback integer_tests numberparsingcheck stringparsingcheck pointercheck parse_many_test basictests errortests readme_examples readme_examples_noexceptions
COMPARISONEXECUTABLES=minifiercompetition parsingcompetition parseandstatcompetition distinctuseridcompetition allparserscheckfile allparsingcompetition
SUPPLEMENTARYEXECUTABLES=parse_noutf8validation parse_nonumberparsing parse_nostringparsing

all:  $(MAINEXECUTABLES)

competition:  $(COMPARISONEXECUTABLES)

.PHONY: benchmark test

benchmark:
	bash ./scripts/parser.sh
	bash ./scripts/parseandstat.sh

run_basictests: basictests
	./basictests

run_errortests: errortests
	./errortests

run_numberparsingcheck: numberparsingcheck
	./numberparsingcheck

run_integer_tests: integer_tests
	./integer_tests

run_stringparsingcheck: stringparsingcheck
	./stringparsingcheck

run_jsoncheck: jsoncheck
	./jsoncheck

run_parse_many_test: parse_many_test
	./parse_many_test

run_jsoncheck_westmere: jsoncheck_westmere
	./jsoncheck_westmere

run_jsoncheck_fallback: jsoncheck_fallback
	./jsoncheck_fallback

run_pointercheck: pointercheck
	./pointercheck

run_issue150_sh: allparserscheckfile
	./scripts/issue150.sh

quickstart:
	cd examples/quickstart && make quickstart

run_quickstart:
	cd examples/quickstart && make test

run_testjson2json_sh: minify json2json
	./scripts/testjson2json.sh

$(FEATURE_JSON_FILES): benchmark/genfeaturejson.rb
	ruby ./benchmark/genfeaturejson.rb

run_benchfeatures: benchfeatures $(FEATURE_JSON_FILES)
	./benchfeatures -n 1000

test: quicktests slowtests
	@echo "It looks like the code is good!"

quiettest: quicktests slowtests

quicktests: run_basictests run_quickstart readme_examples readme_examples_noexceptions run_jsoncheck run_numberparsingcheck run_integer_tests run_stringparsingcheck run_jsoncheck run_parse_many_test run_pointercheck run_jsoncheck_westmere run_jsoncheck_fallback

slowtests: run_testjson2json_sh run_issue150_sh

amalgamate:
	./amalgamation.sh

singleheader/simdjson.h singleheader/simdjson.cpp singleheader/amalgamation_demo.cpp: amalgamation.sh src/simdjson.cpp $(SRCHEADERS) $(INCLUDEHEADERS)
	./amalgamation.sh

singleheader/demo: singleheader/simdjson.h singleheader/simdjson.cpp singleheader/amalgamation_demo.cpp
	$(CXX) $(CXXFLAGS) -o singleheader/demo singleheader/amalgamation_demo.cpp -Isingleheader

submodules:
	-git submodule update --init --recursive
	-touch submodules

$(JSON_INCLUDE) $(SAJSON_INCLUDE) $(RAPIDJSON_INCLUDE) $(JSON11_INCLUDE) $(FASTJSON_INCLUDE) $(GASON_INCLUDE) $(UJSON4C_INCLUDE) $(CJSON_INCLUDE) $(JSMN_INCLUDE) : submodules

parse: benchmark/parse.cpp benchmark/event_counter.h benchmark/benchmarker.h $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS)

get_corpus_benchmark: benchmark/get_corpus_benchmark.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o get_corpus_benchmark benchmark/get_corpus_benchmark.cpp $(LIBFILES) $(LIBFLAGS)

parse_stream: benchmark/parse_stream.cpp benchmark/event_counter.h benchmark/benchmarker.h $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_stream benchmark/parse_stream.cpp $(LIBFILES) $(LIBFLAGS)

benchfeatures: benchmark/benchfeatures.cpp benchmark/event_counter.h benchmark/benchmarker.h $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o benchfeatures benchmark/benchfeatures.cpp $(LIBFILES) $(LIBFLAGS)

perfdiff: benchmark/perfdiff.cpp
	$(CXX) $(CXXFLAGS) -o perfdiff benchmark/perfdiff.cpp $(LIBFILES) $(LIBFLAGS)

checkperf:
	bash ./scripts/checkperf.sh $(REFERENCE_VERSION)

statisticalmodel: benchmark/statisticalmodel.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o statisticalmodel benchmark/statisticalmodel.cpp $(LIBFILES) $(LIBFLAGS)


parse_noutf8validation: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_noutf8validation -DSIMDJSON_SKIPUTF8VALIDATION benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS)

parse_nonumberparsing: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_nonumberparsing  -DSIMDJSON_SKIPNUMBERPARSING benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS)

parse_nostringparsing: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_nostringparsing  -DSIMDJSON_SKIPSTRINGPARSING benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS)


jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck tests/jsoncheck.cpp -I. $(LIBFILES) $(LIBFLAGS)

parse_many_test:tests/parse_many_test.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_many_test tests/parse_many_test.cpp -I. $(LIBFILES) $(LIBFLAGS)


jsoncheck_westmere:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck_westmere tests/jsoncheck.cpp -I. $(LIBFILES) $(LIBFLAGS) -DSIMDJSON_IMPLEMENTATION_HASWELL=0

jsoncheck_fallback:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck_fallback tests/jsoncheck.cpp -I. $(LIBFILES) $(LIBFLAGS) -DSIMDJSON_IMPLEMENTATION_HASWELL=0 -DSIMDJSON_IMPLEMENTATION_WESTMERE=0 -DSIMDJSON_IMPLEMENTATION_ARM64=0

basictests:tests/basictests.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o basictests tests/basictests.cpp -I. $(LIBFILES) $(LIBFLAGS)

errortests:tests/errortests.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o errortests tests/errortests.cpp -I. $(LIBFILES) $(LIBFLAGS)

readme_examples: tests/readme_examples.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o readme_examples tests/readme_examples.cpp -I. $(LIBFILES) $(LIBFLAGS)

readme_examples_noexceptions: tests/readme_examples_noexceptions.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o readme_examples_noexceptions tests/readme_examples_noexceptions.cpp -I. $(LIBFILES) $(LIBFLAGS) -fno-exceptions

numberparsingcheck: tests/numberparsingcheck.cpp $(HEADERS) src/simdjson.cpp
	$(CXX) $(CXXFLAGS) -o numberparsingcheck tests/numberparsingcheck.cpp -I. $(LIBFLAGS) -DJSON_TEST_NUMBERS

integer_tests:tests/integer_tests.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o integer_tests tests/integer_tests.cpp -I. $(LIBFILES) $(LIBFLAGS)



stringparsingcheck: tests/stringparsingcheck.cpp $(HEADERS) src/simdjson.cpp
	$(CXX) $(CXXFLAGS) -o stringparsingcheck tests/stringparsingcheck.cpp -I. $(LIBFLAGS) -DJSON_TEST_STRINGS

pointercheck:tests/pointercheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o pointercheck tests/pointercheck.cpp -I. $(LIBFILES) $(LIBFLAGS)

minifiercompetition: benchmark/minifiercompetition.cpp submodules $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o minifiercompetition benchmark/minifiercompetition.cpp -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE)

minify: tools/minify.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o minify tools/minify.cpp -I. $(LIBFILES) $(LIBFLAGS)

json2json: tools/json2json.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o json2json $ tools/json2json.cpp -I. $(LIBFILES) $(LIBFLAGS)

jsonpointer: tools/jsonpointer.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsonpointer $ tools/jsonpointer.cpp -I. $(LIBFILES) $(LIBFLAGS)

jsonstats: tools/jsonstats.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsonstats $ tools/jsonstats.cpp -I. $(LIBFILES) $(LIBFLAGS)

ujdecode.o: $(UJSON4C_INCLUDE)
	$(CC) $(CFLAGS) -c dependencies/ujson4c/src/ujdecode.c

parseandstatcompetition: benchmark/parseandstatcompetition.cpp $(HEADERS) $(LIBFILES) submodules
	$(CXX) $(CXXFLAGS) -o parseandstatcompetition benchmark/parseandstatcompetition.cpp -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE)

distinctuseridcompetition: benchmark/distinctuseridcompetition.cpp $(HEADERS) $(LIBFILES) submodules
	$(CXX) $(CXXFLAGS) -o distinctuseridcompetition benchmark/distinctuseridcompetition.cpp  -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE)

parsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES) submodules
	@echo "In case of build error due to missing files, try 'make clean'"
	$(CXX) $(CXXFLAGS) -o parsingcompetition benchmark/parsingcompetition.cpp -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE)

allparsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES) $(EXTRAOBJECTS) submodules
	$(CXX) $(CXXFLAGS) -o allparsingcompetition benchmark/parsingcompetition.cpp  $(EXTRAOBJECTS) -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE) $(EXTRADEPSINCLUDE)  -DALLPARSER


allparserscheckfile: tests/allparserscheckfile.cpp $(HEADERS) $(LIBFILES) $(EXTRAOBJECTS) submodules
	$(CXX) $(CXXFLAGS) -o allparserscheckfile tests/allparserscheckfile.cpp $(EXTRAOBJECTS) -I. $(LIBFILES) $(LIBFLAGS) $(COREDEPSINCLUDE) $(EXTRADEPSINCLUDE)

.PHONY:  clean cppcheck cleandist

cppcheck:
	cppcheck --enable=all src/*.cpp  benchmarks/*.cpp tests/*.cpp -Iinclude -I. -Ibenchmark/linux

everything: $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES) quickstart

clean:
	rm -f submodules $(EXTRAOBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES)
	cd examples/quickstart && make clean

cleandist:
	rm -f submodules $(EXTRAOBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES)

doc/api: Doxyfile $(HEADERS)
	doxygen
