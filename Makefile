server: server.o
	g++ -o server server.o -lpthread

server.o: server.c server.h
	g++ -std=c++11 -c server.c

clean:
	rm server *.o


