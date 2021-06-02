CC = g++
CPPFLAGS = -std=c++17 -O3

CPPFILES = $(shell find . -maxdepth 2 -name "*.cpp")
OBJS = $(CPPFILES:.cpp=.o)
INCLUDES = $(CPPFILES:.cpp=.d)

all: dmck

include $(INCLUDES)

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM -MT $*.o $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

dmck: $(OBJS)
	$(CC) -o dmck_run $^ -pthread -lhiredis
	
.PHONY: stop clean run

stop:
	@../experiment/redis_test/shutdown.sh 6379 6380 6381 6382 6383 6384 6385 6386 6387 6388 6389 6390 6391 6392 6393 2>/dev/null 1>&2

clean:
	rm -rf dmck_run $(OBJS) $(INCLUDES) *.rdb *.log
	
run: dmck stop
	@rm -rf *.rdb *.log
	@./dmck_run
