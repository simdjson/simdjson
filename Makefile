
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11 -g2 -O2 -march=native -Wall -Wextra -Wshadow -Iinclude  -Ibenchmark/linux -Idependencies/double-conversion -Idependencies/rapidjson/include -Ldependencies/double-conversion/release
LIBFLAGS = -ldouble-conversion
EXECUTABLES=parse jsoncheck minifiercompetition parsingcompetition 
DOUBLEEXECUTABLES=parsedouble jsoncheckdouble parsingcompetitiondouble

HEADERS=include/jsonparser/jsonparser.h include/jsonparser/common_defs.h include/jsonparser/jsonioutil.h benchmark/benchmark.h benchmark/linux/linux-perf-events.h include/jsonparser/simdjson_internal.h include/jsonparser/stage1_find_marks.h include/jsonparser/stage2_flatten.h include/jsonparser/stage3_ape_machine.h include/jsonparser/stage4_shovel_machine.h include/jsonparser/stage34_unified.h
LIBFILES=src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp        src/stage3_ape_machine.cpp    src/stage4_shovel_machine.cpp src/stage34_unified.cpp
MINIFIERHEADERS=include/jsonparser/jsonminifier.h include/jsonparser/simdprune_tables.h
MINIFIERLIBFILES=src/jsonminifier.cpp

EXTRA_EXECUTABLES=parsenocheesy parsenodep8

LIBDOUBLE:=dependencies/double-conversion/release/libdouble-conversion.a
RAPIDJSON_INCLUDE:=dependencies/rapidjson/include

LIBS=$(RAPIDJSON_INCLUDE) $(LIBDOUBLE)

all: $(LIBS) $(EXECUTABLES) $(DOUBLEEXECUTABLES)

test: jsoncheck
	./jsoncheck

$(RAPIDJSON_INCLUDE):
	git submodule update --init --recursive

$(LIBDOUBLE) : dependencies/double-conversion/README.md
	cd dependencies/double-conversion/ && mkdir -p release && cd release && cmake .. && make


bench: benchmarks/bench.cpp $(RAPIDJSON_INCLUDE) $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -I$(RAPIDJSON_INCLUDE) -Iinclude  -march=native -lm -Wall -Wextra -Wno-narrowing



parse: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)

minifiercompetition: benchmark/minifiercompetition.cpp $(HEADERS) $(MINIFIERHEADERS) $(LIBFILES) $(MINIFIERLIBFILES)
	$(CXX) $(CXXFLAGS) -o minifiercompetition $(LIBFILES) $(MINIFIERLIBFILES) benchmark/minifiercompetition.cpp -I. $(LIBFLAGS)

parsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsingcompetition $(LIBFILES) benchmark/parsingcompetition.cpp -I. $(LIBFLAGS)

parsedouble: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsedouble $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS) -DDOUBLECONV

jsoncheckdouble:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheckdouble $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS) -DDOUBLECONV

parsingcompetitiondouble: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsingcompetitiondouble $(LIBFILES) benchmark/parsingcompetition.cpp -I. $(LIBFLAGS) -DDOUBLECONV


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


dependencies/double-conversion/README.md:
	git pull && git submodule init && git submodule update && git submodule status

clean:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)
