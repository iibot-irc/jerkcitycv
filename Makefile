EXECUTABLE=jerkcity

.PHONY: clean tags

all:
	g++ -o $(EXECUTABLE) --std=c++0x `pkg-config --libs --cflags opencv` *.cc -g

clean:
	rm -f $(EXECUTABLE)

tags:
	ctags -R
