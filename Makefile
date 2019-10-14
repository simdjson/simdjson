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
# E.g., type ' ARCHFLAGS="-march=nehalem" make parse '
###
ifeq ($(architecture),aarch64)
ARCHFLAGS ?= -march=armv8-a+crc+crypto
else 
ifeq ($(architecture),armv7l)
ARCHFLAGS ?= -march=armv7
else
ARCHFLAGS ?= -msse4.2 -mpclmul # lowest supported feature set?
endif
endif

CXXFLAGS = $(ARCHFLAGS) -std=c++17   -Wall -Wextra -Wshadow -Iinclude -Isrc -Ibenchmark/linux $(EXTRAFLAGS)
CFLAGS =  $(ARCHFLAGS)  -Idependencies/ujson4c/3rdparty -Idependencies/ujson4c/src $(EXTRAFLAGS)


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

MAINEXECUTABLES=parse minify json2json jsonstats statisticalmodel jsonpointer
TESTEXECUTABLES=jsoncheck integer_tests numberparsingcheck stringparsingcheck pointercheck
COMPARISONEXECUTABLES=minifiercompetition parsingcompetition parseandstatcompetition distinctuseridcompetition allparserscheckfile allparsingcompetition
SUPPLEMENTARYEXECUTABLES=parse_noutf8validation parse_nonumberparsing parse_nostringparsing

# Load headers and sources
LIBHEADERS=src/arm64/simd_input.h src/arm64/simdutf8check.h src/arm64/stage1_find_marks.h src/arm64/stage2_build_tape.h src/arm64/stringparsing.h src/generic/stage1_find_marks_flatten.h src/generic/stage1_find_marks.h src/generic/stage2_build_tape.h src/generic/stringparsing.h src/haswell/simd_input.h src/haswell/simdutf8check.h src/haswell/stage1_find_marks.h src/haswell/stage2_build_tape.h src/haswell/stringparsing.h src/westmere/simd_input.h src/westmere/simdutf8check.h src/westmere/stage1_find_marks.h src/westmere/stage2_build_tape.h src/westmere/stringparsing.h
PUBHEADERS=include/simdjson/common_defs.h include/simdjson/isadetection.h include/simdjson/jsoncharutils.h include/simdjson/jsonformatutils.h include/simdjson/jsonioutil.h include/simdjson/jsonminifier.h include/simdjson/jsonparser.h include/simdjson/numberparsing.h include/simdjson/padded_string.h include/simdjson/parsedjson.h include/simdjson/parsedjsoniterator.h include/simdjson/portability.h include/simdjson/simdjson.h include/simdjson/simdjson_version.h include/simdjson/simdprune_tables.h include/simdjson/stage1_find_marks.h include/simdjson/stage2_build_tape.h
HEADERS=$(PUBHEADERS) $(LIBHEADERS)

LIBFILES=src/jsonioutil.cpp src/jsonparser.cpp src/simdjson.cpp src/stage1_find_marks.cpp src/stage2_build_tape.cpp src/parsedjson.cpp src/parsedjsoniterator.cpp
MINIFIERHEADERS=include/simdjson/jsonminifier.h include/simdjson/simdprune_tables.h
MINIFIERLIBFILES=src/jsonminifier.cpp


RAPIDJSON_INCLUDE:=dependencies/rapidjson/include
SAJSON_INCLUDE:=dependencies/sajson/include
JSON11_INCLUDE:=dependencies/json11/json11.hpp
FASTJSON_INCLUDE:=dependencies/include/fastjson/fastjson.h
GASON_INCLUDE:=dependencies/gason/src/gason.h
UJSON4C_INCLUDE:=dependencies/ujson4c/src/ujdecode.c
CJSON_INCLUDE:=dependencies/cJSON/cJSON.h
JSMN_INCLUDE:=dependencies/jsmn/jsmn.h
JSON_INCLUDE:=dependencies/json/single_include/nlohmann/json.hpp

