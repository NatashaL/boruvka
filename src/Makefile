CC=g++ 
CLIBS= -lpthread
	
all: boruvkaparallel
	
time.o: time.c timer.h
	$(CC) -c -Wall time.c -w

graph_gen.o: graph_gen.cpp graph_gen.hpp
	$(CC) -c -Wall graph_gen.cpp -w

boruvkasequential.o: boruvkasequential.cpp boruvkasequential.hpp
	$(CC) -c -Wall boruvkasequential.cpp -w

boruvkaparallel: boruvkaparallel.cpp time.o graph_gen.o boruvkasequential.o
	$(CC) -O3 -o boruvkaparallel boruvkaparallel.cpp time.o graph_gen.o boruvkasequential.o $(CLIBS) -w -fpermissive

clean:
	rm -f *mp *pt *.o

distclean: clean
	rm -f *~
