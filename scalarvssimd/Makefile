HEADERS:=include/avxprocessing.h    include/benchmark.h        include/common_defs.h      include/jsonstruct.h       include/scalarprocessing.h include/util.h
bench: benchmarks/bench.cpp rapidjson/license.txt $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -Irapidjson/include -Iinclude  -march=native -lm -Wall -Wextra

rapidjson/license.txt:
	git submodule foreach git pull origin master
	

clean:
	rm -f bench
