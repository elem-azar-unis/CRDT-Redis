CXXFLAGS = -std=c++14 -pthread -lhiredis -O3

bench: main.o queue_log.o constants.h generator.h queue_log.h
	g++ -o bench_start main.o queue_log.o -pthread -lhiredis
	
clean:
	rm -rf bench_start *.o
	
run:
	make clean
	make
	./bench_start