LIBS=$(RAPIDJSON_INCLUDE) $(JSON_INCLUDE) $(SAJSON_INCLUDE) $(JSON11_INCLUDE) $(FASTJSON_INCLUDE) $(GASON_INCLUDE) $(UJSON4C_INCLUDE) $(CJSON_INCLUDE) $(JSMN_INCLUDE)

EXTRAOBJECTS=ujdecode.o
all:  $(MAINEXECUTABLES)

competition:  $(COMPARISONEXECUTABLES)

.PHONY: benchmark test

benchmark:
	bash ./scripts/parser.sh
	bash ./scripts/parseandstat.sh

run_basictests: basictests
	./basictests

run_numberparsingcheck: numberparsingcheck
	./numberparsingcheck

run_integer_tests: integer_tests
	./integer_tests

run_stringparsingcheck: stringparsingcheck
	./stringparsingcheck

run_jsoncheck: jsoncheck
	./jsoncheck

run_pointercheck: pointercheck
	./pointercheck

run_issue150_sh: allparserscheckfile
	./scripts/issue150.sh

run_testjson2json_sh: minify json2json
	./scripts/testjson2json.sh

test: run_basictests run_numberparsingcheck run_integer_tests run_stringparsingcheck run_jsoncheck run_pointercheck run_testjson2json_sh run_issue150_sh
	@echo "It looks like the code is good!"

quiettest: run_basictests run_numberparsingcheck run_integer_tests run_stringparsingcheck run_jsoncheck run_pointercheck run_testjson2json_sh run_issue150_sh

quicktests: run_basictests run_numberparsingcheck run_integer_tests run_stringparsingcheck run_jsoncheck run_pointercheck

slowtests: run_testjson2json_sh run_issue150_sh

amalgamate:
	./amalgamation.sh
	$(CXX) $(CXXFLAGS) -o singleheader/demo  ./singleheader/amalgamation_demo.cpp   -Isingleheader   

submodules: 
	-git submodule update --init --recursive
	-touch submodules

$(JSON_INCLUDE) $(SAJSON_INCLUDE) $(RAPIDJSON_INCLUDE) $(JSON11_INCLUDE) $(FASTJSON_INCLUDE) $(GASON_INCLUDE) $(UJSON4C_INCLUDE) $(CJSON_INCLUDE) $(JSMN_INCLUDE) : submodules

parse: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

perfdiff: benchmark/perfdiff.cpp
	$(CXX) $(CXXFLAGS) -o perfdiff benchmark/perfdiff.cpp $(LIBFLAGS)

checkperf:
	bash ./scripts/checkperf.sh $(REFERENCE_VERSION)

statisticalmodel: benchmark/statisticalmodel.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o statisticalmodel $(LIBFILES) benchmark/statisticalmodel.cpp $(LIBFLAGS)


parse_noutf8validation: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_noutf8validation -DSIMDJSON_SKIPUTF8VALIDATION $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

parse_nonumberparsing: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_nonumberparsing  -DSIMDJSON_SKIPNUMBERPARSING $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

parse_nostringparsing: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse_nostringparsing  -DSIMDJSON_SKIPSTRINGPARSING $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)


jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)

basictests:tests/basictests.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o basictests $(LIBFILES) tests/basictests.cpp -I. $(LIBFLAGS)


numberparsingcheck:tests/numberparsingcheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o numberparsingcheck tests/numberparsingcheck.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/simdjson.cpp src/stage1_find_marks.cpp  src/parsedjson.cpp       -I. $(LIBFLAGS) -DJSON_TEST_NUMBERS

integer_tests:tests/integer_tests.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o integer_tests tests/integer_tests.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/simdjson.cpp src/stage1_find_marks.cpp src/stage2_build_tape.cpp src/parsedjson.cpp       -I. $(LIBFLAGS) 



stringparsingcheck:tests/stringparsingcheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o stringparsingcheck tests/stringparsingcheck.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/simdjson.cpp src/stage1_find_marks.cpp  src/parsedjson.cpp      -I. $(LIBFLAGS) -DJSON_TEST_STRINGS

