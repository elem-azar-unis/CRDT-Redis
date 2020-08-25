CC = g++
CPPFLAGS = -std=c++14 -Wno-unused-result -O3

CPPFILES = $(shell find . -maxdepth 2 -name "*.cpp")
OBJS = $(CPPFILES:.cpp=.o)
INCLUDES = $(CPPFILES:.cpp=.d)

all: bench

include $(INCLUDES)

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM -MT $*.o $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

bench: $(OBJS)
	$(CC) -o bench_start $^ -pthread -lhiredis
	
.PHONY: stop clean run

stop:
	@../redis_test/shutdown.sh 6379 6380 6381 6382 6383 6384 6385 6386 6387 6388 6389 6390 6391 6392 6393 2>/dev/null 1>&2
	@sudo tc qdisc del dev lo root 2>/dev/null || true

clean:
	rm -rf bench_start $(OBJS) $(INCLUDES) *.rdb *.log
	
run: bench stop
	@rm -rf *.rdb *.log
	@./bench_start