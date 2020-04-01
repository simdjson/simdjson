ROOT=../..
SINGLEHEADER=$(ROOT)/singleheader
JSONEXAMPLES=$(ROOT)/jsonexamples

test: quickstart twitter.json
	./quickstart
simdjson.cpp: $(SINGLEHEADER)/simdjson.cpp
	cp $(SINGLEHEADER)/simdjson.cpp .
simdjson.h: $(SINGLEHEADER)/simdjson.h
	cp $(SINGLEHEADER)/simdjson.h .
twitter.json: $(JSONEXAMPLES)/twitter.json
	cp $(JSONEXAMPLES)/twitter.json .
quickstart: quickstart.cpp simdjson.cpp simdjson.h
	c++ -o ./quickstart quickstart.cpp simdjson.cpp -std=c++17
clean:
	rm -f simdjson.cpp simdjson.h twitter.json quickstart
