CXX = g++
CXXFLAGS = -std=c++14 -Wno-unused-result -O3

HEADERS = $(shell find . -maxdepth 2 -name "*.h")
CFILES = $(shell find . -maxdepth 2 -name "*.cpp")
OBJS = $(CFILES:.cpp=.o)

bench: $(OBJS) $(HEADERS)
	$(CXX) -o bench_start $(OBJS) -pthread -lhiredis
	
.PHONY: stop clean run

stop:
	../redis_test/shutdown.sh 6379 6380 6381 6382 6383 6384 6385 6386 6387 6388 6389 6390 6391 6392 6393
	sudo tc qdisc del dev lo root

clean:
	rm -rf bench_start $(OBJS) *.rdb *.log
	
run: bench
	rm -rf *.rdb *.log
	./bench_start