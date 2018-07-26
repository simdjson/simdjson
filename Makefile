
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow
#CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Wno-implicit-function-declaration

EXECUTABLES=parse



all: $(EXECUTABLES)
	-./parse

parse: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parse main.cpp

testflatten: parse parsenocheesy parsenodep2 parsenodep4 parsenodep6 parsenodep8 parsenodep10 parsenodep12 parsenodep14 parsenodep16
	for filename in jsonexamples/twitter.json jsonexamples/gsoc-2018.json jsonexamples/citm_catalog.json jsonexamples/canada.json ; do \
        	echo $$filename ; \
		set -x; \
		./parse $$filename ; \
		./parsenocheesy $$filename ; \
		./parsenocheesy $$filename ; \
		./parsenodep2 $$filename ; \
		./parsenodep4 $$filename ; \
		./parsenodep6 $$filename ; \
		./parsenodep8 $$filename ; \
		./parsenodep10 $$filename ; \
		./parsenodep12 $$filename ; \
		./parsenodep14 $$filename ; \
		./parsenodep16 $$filename ; \
		set +x; \
	done

parsenocheesy: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenocheesy main.cpp -DSUPPRESS_CHEESY_FLATTEN

parsenodep2: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep2 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=2

parsenodep4: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep4 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=4

parsenodep6: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep6 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=6

parsenodep8: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep8 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=8

parsenodep10: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep10 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=10

parsenodep12: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep12 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=12

parsenodep14: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep14 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=14

parsenodep16: main.cpp common_defs.h linux-perf-events.h
	$(CXX) $(CXXFLAGS) -o parsenodep16 main.cpp -DNO_PDEP_PLEASE -DNO_PDEP_WIDTH=16

clean:
	rm -f $(EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES)
