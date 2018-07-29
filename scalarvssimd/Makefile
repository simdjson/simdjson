HEADERS:=include/avxprocessing.h    include/benchmark.h        include/common_defs.h      include/jsonstruct.h       include/ include/util.h

RAPIDJSON_INCLUDE:=../dependencies/rapidjson/include

bench: benchmarks/bench.cpp $(RAPIDJSON_INCLUDE) $(HEADERS)
	$(CXX) -std=c++11 -O3 -o $@ benchmarks/bench.cpp -I$(RAPIDJSON_INCLUDE) -Iinclude  -march=native -lm -Wall -Wextra -Wno-narrowing

$(RAPIDJSON_INCLUDE):
	git submodule update --init --recursive

clean:
	rm -f bench
