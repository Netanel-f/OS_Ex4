
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <map>
#include <iostream>
#include <algorithm>
#include <errno.h>
#include "whatsappio.h"

//// ============================   Constants =====================================================
static const int MAX_QUEUE = 10;
static const int MAX_HOST_NAME_LEN = 30;
static const int MAX_GROUP_NAME_LEN = 30;   //todo delete
static const int BUFF_SIZE = 256;   //todo delete
static const int MAX_MESSAGE_LENGTH = 256;  //todo delete
static const int FUCK = 1; // todo exit val?    //todo delete


//// ===========================  Structs & Typedefs ==============================================


// command
struct Command { //todo make init?
    command_type type;
    std::string name;
    std::string message;
    std::vector<std::string> clients;
    std::string sender;
    int senderSockfd;
};


typedef std::map<std::string, int> clientsDB;
typedef std::vector<std::string> GroupMembers1;
typedef std::map<std::string, GroupMembers1> GroupsDB1;


//todo  ===========================   TODOS ===============================================


//// ============================  Class Declarations =============================================

/**
 * Class representing running instance of Server.
 */
class Server {

    std::string serverName;
    unsigned short serverPort;
    int welcomeSocket;


    clientsDB clients1;
    GroupsDB1 groups1;
    char readBuf[WA_MAX_INPUT+1];
    char writeBuf[WA_MAX_INPUT+1];

    fd_set clientsfds;
    fd_set readfds; //Represent a set of file descriptors.

public:

    //// C-tor
    Server(unsigned short portNumber);

    //// server actions
    void selectPhase();
    bool shouldTerminateServer();
    void handleClientRequest(int sockfd);
    void writeToClient(int sockfd, const std::string& command);

private:

    //// send/recv

    //// DB queries
    bool isClient(std::string& name);
    bool isGroup(std::string& name);
    int getMaxfd();
    std::string getClientNameById(int sockfd);


    //// request handling
    void createGroup(Command cmd);
    void send(Command cmd);
    void who(Command cmd);
    void clientExit(Command cmd);
    void registerClient(Command cmd);
    void tempregisterClient(int sockfd);

    //// name legality
    bool isTakenName(std::string& name);

    void killAllSockets();
    void killServer();

};

//// ===============================  Forward Declarations ============================================

//// input checking
int parsePortNum(int argc, char** argv);

//// errors
//void errCheck(int& retVal, const std::string& funcName, int successVal = 0);

//// ===============================  Class Server ============================================

//// server actions

