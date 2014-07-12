CXX=clang++
EXECUTABLE=jerkcity
CFLAGS=--std=c++0x -O3 -g
LDFLAGS=-lboost_program_options -lboost_filesystem -lboost_system `pkg-config --libs opencv`

.PHONY: clean tags

all:
	$(CXX) -o $(EXECUTABLE) $(CFLAGS) *.cc $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)

tags:
	ctags -R