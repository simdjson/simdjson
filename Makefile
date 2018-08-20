
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Iinclude  -Iinclude/linux -Idependencies/double-conversion -Ldependencies/double-conversion/release
LIBFLAGS = -ldouble-conversion

EXECUTABLES=parse jsoncheck
HEADERS=include/common_defs.h include/jsonioutil.h include/linux/linux-perf-events.h include/simdjson_internal.h include/stage1_find_marks.h include/stage2_flatten.h include/stage3_ape_machine.h include/stage4_shovel_machine.h
LIBFILES=src/stage1_find_marks.cpp     src/stage2_flatten.cpp        src/stage3_ape_machine.cpp    src/stage4_shovel_machine.cpp
EXTRA_EXECUTABLES=parsenocheesy parsenodep8

LIDDOUBLE:=dependencies/double-conversion/release/libdouble-conversion.a

LIBS=$(LIDDOUBLE)

all: $(LIBS) $(EXECUTABLES)

test: jsoncheck
	./jsoncheck

$(LIDDOUBLE) : dependencies/double-conversion/README.md
	cd dependencies/double-conversion/ && mkdir -p release && cd release && cmake .. && make

parse: benchmark/parse.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) benchmark/parse.cpp $(LIBFLAGS)

jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)


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
