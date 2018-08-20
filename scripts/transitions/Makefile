all:../../include/transitions.h

../../include/transitions.h: generatetransitions
	./generatetransitions > ../../include/jsonparser/transitions.h

generatetransitions:  generatetransitions.cpp
	$(CXX) -o generatetransitions  generatetransitions.cpp  -I../../include

clean:
	rm -f generatetransitions
