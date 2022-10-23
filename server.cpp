#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>

#define MAX_CLIENT 20
using namespace std;

struct playerInfo{
    string name = "";
    string email = "";
    bool isInGame = false;

    void Clear(){
        name = "";
        email = "";
        isInGame = false;
    }
};
class Game{
public:
    Game(){
    }
    void StartGame(string specificNumber = ""){
        if(specificNumber == ""){
            int random = (rand() % 9000) + 1000;
            while(!isValidNumber(random)){
                random = (rand() % 9000) + 1000;
            }
            targetNumber = to_string(random);
        }
        else
            targetNumber = specificNumber;
        
        guessTime = 0;
        isEnded = false;
    }
    string Guess(string number){
        int Anumber = 0, Bnumber = 0;
        bool visited[4] = {0};
        for(int i = 0;i<4;i++)
            visited[i] = false;

        for(int i = 0;i<4;i++){
            if(targetNumber[i] == number[i]){
                visited[i] = true;
                Anumber++;
            }
        }
        if(Anumber == 4){
            isEnded = true;
            return "You got the answer!";
        }
        for(int i = 0 ; i < 4 ; i++){
            for(int j = 0 ; j < 4 ; j++){
                if(!visited[i] && !visited[j] && targetNumber[i] == number[j]){
                    visited[i] = visited[j] = true;
                    Bnumber++;
                }
            }
        }
        guessTime++;

        if(guessTime == 5)
            isEnded = true;

        return to_string(Anumber) + "A" + to_string(Bnumber) + "B" + (guessTime == 5 ? "\nYou lose the game!" : "");

        // if(guessTime == 5)
        //     return "You lose the game!";
    }
    bool isEnded = false;
private:
    int guessTime = 0;
    string targetNumber;
    bool isValidNumber(int number){
        int a, b, c, d;
        a = (number%10);
        number /= 10;
        b = (number%10);
        number /= 10;
        c = (number%10);
        number /= 10;
        d = (number%10);
        if(a==b || b==c || c==d || d==a)
            return false;

        return true;
    }

};
class UDPProtocol{
public:
    UDPProtocol(){
        udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(udpSockfd == -1)
        {
            perror("Server failed to create UDPsocket");
            exit(EXIT_FAILURE);
        }
    }
    void Bind(){
        if(bind(udpSockfd, (const struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0){
            perror("Server fail to bind UDP");
            Close();
            exit(EXIT_FAILURE);
        }
    }
    void SetServerAddress(struct sockaddr_in* sa){
        serverAddress = sa;
    }
    void SendMessage(string mes){
        sendto(udpSockfd, mes.c_str(), mes.size(), MSG_CONFIRM, (struct sockaddr*)&clientAddress, (socklen_t)sizeof(clientAddress));
    }
    string ReceiveMessage(){
        len = sizeof(clientAddress);
        int n = recvfrom(udpSockfd, (char*)buffer, 50, MSG_WAITALL, (struct sockaddr*)&clientAddress, (socklen_t*)&len);
        buffer[n] = '\0';
        return buffer;
    }
    void Close(){
        close(udpSockfd);
    }
    int udpSockfd = 0;
private:
    struct sockaddr_in* serverAddress={0};
    struct sockaddr_in clientAddress = {0};
    char buffer[1024]={0};
    int len = 0;
    //socklen_t len = 0;
};
class TCPProtocol{
public: 
    TCPProtocol(){
        tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(tcpSockfd == -1)
        {
            perror("Server failed to create UDPsocket");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i<MAX_CLIENT + 10;i++)
            configSockfds[i] = -1;
    }
    void SetServerAddress(struct sockaddr_in* sa){
        serverAddress = sa;
    }
    void Bind(){
        if (bind(tcpSockfd, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0) {
            printf("TCP socket bind failed...\n");
            exit(0);
        }
    }
    void Listen(){
        if (listen(tcpSockfd, MAX_CLIENT) < 0) {
            printf("Listen failed...\n");
            exit(0);
        }
    }
    int Accept(){
        int newSocket = accept(tcpSockfd, (struct sockaddr*)0, (socklen_t*)(sizeof(0)));
        
        if (newSocket < 0) {
            printf("TCP server accept failed...\n");
            exit(0);
        }
        else{
            for(int i = 0;i< MAX_CLIENT + 10;i++){
                if(configSockfds[i] == -1){
                    configSockfds[i] = newSocket;
                    break;
                }
            }       
        }
        return newSocket;
    }
    void SendMessage(string mes, int clientSocketfd){
        write(configSockfds[clientSocketfd], mes.c_str(), mes.size());
    }
    string ReceiveMessage(int clientSocketfd){
        int val;
        if((val = read(configSockfds[clientSocketfd], buffer, sizeof(buffer)))==0){
            getpeername(tcpSockfd, (struct sockaddr*)serverAddress, (socklen_t*)(sizeof(*serverAddress)));
            Close();
        }
        buffer[val] = '\0';
        return buffer;
    }
    void Close(){
        close(tcpSockfd);
        tcpSockfd = 0;
        for(int i = 0;i<MAX_CLIENT + 10 ; i++)
            configSockfds[i] = -1;
    }
    int tcpSockfd = 0;
    // int configSockfd = 0;
    int configSockfds[MAX_CLIENT + 10] = {0};
private:
    struct sockaddr_in* serverAddress={0};
    struct sockaddr_in clientAddress;
    char buffer[50]={0};
    socklen_t len = 0;
};

class Server{
public:
    Server(int port){
        Port = port;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);
        
        TCP();
        UDP();
        nready = 0;
    };
    void LogicLoop(){
        while(true){

            FD_ZERO(&rset); //Clear the socket set
            FD_SET(masterTCPSocket->tcpSockfd, &rset);
	    	FD_SET(uProtocol.udpSockfd, &rset);

            // set listenfd and udpfd in readset
            for(int i = 0;i < MAX_CLIENT;i++){
                if(masterTCPSocket->configSockfds[i] < 0)
                    continue;
                FD_SET(masterTCPSocket->configSockfds[i], &rset);
            }

            int maxfdp1 = max(masterTCPSocket->tcpSockfd, uProtocol.udpSockfd);
            for(int i = 0 ; i < MAX_CLIENT ; i++){
                if(masterTCPSocket->configSockfds[i] < 0)
                    continue;
                maxfdp1 = max(masterTCPSocket->configSockfds[i], maxfdp1);
            }
            nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
            
            if(nready < 0)
                cout << "select error.\n";

            // master TCP socket
            if (FD_ISSET(masterTCPSocket->tcpSockfd, &rset))  
            {  
                int newSocket = masterTCPSocket->Accept();
                
                cout << "New connection.\n";
                string welcome_mes = "*****Welcome to Game 1A2B*****";
                //send new connection greeting message 
                if( send(newSocket, welcome_mes.c_str(), welcome_mes.size(), 0) != welcome_mes.size())  
                {  
                    perror("Failed to send welcome meassage\n");  
                }  
                // Add new socket to array of sockets 
                //AddTCPClient(newSocket); 
            }
            // UDP
            if (FD_ISSET(uProtocol.udpSockfd, &rset)) {
                string rcvmes = uProtocol.ReceiveMessage();
                HandleCommand(rcvmes);
            }
            // TCP
            for(int i = 0; i< MAX_CLIENT;i++){
                if(masterTCPSocket->configSockfds[i] == -1)
                    continue;
                if (FD_ISSET(masterTCPSocket->configSockfds[i], &rset)) {
                    string rcvmes = masterTCPSocket->ReceiveMessage(i);
                    HandleCommand(rcvmes, i);
                }
            }
        }
    };
    void HandleCommand(string command, int tcpNum = -1){
        vector<string> cmds = SplitCommand(command);
        if(tcpNum == -1){
            if(cmds[0] == "register")
                Register(cmds);
            else if(cmds[0] == "game-rule")
                GameRule();
            else
                uProtocol.SendMessage("Not a valid command.");
        }
        else if(!players[tcpNum].isInGame){
            if(cmds[0] == "login")
                Login(cmds, tcpNum);
            else if(cmds[0] == "logout")
                Logout(tcpNum);
            else if(cmds[0] == "start-game")
                StartGame(cmds, tcpNum);
            else
                masterTCPSocket->SendMessage("Not a valid command.", tcpNum);
        }
        // In Game
        else{
            string num = cmds[0];
            if(!isValidGuess(num))
                masterTCPSocket->SendMessage("Your guess should be a 4-digit number.", tcpNum);
            else{
                masterTCPSocket->SendMessage(games[tcpNum].Guess(num), tcpNum);
                if(games[tcpNum].isEnded)
                    players[tcpNum].isInGame = false;
            }
        }
    }
    int Port;

private:
    void Register(vector<string> cmds){
        if(cmds.size() != 4) {
			uProtocol.SendMessage("Usage: register <username> <email> <password>");
			return;
		}
		if(user2data.find(cmds[1]) != user2data.end()) {
			uProtocol.SendMessage("Username is already used.");
			return;
		}
		if(email2data.find(cmds[2]) != email2data.end()) {
			uProtocol.SendMessage("Email is already used.");
			return;
		}
		email2data[cmds[2]] = {cmds[1], cmds[3]};
		user2data[cmds[1]] = {cmds[2], cmds[3]};

	    uProtocol.SendMessage("Register successfully.");
    }
    void GameRule(){
        uProtocol.SendMessage("1. Each question is a 4-digit secret number.\n2. After each guess, you will get a hint with the following information:\n2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\nThe hint will be formatted as \"xAyB\".\n3. 5 chances for each question.");
    }
    void Login(vector<string> cmds, int clientIndex){
        
        if(players[clientIndex].name != "") {
			masterTCPSocket->SendMessage("Please logout first.", clientIndex);
			return;
		}
        if(cmds.size() != 3) {
			masterTCPSocket->SendMessage("Usage: login <username> <password>", clientIndex);
			return;
		}
		if(user2data.find(cmds[1]) == user2data.end()) {
			masterTCPSocket->SendMessage("Username not found.", clientIndex);
			return;
		}
		if(user2data[cmds[1]].second != cmds[2]) {
			masterTCPSocket->SendMessage("Password not correct.", clientIndex);
			return;
		}
        masterTCPSocket->SendMessage("Welcome, " + cmds[1] + ".", clientIndex);
		players[clientIndex].name = cmds[1];
    }
    void Logout(int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("Please login first.", clientIndex);
		}
		else {
			masterTCPSocket->SendMessage("Bye, " + players[clientIndex].name + ".", clientIndex);
			players[clientIndex].Clear();
		}
    }
    void StartGame(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == "") {
			masterTCPSocket->SendMessage("Please login first.", clientIndex);
			return;
		}
		if(cmds.size() > 2) {
			masterTCPSocket->SendMessage("Usage: start-game <4-digit number>", clientIndex);
			return;
		}
		if(cmds.size() > 1 && !isValidGuess(cmds[1])) {
			masterTCPSocket->SendMessage("Usage: start-game <4-digit number>", clientIndex);
			return;
		}
		masterTCPSocket->SendMessage("Please typing a 4-digit number:", clientIndex);
        games[clientIndex].StartGame(cmds.size() > 1?cmds[1]:"");
        
		
        players[clientIndex].isInGame = true;
    }
    void TCP(){
        //set master socket to allow multiple connections
        int opt = 1;
        //create a master socket 
        masterTCPSocket = new TCPProtocol();
        if(setsockopt(masterTCPSocket->tcpSockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0 )  
        {  
            perror("setsockopt");  
            exit(EXIT_FAILURE);  
        }
        masterTCPSocket->SetServerAddress(&serverAddress);
        masterTCPSocket->Bind();
        masterTCPSocket->Listen();
    }
    void UDP(){
        uProtocol;
        uProtocol.SetServerAddress(&serverAddress);
        uProtocol.Bind();       
    }
    vector<string> SplitCommand(string cmd){
        vector<string> cmds;
        string tmp = "";
        for(auto c : cmd)
        {
            if(c == ' ')
            {
                cmds.push_back(tmp);
                tmp = "";
            }
            else{
                if((int)c < 32) // Specific
                    continue;
                tmp.push_back(c);
            }
        }
        cmds.push_back(tmp);
        return cmds;
    }
    bool isValidGuess(string port){
        if(port.size() != 4)
            return false;

        for(int i = 0 ;i< 4;i++)
        {
            if((int)port[i] < '0' || (int)port[i] > '9')
            return false;
        }
        return true;
    }
    
    fd_set rset;
    int nready;

    UDPProtocol uProtocol;
    TCPProtocol* masterTCPSocket;
    struct sockaddr_in serverAddress={0};
    map<string, pair<string, string> > user2data;
    map<string, pair<string, string> > email2data;
    playerInfo players[MAX_CLIENT + 3];
    Game games[MAX_CLIENT + 3];
};
int main(int argc, char** argv) {

    if(argc < 2){
        cout << "Please input the port for server side !!\n";
        return 0;
    }
    Server* server = new Server(atoi(argv[1]));

    server->LogicLoop();

    return 0;
}