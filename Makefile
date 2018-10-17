
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11 -g2 -O3 -march=native -Wall -Wextra -Wshadow -Iinclude  -Ibenchmark/linux  -Idependencies/rapidjson/include -Idependencies/sajson/include
EXECUTABLES=parse jsoncheck numberparsingcheck minifiercompetition parsingcompetition minify allparserscheckfile

HEADERS= include/jsonparser/stringparsing.h include/jsonparser/numberparsing.h include/jsonparser/jsonparser.h include/jsonparser/common_defs.h include/jsonparser/jsonioutil.h benchmark/benchmark.h benchmark/linux/linux-perf-events.h include/jsonparser/simdjson_internal.h include/jsonparser/stage1_find_marks.h include/jsonparser/stage2_flatten.h include/jsonparser/stage34_unified.h
LIBFILES=src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp        src/stage34_unified.cpp
MINIFIERHEADERS=include/jsonparser/jsonminifier.h include/jsonparser/simdprune_tables.h
MINIFIERLIBFILES=src/jsonminifier.cpp

EXTRA_EXECUTABLES=parsenocheesy parsenodep8

RAPIDJSON_INCLUDE:=dependencies/rapidjson/include
SAJSON_INCLUDE:=dependencies/sajson/include


LIBS=$(RAPIDJSON_INCLUDE) $(SAJSON_INCLUDE)

all: $(LIBS) $(EXECUTABLES)

test: jsoncheck numberparsingcheck
	-./numberparsingcheck
	./jsoncheck

$(SAJSON_INCLUDE):
	git submodule update --init --recursive

$(RAPIDJSON_INCLUDE):
	git submodule update --init --recursive


bench: benchmarks/bench.cpp $(RAPIDJSON_INCLUDE) $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -I$(RAPIDJSON_INCLUDE) -Iinclude  -march=native -lm -Wall -Wextra -Wno-narrowing


parse: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)

numberparsingcheck:tests/numberparsingcheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o numberparsingcheck tests/numberparsingcheck.cpp  src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp  -I. $(LIBFLAGS) -DJSON_TEST_NUMBERS

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

testflatten: parse parsenocheesy parsenodep8 parsenodep10 parsenodep12
	for filename in jsonexamples/twitter.json jsonexamples/gsoc-2018.json jsonexamples/citm_catalog.json jsonexamples/canada.json ; do \
        	echo $$filename ; \
		set -x; \
		./parsenocheesy $$filename ; \
		./parse $$filename ; \
		./parsenodep8 $$filename ; \
		./parsenodep10 $$filename ; \
		./parsenodep12 $$filename ; \
		set +x; \
	done

parsenocheesy: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenocheesy benchmark/parse.cpp $(LIBFILES) -DSUPPRESS_CHEESY_FLATTEN

parsenodep8: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep8 benchmark/parse.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=8

parsenodep10: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep12 benchmark/parse.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=10

parsenodep12: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep12 benchmark/parse.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=12

clean:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)
