server: server.o
	gcc -o server server.o -lpthread

server.o: server.c
	gcc -c server.c

clean:
	rm server *.o