pointercheck:tests/pointercheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o pointercheck tests/pointercheck.cpp src/stage2_build_tape.cpp src/jsonioutil.cpp src/jsonparser.cpp src/simdjson.cpp src/stage1_find_marks.cpp  src/parsedjson.cpp src/parsedjsoniterator.cpp -I. $(LIBFLAGS)

minifiercompetition: benchmark/minifiercompetition.cpp $(HEADERS) submodules $(MINIFIERHEADERS) $(LIBFILES) $(MINIFIERLIBFILES)
	$(CXX) $(CXXFLAGS) -o minifiercompetition $(LIBFILES) $(MINIFIERLIBFILES) benchmark/minifiercompetition.cpp -I. $(LIBFLAGS) $(COREDEPSINCLUDE)

minify: tools/minify.cpp $(HEADERS) $(MINIFIERHEADERS) $(LIBFILES) $(MINIFIERLIBFILES)
	$(CXX) $(CXXFLAGS) -o minify $(MINIFIERLIBFILES) $(LIBFILES) tools/minify.cpp -I.

json2json: tools/json2json.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o json2json $ tools/json2json.cpp $(LIBFILES) -I.

jsonpointer: tools/jsonpointer.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsonpointer $ tools/jsonpointer.cpp $(LIBFILES) -I.

jsonstats: tools/jsonstats.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsonstats $ tools/jsonstats.cpp $(LIBFILES) -I.

ujdecode.o: $(UJSON4C_INCLUDE)
	$(CC) $(CFLAGS) -c dependencies/ujson4c/src/ujdecode.c

parseandstatcompetition: benchmark/parseandstatcompetition.cpp $(HEADERS) $(LIBFILES) submodules
	$(CXX) $(CXXFLAGS)  -o parseandstatcompetition $(LIBFILES) benchmark/parseandstatcompetition.cpp -I. $(LIBFLAGS) $(COREDEPSINCLUDE)

distinctuseridcompetition: benchmark/distinctuseridcompetition.cpp $(HEADERS) $(LIBFILES) submodules
	$(CXX) $(CXXFLAGS)  -o distinctuseridcompetition $(LIBFILES) benchmark/distinctuseridcompetition.cpp  -I. $(LIBFLAGS) $(COREDEPSINCLUDE)

parsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES) submodules 
	@echo "In case of build error due to missing files, try 'make clean'"
	$(CXX) $(CXXFLAGS)  -o parsingcompetition $(LIBFILES) benchmark/parsingcompetition.cpp -I. $(LIBFLAGS) $(COREDEPSINCLUDE)

allparsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES) $(EXTRAOBJECTS) submodules
	$(CXX) $(CXXFLAGS)  -o allparsingcompetition $(LIBFILES) benchmark/parsingcompetition.cpp  $(EXTRAOBJECTS) -I. $(LIBFLAGS) $(COREDEPSINCLUDE) $(EXTRADEPSINCLUDE)  -DALLPARSER


allparserscheckfile: tests/allparserscheckfile.cpp $(HEADERS) $(LIBFILES) $(EXTRAOBJECTS) submodules
	$(CXX) $(CXXFLAGS) -o allparserscheckfile $(LIBFILES) tests/allparserscheckfile.cpp $(EXTRAOBJECTS) -I. $(LIBFLAGS) $(COREDEPSINCLUDE) $(EXTRADEPSINCLUDE)

.PHONY:  clean cppcheck cleandist

cppcheck:
	cppcheck --enable=all src/*.cpp  benchmarks/*.cpp tests/*.cpp -Iinclude -I. -Ibenchmark/linux

everything: $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES)

clean:
	rm -f submodules $(EXTRAOBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES)

cleandist:
	rm -f submodules $(EXTRAOBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES) $(SUPPLEMENTARYEXECUTABLES)
