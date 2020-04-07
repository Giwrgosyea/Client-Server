all: client server

client: client.o
	g++ client.o -o client

client.o: client.cpp
	g++ -c client.cpp

server: server.o 
	g++ server.o -o server -pthread

server.o: server.cpp 
	g++ -c server.cpp -pthread


clean:
	rm -rf *o

