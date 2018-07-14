
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h


.PHONY: clean cleandist

CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow
#CXXFLAGS =  -std=c++11 -O2 -march=native -Wall -Wextra -Wshadow -Wno-implicit-function-declaration

EXECUTABLES=parse



all: $(EXECUTABLES)

parse: main.cpp common_defs.h  vecdecode.h
	$(CXX) $(CXXFLAGS) -o parse main.cpp


clean:
	rm -f $(EXECUTABLES)

cleandist:
	rm -f $(EXECUTABLES)
