CXX=clang++
EXECUTABLE=jerkcity
CFLAGS=--std=c++1y -O3 -g -Wall -Wextra
LDFLAGS=-lboost_program_options -lboost_filesystem -lboost_system `pkg-config --libs opencv`

.PHONY: clean tags

all:
	$(CXX) -o $(EXECUTABLE) $(CFLAGS) *.cc $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)

tags:
	ctags -R
