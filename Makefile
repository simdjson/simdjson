
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

#CXXFLAGS =  -g -std=c++11 -march=native -Wall -Wextra -Wshadow -Idependencies/double-conversion -Ldependencies/double-conversion/release
CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Idependencies/double-conversion -Ldependencies/double-conversion/release
LIBFLAGS = -ldouble-conversion
#CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Wno-implicit-function-declaration

EXECUTABLES=parse

EXTRA_EXECUTABLES=parsenocheesy parsenodep8

LIDDOUBLE:=dependencies/double-conversion/release/libdouble-conversion.a

LIBS=$(LIDDOUBLE)

all: $(LIBS) $(EXECUTABLES)
	-./parse

$(LIDDOUBLE) : dependencies/double-conversion/README.md
	cd dependencies/double-conversion/ && mkdir -p release && cd release && cmake .. && make

parse: main.cpp stage1_find_marks.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parse stage1_find_marks.cpp stage2_flatten.cpp stage3_ape_machine.cpp stage4_shovel_machine.cpp main.cpp $(LIBFLAGS)

parsehisto: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsehisto main.cpp $(LIBFLAGS) -DBUILDHISTOGRAM

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

parsenocheesy: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenocheesy main.cpp -DSUPPRESS_CHEESY_FLATTEN

parsenodep8: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep8 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=8

parsenodep10: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep12 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=10


parsenodep12: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep12 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=12


dependencies/double-conversion/README.md:
	git pull && git submodule init && git submodule update && git submodule status

clean:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES) $(EXTRA_EXECUTABLES)
