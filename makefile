all : client server
	g++ client.cpp -o client
	g++ server.cpp -o server