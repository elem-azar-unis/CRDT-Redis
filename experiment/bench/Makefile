CXX = g++
CXXFLAGS = -std=c++14 -Wno-unused-result -O3

HEADERS = $(shell find ./ -maxdepth 2 -name "*.h")
CFILES = $(shell find ./ -maxdepth 2 -name "*.cpp")
OBJS = $(CFILES:.cpp=.o)

bench: $(OBJS) $(HEADERS)
	$(CXX) -o bench_start $(OBJS) -pthread -lhiredis
	
.PHONY: clean run
clean:
	rm -rf bench_start $(OBJS)
	
run:
	make clean
	make
	./bench_start