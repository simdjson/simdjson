
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11  -march=native -Wall -Wextra -Wshadow -Iinclude  -Ibenchmark/linux  -Idependencies/rapidjson/include -Idependencies/sajson/include -Idependencies/json11 -Idependencies/fastjson/src -Idependencies/fastjson/include 

ifeq ($(SANITIZE),1)
	CXXFLAGS += -g3 -O0  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
else
	CXXFLAGS += -O3
endif

EXECUTABLES=parse jsoncheck numberparsingcheck stringparsingcheck minifiercompetition parsingcompetition minify allparserscheckfile

HEADERS= include/jsonparser/simdutf8check.h include/jsonparser/stringparsing.h include/jsonparser/numberparsing.h include/jsonparser/jsonparser.h include/jsonparser/common_defs.h include/jsonparser/jsonioutil.h benchmark/benchmark.h benchmark/linux/linux-perf-events.h include/jsonparser/simdjson_internal.h include/jsonparser/stage1_find_marks.h include/jsonparser/stage2_flatten.h include/jsonparser/stage34_unified.h include/jsonparser/jsoncharutils.h
LIBFILES=src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp        src/stage34_unified.cpp
MINIFIERHEADERS=include/jsonparser/jsonminifier.h include/jsonparser/simdprune_tables.h
MINIFIERLIBFILES=src/jsonminifier.cpp


RAPIDJSON_INCLUDE:=dependencies/rapidjson/include
SAJSON_INCLUDE:=dependencies/sajson/include
JSON11_INCLUDE:=dependencies/json11/json11.hpp
FASTJSON_INCLUDE:=dependencies/include/fastjson/fastjson.h

LIBS=$(RAPIDJSON_INCLUDE) $(SAJSON_INCLUDE)

all: $(LIBS) $(EXECUTABLES)

test: jsoncheck numberparsingcheck stringparsingcheck
	./numberparsingcheck
	./stringparsingcheck
	./jsoncheck
	@echo 
	@tput setaf 2
	@echo "It looks like the code is good!"
	@tput sgr0


$(SAJSON_INCLUDE):
	git submodule update --init --recursive

$(RAPIDJSON_INCLUDE):
	git submodule update --init --recursive

$(JSON11_INCLUDE):
	git submodule update --init --recursive

$(FASTJSON_INCLUDE):
	git submodule update --init --recursive

bench: benchmarks/bench.cpp $(RAPIDJSON_INCLUDE) $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -I$(RAPIDJSON_INCLUDE) -Iinclude  -march=native -lm -Wall -Wextra -Wno-narrowing


parse: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)

numberparsingcheck:tests/numberparsingcheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o numberparsingcheck tests/numberparsingcheck.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp  -I. $(LIBFLAGS) -DJSON_TEST_NUMBERS


stringparsingcheck:tests/stringparsingcheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o stringparsingcheck tests/stringparsingcheck.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp  -I. $(LIBFLAGS) -DJSON_TEST_STRINGS


minifiercompetition: benchmark/minifiercompetition.cpp $(HEADERS) $(MINIFIERHEADERS) $(LIBFILES) $(MINIFIERLIBFILES)
	$(CXX) $(CXXFLAGS) -o minifiercompetition $(LIBFILES) $(MINIFIERLIBFILES) benchmark/minifiercompetition.cpp -I. $(LIBFLAGS)

minify: tools/minify.cpp $(HEADERS) $(MINIFIERHEADERS) $(LIBFILES) $(MINIFIERLIBFILES)
	$(CXX) $(CXXFLAGS) -o minify $(MINIFIERLIBFILES) $(LIBFILES) tools/minify.cpp -I. 

parsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsingcompetition $(LIBFILES) benchmark/parsingcompetition.cpp -I. $(LIBFLAGS)

allparserscheckfile: tests/allparserscheckfile.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o allparserscheckfile $(LIBFILES) tests/allparserscheckfile.cpp -I. $(LIBFLAGS)

parsehisto: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsehisto benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS) -DBUILDHISTOGRAM

clean:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)
