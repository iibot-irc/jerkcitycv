EXECUTABLE=jerkcity

all:
	g++ -O3 `pkg-config --libs --cflags opencv` *.cc -o $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE)
