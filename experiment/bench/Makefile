CC = g++
CXXFLAGS = -std=c++14 -pthread -lhiredis -O3

HEADERS = $(shell find ./ -name "*.h")
CFILES = $(shell find ./ -name "*.cpp")
OBJS = $(CFILES:.c=.o)

bench: $(OBJS) $(HEADERS)
	$(CC) -o bench_start $(OBJS) -pthread -lhiredis
	
clean:
	rm -rf bench_start *.o
	
run:
	make clean
	make
	./bench_start