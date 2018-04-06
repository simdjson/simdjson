HEADERS:=avxprocessing.h    benchmark.h        common_defs.h      jsonstruct.h       scalarprocessing.h util.h
bench: bench.cpp $(HEADERS)
	$(CXX) -O3 -o $@ bench.cpp  -march=native -lm -Wall -Wextra

clean:
	rm -f bench
