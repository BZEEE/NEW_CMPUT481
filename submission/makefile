
psrs: sharedMemory.o
	g++ sharedMemory.o -std=c++11 -lpthread -o psrs

sharedMemory.o: sharedMemory.cpp
	g++ -c sharedMemory.cpp -std=c++11 -lpthread 

clean:
	rm -f *.out *.o
	