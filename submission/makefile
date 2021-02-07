
psrs: sharedMemory.o FORCE
	g++ sharedMemory.o -std=c++11 -lpthread -o psrs.exe

sharedMemory.o: sharedMemory.cpp FORCE
	g++ -c sharedMemory.cpp -std=c++11 -lpthread 

clean:
	rm -f *.out *.o *.exe
	
FORCE:
