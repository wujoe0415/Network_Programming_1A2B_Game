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
    void StartGame(){
        int random = (rand() % 9000) + 1000;
        while(!isValidNumber(random))
            random = (rand() % 9000) + 1000;

        targetNumber = to_string(random);
        guessTime = 0;
    }
    string Guess(string number){
        int Anumber = 0, Bnumber = 0;
        for(int i = 0;i<4;i++){
            if(targetNumber[i] == number[i])
                Anumber++;
        }
        if(Anumber == 4)
            return "You got the answer!\n";

        for(int i = 0;i<4;i++){
            for(int j = 0;j<4;j++){
                if(targetNumber[i] == number[j])
                    Bnumber++;
            }
        }
        Bnumber -= Anumber;
        guessTime++;
        if(guessTime == 5)
            return "You lose the game!\n";

        return to_string(Anumber) + "A" + to_string(Bnumber) + "B\n";
    }
private:
    int guessTime = 0;
    string targetNumber;
    bool isValidNumber(int number){
        int a, b, c, d;
        a = (number%=10);
        b = (number%=10);
        c = (number%=10);
        d = (number%=10);

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
    void SendMessage(string mes){}
    string ReceiveMessage(){
        int n = recvfrom(udpSockfd, (char*)buffer, 50, MSG_WAITALL, 0, &len);
        buffer[n] = '\n';
        return buffer;
    }
    void Close(){
        close(udpSockfd);
    }
    int udpSockfd = 0;
private:
    struct sockaddr_in* serverAddress={0};
    char buffer[1024]={0};
    socklen_t len = 0;
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
    void Accept(){
        configSockfd = accept(tcpSockfd, (struct sockaddr*)&clientAddress, (socklen_t*)(sizeof(clientAddress)));
        if (configSockfd < 0) {
            printf("TCP server accept failed...\n");
            exit(0);
        }
        else
            printf("TCP server accept the client...\n");
    }
    void SendMessage(string mes){
        write(configSockfd, mes.c_str(), mes.size());
    }
    string ReceiveMessage(){
        int val;
        if((val = read(configSockfd, buffer, sizeof(buffer)))==0){
            getpeername(tcpSockfd, (struct sockaddr*)serverAddress, (socklen_t*)(sizeof(*serverAddress)));
            Close();
        }
        buffer[val] = '\0';
        return buffer;
    }
    void Close(){
        close(tcpSockfd);
        tcpSockfd = 0;
        configSockfd = 0;
    }
    int tcpSockfd = 0;
    int configSockfd = 0;
private:
    struct sockaddr_in* serverAddress={0};
    struct sockaddr_in clientAddress={0};
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
        FD_ZERO(&rset); //Clear the socket set
        FD_SET(masterTCPSocket->tcpSockfd, &rset);
		FD_SET(uProtocol.udpSockfd, &rset);

        int maxfdp1 = uProtocol.udpSockfd;
        for(int i = 0 ; i < tProtocols.size() ; i++)
           maxfdp1 = max(tProtocols[i].configSockfd, maxfdp1);
        maxfdp1 += 1;

        while(true){
            // set listenfd and udpfd in readset
            for(int i = 0;i < tProtocols.size();i++)
                FD_SET(tProtocols[i].tcpSockfd, &rset);

            FD_SET(uProtocol.udpSockfd, &rset);
    
            nready = select(maxfdp1, &rset, NULL, NULL, NULL);
            if(nready < 0)
                cout << "select error.\n";

            // master TCP socket
            if (FD_ISSET(masterTCPSocket->tcpSockfd, &rset))  
            {  
                masterTCPSocket->Accept();
                int newSocket = masterTCPSocket->configSockfd;
                
                cout << "New connection.\n";
                string welcome_mes = "*****Welcome to Game 1A2B*****";
                //send new connection greeting message 
                if( send(newSocket, welcome_mes.c_str(), welcome_mes.size(), 0) != welcome_mes.size())  
                {  
                    perror("Failed to send welcome meassage\n");  
                }  
                // Add new socket to array of sockets 
                AddTCPClient(newSocket); 
            }
            // UDP
            if (FD_ISSET(uProtocol.udpSockfd, &rset)) {
                string rcvmes = uProtocol.ReceiveMessage();
                HandleCommand(rcvmes);
            }
            // TCP
            for(int i = 0; i< tProtocols.size();i++){
                if (FD_ISSET(tProtocols[i].tcpSockfd, &rset)) { 
                    tProtocols[i].Accept();
                    string rcvmes = tProtocols[i].ReceiveMessage();
                    HandleCommand(rcvmes, i);

                    // string mes;
                    // getline(cin, mes);
                    // tProtocols[i].SendMessage(mes);
                    // close(tProtocols[i].configSockfd);
                }
            }
        }
    };
    void HandleCommand(string command, int tcpNum = -1){
        vector<string> cmds = SplitCommand(command);

        if(tcpNum == -1){
            if(cmds[0] == "register"){
                Register(cmds);
            }
            else if(cmds[0] == "game-rule"){
                GameRule();
            }
            else
                uProtocol.SendMessage("Not a valid command.\n");
        }
        else if(!players[tcpNum].isInGame){
            if(cmds[0] == "login"){
                cout<<"logiin\n";
            }
            else if(cmds[0] == "logout"){
                    cout<<"logout\n";
            }
            else if(cmds[0] == "start-game")
            {
                cout<<"start-game\n";
            }
            else{
                tProtocols[tcpNum].SendMessage("Not a valid command.\n");
            }
        }
        // In Game
        else{
            string num = cmds[0];
            if(!isValidGuess(num))
                tProtocols[tcpNum].SendMessage("Your guess should be a 4-digit number.");
            tProtocols[tcpNum].SendMessage(games[tcpNum].Guess(num));
        }
            
    }
    UDPProtocol uProtocol;
    vector<TCPProtocol> tProtocols;
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
			tProtocols[clientIndex].SendMessage("Please logout first.");
			return;
		}
        if(cmds.size() != 3) {
			tProtocols[clientIndex].SendMessage("Usage: login <username> <password>");
			return;
		}
		if(user2data.find(cmds[1]) == user2data.end()) {
			tProtocols[clientIndex].SendMessage("Username not found.");
			return;
		}
		if(user2data[cmds[1]].second != cmds[2]) {
			tProtocols[clientIndex].SendMessage("Password not correct.");
			return;
		}
		tProtocols[clientIndex].SendMessage("Welcome, " + cmds[1] + ".");
		players[clientIndex].name = cmds[1];
    }
    void Logout(int clientIndex){
        if(players[clientIndex].name == "") {
			tProtocols[clientIndex].SendMessage("Please login first.");
		}
		else {
			tProtocols[clientIndex].SendMessage("Bye, " + players[clientIndex].name + ".");
			players[clientIndex].Clear();
		}
    }
    void StartGame(vector<string> cmds, int clientIndex){
        if(players[clientIndex].name == "") {
			tProtocols[clientIndex].SendMessage("Please login first.");
			return;
		}
		if(cmds.size() > 2) {
			tProtocols[clientIndex].SendMessage("Usage: start-game <4-digit number>");
			return;
		}
		if(cmds.size() > 1 && !isValidGuess(cmds[1])) {
			tProtocols[clientIndex].SendMessage("Usage: start-game <4-digit number>");
			return;
		}
        games[clientIndex].StartGame();
		tProtocols[clientIndex].SendMessage("Please typing a 4-digit number:");
		
        
        players[clientIndex].isInGame = true;

		// if(comm.size() == 2) {
		// 	game[ind] = comm[1];
		// }
		// else {
		// 	int t = rand() % 10000;
		// 	game[ind] = "";
		// 	for(int i = 1; i < 10000; i *= 10) {
		// 		game[ind] += (char)('0' + t / i % 10);
		// 	}
		// }
		// cout << game[ind] << '\n';
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
        
        tProtocols;
        tProtocols.clear();

    }
    void UDP(){
        uProtocol;
        uProtocol.SetServerAddress(&serverAddress);
        uProtocol.Bind();       
    }
    void AddTCPClient(int newSocket){
        if(tProtocols.size() >= MAX_CLIENT)
        {
            cout << "Acheive Max Client Number.\n";
            return;
        }

        TCPProtocol temp;
        temp.SetServerAddress(&serverAddress);
        temp.tcpSockfd = newSocket;
        temp.configSockfd = newSocket;

        for(auto tProtocol : tProtocols){
            if(tProtocol.configSockfd == 0 && tProtocol.tcpSockfd == 0)
            {
                tProtocol = temp;
                return;
            }
        }
        tProtocols.push_back(temp);
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
    cout<<"End Loop\n";
    return 0;
}