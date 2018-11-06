CXXFLAGS = -std=c++14

bench: main.o
	g++ -o bench_start main.o -pthread -lhiredis
	
clean:
	rm bench_start main.o