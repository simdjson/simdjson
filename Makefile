
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

#CXXFLAGS =  -g -std=c++11 -march=native -Wall -Wextra -Wshadow -Idependencies/double-conversion -Ldependencies/double-conversion/release
CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Idependencies/double-conversion -Ldependencies/double-conversion/release
LIBFLAGS = -ldouble-conversion
#CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Wno-implicit-function-declaration

EXECUTABLES=parse jsoncheck
HEADERS=common_defs.h jsonioutil.h linux-perf-events.h simdjson_internal.h stage1_find_marks.h stage2_flatten.h stage3_ape_machine.h stage4_shovel_machine.h
LIBFILES=stage1_find_marks.cpp     stage2_flatten.cpp        stage3_ape_machine.cpp    stage4_shovel_machine.cpp
EXTRA_EXECUTABLES=parsenocheesy parsenodep8

LIDDOUBLE:=dependencies/double-conversion/release/libdouble-conversion.a

LIBS=$(LIDDOUBLE)

all: $(LIBS) $(EXECUTABLES)

test: jsoncheck
	./jsoncheck

$(LIDDOUBLE) : dependencies/double-conversion/README.md
	cd dependencies/double-conversion/ && mkdir -p release && cd release && cmake .. && make

parse: main.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parse $(LIBFILES) main.cpp $(LIBFLAGS)

jsoncheck:tests/jsoncheck.cpp $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o jsoncheck $(LIBFILES) tests/jsoncheck.cpp -I. $(LIBFLAGS)


parsehisto: main.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsehisto main.cpp $(LIBFILES) $(LIBFLAGS) -DBUILDHISTOGRAM

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

parsenocheesy: main.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenocheesy main.cpp $(LIBFILES) -DSUPPRESS_CHEESY_FLATTEN

parsenodep8: main.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep8 main.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=8

parsenodep10: main.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep12 main.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=10


parsenodep12: main.cpp  $(HEADERS) $(LIBFILES)
	$(CXX) $(CXXFLAGS) -o parsenodep12 main.cpp $(LIBFILES) -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=12


dependencies/double-conversion/README.md:
	git pull && git submodule init && git submodule update && git submodule status

clean:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)
