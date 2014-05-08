server: server.o
	g++ -o server server.o -lpthread

server.o: server.cc server.h
	g++ -std=c++11 -c server.cc

clean:
	rm server *.o


