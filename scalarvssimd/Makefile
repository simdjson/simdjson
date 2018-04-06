HEADERS:=include/avxprocessing.h    include/benchmark.h        include/common_defs.h      include/jsonstruct.h       include/scalarprocessing.h include/util.h
bench: benchmarks/bench.cpp $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -Iinclude  -march=native -lm -Wall -Wextra

clean:
	rm -f bench
