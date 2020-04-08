ROOT=../..
SINGLEHEADER=$(ROOT)/singleheader
JSONEXAMPLES=$(ROOT)/jsonexamples

test: quickstart twitter.json
	./quickstart
quickstart: quickstart.cpp simdjson.cpp simdjson.h
	c++ -o ./quickstart quickstart.cpp simdjson.cpp
clean:
	rm -f simdjson.cpp simdjson.h twitter.json quickstart quickstart11 quickstart14
simdjson.cpp: $(SINGLEHEADER)/simdjson.cpp
	cp $(SINGLEHEADER)/simdjson.cpp .
simdjson.h: $(SINGLEHEADER)/simdjson.h
	cp $(SINGLEHEADER)/simdjson.h .
twitter.json: $(JSONEXAMPLES)/twitter.json
	cp $(JSONEXAMPLES)/twitter.json .

quickstart11: $(ROOT)/src/**.h $(ROOT)/src/**.cpp $(ROOT)/include/**.h $(ROOT)/src/**.cpp
	rm -f simdjson.h simdjson.cpp
	c++ -o ./quickstart11 quickstart.cpp $(ROOT)/src/simdjson.cpp -I$(ROOT)/src -I$(ROOT)/include -std=c++11
test11: quickstart11 twitter.json
	./quickstart11
quickstart14: $(ROOT)/src/**.h $(ROOT)/src/**.cpp $(ROOT)/include/**.h $(ROOT)/src/**.cpp
	rm -f simdjson.h simdjson.cpp
	c++ -o ./quickstart14 quickstart.cpp $(ROOT)/src/simdjson.cpp -I$(ROOT)/src -I$(ROOT)/include -std=c++14
test14: quickstart14 twitter.json
	./quickstart14
quickstart17: $(ROOT)/src/**.h $(ROOT)/src/**.cpp $(ROOT)/include/**.h $(ROOT)/src/**.cpp
	rm -f simdjson.h simdjson.cpp
	c++ -o ./quickstart17 quickstart.cpp $(ROOT)/src/simdjson.cpp -I$(ROOT)/src -I$(ROOT)/include -std=c++17
test17: quickstart17 twitter.json
	./quickstart17
