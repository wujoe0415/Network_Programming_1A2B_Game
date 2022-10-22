#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

using namespace std;
class TCPConnection {
public:
	TCPConnection() {
		TCP_fd = socket(AF_INET, SOCK_STREAM, 0);
	}
	void ReceiveMessage() {

		int val = read(TCP_fd, buffer, 1024);
		buffer[val] = '\0';
		cout << buffer << '\n';
	}
	void SendMessage(string str) {
		puts(("TCP sendMessage : " + str).c_str());
		send(TCP_fd, str.c_str(), str.size(), 0);

	}
	void Close() {
		close(TCP_fd);
	}
    void Connect(){
        if (connect(TCP_fd, (const sockaddr*)&tcpClientAddress, sizeof(tcpClientAddress)) != 0) {
            cout << "TCP Client fail to connect with the server \n";
            exit(0);
        }
    }
	void SetAddress(struct sockaddr_in sa)
	{
		tcpClientAddress = sa;
	}
private:
	int TCP_fd;
    struct sockaddr_in tcpClientAddress={0};
    char buffer[1024]={0};
    socklen_t adrlen = 0;
};
class UDPConnection {
public:
	UDPConnection() {
		UDP_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(UDP_fd < 0) {
			cout << "Client UDP fail to creating\n";
			exit(EXIT_FAILURE);
		}
	}
	void SendMessage(string str) {
		puts(("UDP sendMessage : " + str).c_str());
		sendto(UDP_fd, str.c_str(), str.size(), 0, (struct sockaddr*)&udpClientAddress, sizeof(udpClientAddress));
	}
	void ReceiveMessage() {
		int val = recvfrom(UDP_fd, buffer, 1024, 0, (struct sockaddr*)&udpClientAddress, (socklen_t*)&adrlen);
		buffer[val] = '\0';
		cout << buffer << '\n';
	}
    void Close() {
		close(UDP_fd);
	}
	void SetAddress(struct sockaddr_in sa){
		udpClientAddress = sa;
	}
private:
	int UDP_fd;
    struct sockaddr_in udpClientAddress={0};
    char buffer[1024]={0};
    socklen_t adrlen = 0;
};

int main(int argc, char** argv) {
	if (argc < 3) {
		cout << "Please input the port and IP for client side !!\n";
		return 0;
	}
	int Port = atoi(argv[2]); // extract the port
	
	sockaddr_in ClientAddress;
	ClientAddress.sin_family = AF_INET;
	ClientAddress.sin_port = htons(Port);
	if(inet_pton(AF_INET, argv[1], &ClientAddress.sin_addr) <= 0) {
		cout << "\nInvalid address/ Address not supported\n";
		return 0;
	}
	UDPConnection udp;
	udp.SetAddress(ClientAddress);

	TCPConnection tcp; 
	tcp.SetAddress(ClientAddress);
	tcp.Connect();
	// in this place
	tcp.ReceiveMessage();


	string command;
	while(getline(cin, command)) {
		string tmp = "";
		for(int i = 0; i < command.size(); i ++) {
			if(command[i] == ' ') {
				break;
			}
			tmp += command[i];
		}
		if(tmp == "exit") {
			udp.Close();
			tcp.Close();
			break;
		}
		else if(tmp=="register" || tmp == "game-rule") 
		{
			udp.SendMessage(command);
			udp.ReceiveMessage();
		}
		else {
			tcp.SendMessage(command);
			tcp.ReceiveMessage();
		}
	}
	return 0;
}
