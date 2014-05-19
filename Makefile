.PHONY: all clean

CC = g++
CFLAGS = -std=c++11 -g -Wall `pkg-config --cflags protobuf`

all: protoc_middleman xdr_middleman server client

protoc_middleman: TransMessage.proto
	protoc --cpp_out=. TransMessage.proto
	@touch protoc_middleman

xdr_middleman: TransMessage.x
	rpcgen TransMessage.x
	@touch xdr_middleman

server.o: server.c
	pkg-config --cflags protobuf
	$(CC) -c -o server.o $(CFLAGS) server.c

client.o: client.c
	$(CC) -c -o client.o $(CFLAGS) client.c

TransMessage.pb.o: TransMessage.pb.cc
	pkg-config --cflags protobuf
	$(CC) -c -o TransMessage.pb.o $(CFLAGS) TransMessage.pb.cc

TransMessage_svc.o: TransMessage_svc.c
	$(CC) -c -o TransMessage_svc.o $(CFLAGS) TransMessage_svc.c

TransMessage_clnt.o: TransMessage_clnt.c
	$(CC) -c -o TransMessage_clnt.o $(CFLAGS) TransMessage_clnt.c

TransMessage_xdr.o: TransMessage_xdr.c
	$(CC) -c -o TransMessage_xdr.o $(CFLAGS) TransMessage_xdr.c

server: server.o TransMessage.pb.o TransMessage_svc.o TransMessage_xdr.o
	$(CC) -o server server.o TransMessage.pb.o TransMessage_svc.o TransMessage_xdr.o -lpthread `pkg-config --libs protobuf`

client: client.o TransMessage.pb.o TransMessage_clnt.o TransMessage_xdr.o
	$(CC) -o client client.o TransMessage.pb.o TransMessage_clnt.o TransMessage_xdr.o `pkg-config --libs protobuf`

clean:
	rm -f server client
	rm -f *.o 
	rm -f protoc_middleman TransMessage.pb.h TransMessage.pb.cc
	rm -f xdr_middleman TransMessage.h TransMessage_svc.c TransMessage_xdr.c TransMessage_clnt.c

