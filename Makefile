
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

DEPSINCLUDE = -Idependencies/rapidjson/include -Idependencies/sajson/include -Idependencies/json11 -Idependencies/fastjson/src -Idependencies/fastjson/include -Idependencies/gason/src -Idependencies/ujson4c/3rdparty -Idependencies/ujson4c/src
CXXFLAGS =  -std=c++11  -march=native -Wall -Wextra -Wshadow -Iinclude  -Ibenchmark/linux  $(DEPSINCLUDE)    
CFLAGS = -march=native  -Idependencies/ujson4c/3rdparty -Idependencies/ujson4c/src
ifeq ($(SANITIZE),1)
	CXXFLAGS += -g3 -O0  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
	CFLAGS += -g3 -O0  -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
else
	CXXFLAGS += -O3
	CFLAGS += -O3
endif

MAINEXECUTABLES=parse minify
TESTEXECUTABLES=jsoncheck numberparsingcheck stringparsingcheck 
COMPARISONEXECUTABLES=minifiercompetition parsingcompetition  allparserscheckfile

HEADERS= include/simdjson/simdutf8check.h include/simdjson/stringparsing.h include/simdjson/numberparsing.h include/simdjson/jsonparser.h include/simdjson/common_defs.h include/simdjson/jsonioutil.h benchmark/benchmark.h benchmark/linux/linux-perf-events.h include/simdjson/parsedjson.h include/simdjson/stage1_find_marks.h include/simdjson/stage2_flatten.h include/simdjson/stage34_unified.h include/simdjson/jsoncharutils.h include/simdjson/jsonformatutils.h
LIBFILES=src/jsonioutil.cpp src/jsonparser.cpp src/stage1_find_marks.cpp     src/stage2_flatten.cpp        src/stage34_unified.cpp
MINIFIERHEADERS=include/simdjson/jsonminifier.h include/simdjson/simdprune_tables.h
MINIFIERLIBFILES=src/jsonminifier.cpp


RAPIDJSON_INCLUDE:=dependencies/rapidjson/include
SAJSON_INCLUDE:=dependencies/sajson/include
JSON11_INCLUDE:=dependencies/json11/json11.hpp
FASTJSON_INCLUDE:=dependencies/include/fastjson/fastjson.h
GASON_INCLUDE:=dependencies/gason/src/gason.h
UJSON4C_INCLUDE:=dependencies/ujson4c/src/ujdecode.c

LIBS=$(RAPIDJSON_INCLUDE) $(SAJSON_INCLUDE) $(JSON11_INCLUDE) $(FASTJSON_INCLUDE) $(GASON_INCLUDE) $(UJSON4C_INCLUDE)
OBJECTS=ujdecode.o
all:  $(MAINEXECUTABLES)

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

$(GASON_INCLUDE):
	git submodule update --init --recursive

$(UJSON4C_INCLUDE):
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

ujdecode.o: $(UJSON4C_INCLUDE)
	$(CC) $(CFLAGS) -c dependencies/ujson4c/src/ujdecode.c 

parsingcompetition: benchmark/parsingcompetition.cpp $(HEADERS) $(LIBFILES) $(OBJECTS)
	$(CXX) $(CXXFLAGS)  -o parsingcompetition $(LIBFILES) benchmark/parsingcompetition.cpp $(OBJECTS) -I. $(LIBFLAGS)

allparserscheckfile: tests/allparserscheckfile.cpp $(HEADERS) $(LIBFILES) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o allparserscheckfile $(LIBFILES) tests/allparserscheckfile.cpp $(OBJECTS) -I. $(LIBFLAGS)

parsehisto: benchmark/parse.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsehisto benchmark/parse.cpp $(LIBFILES) $(LIBFLAGS) -DBUILDHISTOGRAM

cppcheck:
	cppcheck --enable=all src/*.cpp  benchmarks/*.cpp tests/*.cpp -Iinclude -I. -Ibenchmark/linux 


clean:
	rm -f $(OBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES)

cleandist:
	rm -f $(OBJECTS) $(MAINEXECUTABLES) $(EXTRA_EXECUTABLES) $(TESTEXECUTABLES) $(COMPARISONEXECUTABLES)