Server::Server(unsigned short portNumber) {

    char srvName[MAX_HOST_NAME_LEN+1];
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent* hostEnt;

    int retVal = gethostname(srvName, MAX_HOST_NAME_LEN);
    if (retVal < 0) {
        print_error("gethostname", errno);
        exit(FUCK); //todo should we exit?
    }

    bzero(&sa, sizeof(struct sockaddr_in));
    hostEnt = gethostbyname(srvName);
    if (hostEnt == nullptr) {
        print_error("gethostbyname", errno);
        exit(FUCK); //todo should we exit?
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = (sa_family_t) hostEnt->h_addrtype;
    memcpy(&sa.sin_addr, hostEnt->h_addr, (size_t) hostEnt->h_length);
    sa.sin_port = htons(portNumber);

    welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (welcomeSocket < 0) { print_error("socket", errno); }

    retVal = bind(welcomeSocket, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
    if (retVal < 0) {
        print_error("bind", errno);
    }

    retVal = listen(welcomeSocket, MAX_QUEUE);

    if (retVal < 0) { print_error("listen", errno); }

    std::string srv_str(srvName);
    this->serverName = srv_str;
    this->serverPort = portNumber;
    bzero(this->readBuf, WA_MAX_INPUT +1);
    bzero(this->writeBuf, WA_MAX_INPUT +1);
}

void Server::selectPhase() {
    //fd_set clientsfds; //todo J - moved this to be fields for later access
    //fd_set readfds; //Represent a set of file descriptors.
    FD_ZERO(&clientsfds);   //Initializes the file descriptor set fdset to have zero bits for all file descriptors

    FD_SET(welcomeSocket, &clientsfds);  //Sets the bit for the file descriptor fd in the file descriptor set fdset.
    FD_SET(STDIN_FILENO, &clientsfds);

    int retVal;
    int maxfds;
    bool keepLoop = true;
    while (keepLoop) {
        readfds = clientsfds;
        maxfds = this->getMaxfd();
        retVal = select((maxfds + 1), &readfds, nullptr, nullptr, nullptr);
        if (retVal == -1) {
            print_error("select", errno);
            continue;
        }
        else if (retVal == 0) {
            continue;
        }
        //Returns a non-zero value if the bit for the file descriptor fd is set in the file descriptor set pointed to by fdset, and 0 otherwise
        if (FD_ISSET(welcomeSocket, &readfds)) {

            //will also add the client to the clientsfds
            int connectionSocket = accept(welcomeSocket, nullptr, nullptr);
            if (connectionSocket < 0) {
                print_error("accept", errno);
            } else {
                FD_SET(connectionSocket, &clientsfds);
                tempregisterClient(connectionSocket);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            //msg from stdin
            keepLoop = !shouldTerminateServer();
            continue; // todo J changed from break
        }

        else {
            //will check each client if it’s in readfds /todo what does this mean??
            //and then receive a message from him
            for (auto client:this->clients1) {
                if (FD_ISSET(client.second, &readfds)) {
                    handleClientRequest(client.second);
                }
            }
        }
    }
}


bool Server::shouldTerminateServer() {
    std::string serverInput;
    getline(std::cin, serverInput);
    if (serverInput=="EXIT") {
        killAllSockets();
        print_exit();
        return true;
    }
    else {
//        print_invalid_input();    //todo verify that no need to print.
        return false;
    }
}

void Server::writeToClient(int sockfd, const std::string& command) {
//    char buf[WA_MAX_INPUT + 1];
//    bzero(buf, WA_MAX_INPUT + 1);
    strcpy(this->writeBuf, command.c_str()); //todo J maybe n copy

    int bcount = 0; /* counts bytes read */
    int br = 0; /* bytes read this pass */
    while (bcount < WA_MAX_INPUT) { /* loop until full buffer */
        br = (int) write(sockfd, this->writeBuf, (size_t) WA_MAX_INPUT - bcount);
        if (br > 0) {
            bcount += br;
        }
        if (br < 1) {
            print_error("write", errno);
        }
    }
}

void Server::killAllSockets() {
    std::string terminateCmd = "terminated";
    for (auto client:this->clients1) {
        // tell clients that server is terminated.
        this->writeToClient(client.second, terminateCmd);
        //close sockets
        if (close(client.second)!=0) {
            print_error("close", errno);
        }
    }

}

void Server::handleClientRequest(int sockfd) {
    // read from socket

    int bcount = 0; /* counts bytes read */
    int br = 0; /* bytes read this pass */
//    char * buf;
//    bzero(buf, WA_MAX_INPUT + 1);
    while (bcount < WA_MAX_INPUT) { /* loop until full buffer */
        br = (int)read(sockfd, this->readBuf, (size_t) WA_MAX_INPUT - bcount);
        if (br > 0) {
            bcount += br;
//            buf += br;
        }
        if (br == -1) { //todo J bcz read 0 kept happening
            print_error("read", errno);
        }
    }

    std::string incomingMsg = this->readBuf; //todo BUG - this is often empty after connect

    Command cmd;

    cmd.type=INVALID;
    cmd.name="";
    cmd.message="";
    cmd.clients.clear();

    parse_command(incomingMsg, cmd.type, cmd.name, cmd.message, cmd.clients); //todo invalid
    // input on all
    cmd.sender = getClientNameById(sockfd);
    cmd.senderSockfd = sockfd;

    switch (cmd.type) {
        case CREATE_GROUP:
            createGroup(cmd);
            break;
        case SEND:
            send(cmd);
            break;
        case WHO:
            who(cmd);
            break;
        case CONNECT:
            registerClient(cmd);
            break;
        case EXIT:
            clientExit(cmd);
            break;
        case INVALID:
            print_invalid_input();
            break;
        case MESSAGE:
            print_invalid_input();
            break;
        case TERMINATED:
            print_invalid_input();
            break;
    }

};

//// send/recv

//void Server::prepSize(uint64_t size, Client client) {
//    uint64_t datalen = size;
//    ssize_t written = write(client.sockfd, &datalen, sizeof(uint64_t));
//    if (written!=c.sender.size()) {
//        print_error("write", errno);
//    }
//}

//void Server::strToClient(const std::string& str, const std::string& clientName) {
//    Client client = clients[clientName];
//
//    // send string size
//    prepSize(str.size(), client);
//
//    // send string
//    ssize_t written = write(client.sockfd, &str, str.size());
//    if (written!=str.size()) { //todo J is correct error checking?
//        print_error("write", errno);
//    }
//}


//// DB modify
void Server::tempregisterClient(int sockfd) {
    this->clients1.emplace(std::pair<std::string, int>("@" + std::to_string(sockfd), sockfd));
}

void Server::registerClient(Command cmd) {
    this->clients1.erase("@" + std::to_string(cmd.senderSockfd));
    if (isTakenName(cmd.name)) {
        // notify failure to client
        writeToClient(cmd.senderSockfd, "connect D");
        return;
    }
    this->clients1.emplace(std::pair<std::string, int>(cmd.name, cmd.senderSockfd));

    // print success on server
    print_connection_server(cmd.name);
    // notify success to client
    writeToClient(cmd.senderSockfd, "connect T");
}

//// DB queries

//bool Server::isClient(std::string& name) {
//    return ((bool) clients.count(name)); // (count is zero (false) if not there.
//}

bool Server::isClient(std::string& name) {
    return ((bool) this->clients1.count(name)); // (count is zero (false) if not there.
}

bool Server::isGroup(std::string& name) {
    return ((bool) this->groups1.count(name)); // (count is zero (false) if not there.
}

int Server::getMaxfd() {
    int max = this->welcomeSocket;
    for (auto client :this->clients1) {
        if (client.second > max) { max = client.second; }
    }
    return max;
}

std::string Server::getClientNameById(int sockfd) {
    for (auto client:this->clients1){
        if (client.second == sockfd) {
            return client.first;
        }
    }
}



//// request handling

void Server::createGroup(Command cmd) {

    // ensure group name is unique (not taken)
    if (isTakenName(cmd.name)) {
        //print failure on server
        print_create_group(true, false, cmd.sender, cmd.name);

        //report failure to client
        writeToClient(cmd.senderSockfd, "create_group F");
        return;
    }

    // make set of clients (allowing duplicates) and ensuring all clients exists
    GroupMembers1 members = GroupMembers1();

    // add each client
    for (std::string strName : cmd.clients) {
        if (isClient(strName)) {
            members.push_back(strName);
        }
    }

    // add sender to set even if unspecified
    members.push_back(cmd.sender);

    // ensure group has at least 2 members (including creating client)
    if (members.size() < 2) {

        //print failure on server
        print_create_group(true, false, cmd.sender, cmd.name);

        //report failure to client
        writeToClient(cmd.senderSockfd, "create_group F");
    }

    // add this group to DB
    groups1.emplace(std::pair<std::string, GroupMembers1>(cmd.name, members));

    //print success on server
    print_create_group(true, true, cmd.sender, cmd.name);
    //report success to client
    writeToClient(cmd.senderSockfd, "create_group T " + cmd.name);

}

void Server::send(Command cmd) {

    std::string message = "message " + cmd.sender + " " + cmd.message;

    // if name in clients
    if (isClient(cmd.name)) {

        // message the recipient client
        writeToClient(this->clients1[cmd.name], message);

        // notify sender of success
        writeToClient(cmd.senderSockfd, "send T");
        // print success server
        print_send(true, true, cmd.sender, cmd.name, cmd.message);

    } else if (isGroup(cmd.name)) {   // if name in groups

        // ensure caller is in this group
        bool senderInGroup = false;
        for (const std::string & memberName : groups1.at(cmd.name)) {
            if (!(memberName.compare(cmd.sender))) {
                senderInGroup = true;
            }
        }

        if (senderInGroup) {
            for (const std::string & memberName : groups1.at(cmd.name)) {
                if (!(memberName.compare(cmd.sender))) {
                    // notify sender of success
                    writeToClient(cmd.senderSockfd, "send T");
                } else {
                    // message each recipient client
                    writeToClient((this->clients1[memberName]), message);
                }
            }
            
            // print success server
            print_send(true, true, cmd.sender, cmd.name, cmd.message);
            return;
        } else {
            // notify sender of failure
            writeToClient(cmd.senderSockfd, "send F");
            // print fail server
            print_send(true, false, cmd.sender, cmd.name, cmd.message);
        }
    } else { // else error
        // notify sender of failure
        writeToClient(cmd.senderSockfd, "send F");
        // print fail server
        print_send(true, false, cmd.sender, cmd.name, cmd.message);
    }
}

void Server::who(Command cmd) {
    //// order and return names
    std::vector<std::string> namesVec;

    // get all names
    for (auto client: this->clients1) {
        namesVec.emplace_back(client.first);
    }

    std::sort(namesVec.begin(), namesVec.end());

    std::string list = "who T ";

    for (const std::string & name: namesVec) {
        list += name+",";
    }

    //send list to printing
    writeToClient(cmd.senderSockfd, list);
    // print who server
    print_who_server(cmd.sender);
}

void Server::clientExit(Command cmd) {

    // remove sender from all groups
    for (auto group : groups1) {
        group.second.erase(std::remove(group.second.begin(), group.second.end(), cmd.sender),
                           group.second.end());
    }




    // send success to client
    writeToClient(cmd.senderSockfd, "exit T ");

    //close sockets
    if (close(cmd.senderSockfd) != 0) {
        print_error("close", errno);
    }


    // remove sender from server (after reported success) and from fd_set
    this->clients1.erase(cmd.sender);
    FD_CLR(cmd.senderSockfd, &(this->clientsfds));

    //print success to server
    print_exit(true, cmd.sender);

    //todo remove user from fd.set

}

bool Server::isTakenName(std::string& name) {
    // ensure alphanumeric only and name not taken.
    return (isClient(name) || isGroup(name));
}


//// ===============================  Helper Functions ============================================

//// input checking
int parsePortNum(int argc, char** argv) {

    //// check args
    if (argc!=2) {
        print_server_usage();  //todo server shouldnt crash upon receiving illegal requests?
        exit(1);
    }

    int portNumber = atoi(argv[1]);

    //todo verify if needed
//    if (portNumber<0 || portNumber>65535) {
//        print_server_usage();
//        exit(1);
//    }

    return portNumber;
}


//// =============================== Main Function ================================================

int main(int argc, char* argv[]) {

    //// --- Init  ---
    int portNumber = parsePortNum(argc, argv);
    //// init Server
    Server server((unsigned short) portNumber);  // todo J is conversion ok? maybe cast inside parse

    //// --- Setup  ---
    //// create socket
    //// bind to an ip adress and port

    //// --- Wait  ---
    //// listen
    //// accept
    server.selectPhase();
    exit(0);

    //// --- Get Request  ---
    //// read
    //// parse

    //// --- Process  ---
    //// write
    //// close

    //// --- Repeat  ---
}