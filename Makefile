.PHONY: all clean

all: protoc_middleman server

protoc_middleman: TransMessage.proto
	protoc --cpp_out=. TransMessage.proto
	@touch protoc_middleman

server: server.o
	g++ -o server server.o -lpthread

server.o: server.cc server.h
	g++ -std=c++11 -c server.cc

clean:
	rm -f server 
	rm -f *.o 
	rm -f protoc_middleman TransMessage.pb.h TransMessage.pb.cc

