SERVER="server.cpp"
CLIENT="client.cpp"

ID="server1"
PORT="8000"
DIR="directory"

all: mserver.out server.out client.out

include: header/*

mserver.out: mserver.cpp server.cpp client.cpp include
	g++ -std=c++11 -pthread mserver.cpp -o mserver.out

server.out: mserver.cpp server.cpp client.cpp include
	g++ -std=c++11 -pthread server.cpp -o server.out

client.out: mserver.cpp server.cpp client.cpp include
	g++ -std=c++11 -pthread client.cpp -o client.out

mserver: mserver.out
	./mserver.out $(ID) $(PORT)

server: server.out
	./server.out $(ID) $(PORT)

client: client.out
	./client.out $(ID) $(PORT)

# test: