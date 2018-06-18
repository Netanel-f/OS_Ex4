
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

// Todo: check sucess, listed for EXIT,

//// ============================   Constants =====================================================
static const int MAX_QUEUE = 10;
static const int MAX_HOST_NAME_LEN = 30;
static const int MAX_GROUP_NAME_LEN = 30;
static const int BUFF_SIZE = 256;
static const int MAX_MESSAGE_LENGTH = 256;
static const int FUCK = 1; // todo exit val?


//// ===========================  Structs & Typedefs ==============================================


// client
struct Client {
  std::string name;
  int sockfd;
};

// clients Group
struct Group {
  std::string name;
  std::map<std::string, Client *> groupMembers;
};

// command
struct Command { //todo make init?
  command_type type;
  std::string name;
  std::string message;
  std::vector<std::string> clients;
  std::string sender;
};




typedef std::pair<std::string, Client *> ClientPair;
typedef std::pair<std::string, Group *> GroupPair;


//// ===========================   Global Variables ===============================================
//todo J there should probably be none for thread safety

//// ============================  Class Declarations =============================================

/**
 * Class representing running instance of Server.
 */
class Server{

    std::string serverName;
    unsigned short serverPort;
    int welcomeSocket;

    std::string commandStr;
    Command c;

    std::map<std::string, Client *> clients;
    std::map<std::string, Group *> groups;

public:

    //// C-tor
    explicit void Server(unsigned short portNumber);

    //// server actions
    void selectPhase();
    void connectNewClient();
    void serverStdInput();
    void handleClientRequest();

private:
//// DB modify
 private:
    //// send/recv
    void prepSize(uint64_t size, Client *client);

    void MessageToClient(std::string message, const std::string &clientName);
    void successToClient(bool success, const std::string &clientName);

    void whoToClient(std::vector<std::string> sortedVec, const std::string &clientName);

    //// DB modify
    void registerClient(std::string &name);

    //// DB queries
    bool isClient(std::string &name);
    bool isGroup(std::string &name);

    //// request handling
    void createGroup(Command c);
    void send(Command c);
    void who(Command c);
    void clientExit(Command c);

    //// name legality
    bool isLegalGroupOrClientName(std::string &name);
    bool isAlNumString(std::string &str);

};

//// ===============================  Forward Declarations ============================================

//// input checking
int parsePortNum(int argc, char **argv);

//// errors
void errCheck(int &retVal, const std::string &funcName, int successVal = 0);

//// ===============================  Class Server ============================================

//// server actions
//todo when print_error should we exit the program?
void Server::Server(unsigned short portNumber) {

    char srvName[MAX_HOST_NAME_LEN + 1];
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent *hostEnt;

    int retVal = gethostname(srvName, MAX_HOST_NAME_LEN);
    if (retVal < 0) { print_error("gethostname", errno); }

    bzero(&sa,sizeof(struct sockaddr_in));
    hostEnt = gethostbyname(srvName);
    if (hostEnt == nullptr) {
        print_error("gethostbyname", errno);
        //todo should we exit?
    }

    memset(&sa, 0,sizeof(struct sockaddr_in));
    sa.sin_family = hostEnt->h_addrtype;
    memcpy(&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_port = htons(portNumber);

    welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (welcomeSocket < 0){ print_error("socket", errno); }


    retVal = bind(welcomeSocket, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
    if (retVal < 0) {
        print_error("bind", errno);
    }


    retVal = listen(welcomeSocket, MAX_QUEUE);
    if (retVal < 0) { print_error("listen", errno); }

    // todo initialise fields
    strcpy(serverName, srvName);
    serverPort = portNumber;
}


void Server::selectPhase() {
    fd_set clientsfds;
    fd_set readfds; //Represent a set of file descriptors.
    FD_ZERO(&clientsfds);   //Initializes the file descriptor set fdset to have zero bits for all file descriptors

    FD_SET(welcomeSocket, &clientsfds);  //Sets the bit for the file descriptor fd in the file descriptor set fdset.


    FD_SET(STDIN_FILENO, &clientsfds);
    int retVal;
    while (true) {
        readfds = clientsfds;
        retVal = select(MAX_QUEUE+2, &readfds, nullptr, nullptr, nullptr);
        if (retVal == -1) {
            errCheck(retVal, "select");
            //todo terminate server and return -1;
        }else if (retVal == 0) {
            continue;
        }
        //Returns a non-zero value if the bit for the file descriptor fd is set in the file descriptor set pointed to by fdset, and 0 otherwise
        if (FD_ISSET(welcomeSocket, &readfds)) {
            //will also add the client to the clientsfds
            connectNewClient();
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            //msg from stdin
            serverStdInput();
        }

        else {
            //will check each client  if it’s in readfds
            //and then receive a message from him
            handleClientRequest();
        }
        break;//todo remove this!!!!!!!
    }
}

void Server::connectNewClient(){
    //todo do all

    std::string name; //todo get name

    registerClient(name);
}

void Server::serverStdInput(){
    //todo wait for EXIT somehow (Select)
}

void Server::handleClientRequest(){
    commandStr = NULL; //todo read
    // todo clear old command?
    parse_command(commandStr, c.type, c.name, c.message, c.clients);
    c.sender = "get sender"; //todo
    switch (c.type) { //todo
        case CREATE_GROUP:createGroup(c);
            break;

        case SEND:send(c);
            break;

        case WHO:who(c);
            break;

        case EXIT:clientExit(c);
            break;

        case INVALID:
            print_invalid_input();
            break;
    }


};

//// send/recv

void Server::prepSize(uint64_t size, Client *client){
    uint64_t datalen = size;
    ssize_t written = write(client->sockfd, &datalen, sizeof(uint64_t));
    if(written != c.sender.size()){
        print_error("write", errno);
    }
}

void Server::MessageToClient(std::string message, const std::string &clientName){
    Client *client = clients[clientName];

    // send sender name size
    prepSize(c.sender.size(), client);

    //send sender name
    ssize_t written = write(client->sockfd, &c.sender, c.sender.size());
    if(written != c.sender.size()){
        print_error("write", errno);
    }

    // send message size
    prepSize(message.size(), client);

    // send message
    written = write(client->sockfd, &message, message.size());
    if(written != message.size()){
        print_error("write", errno);
    }

}

void Server::successToClient(bool success, const std::string &clientName){
    Client *client = clients[clientName];

    // send bool
    ssize_t written = write(client->sockfd, &success, sizeof(success));
    if(written != sizeof(success)){
        print_error("write", errno);
    }
}

void Server::whoToClient(std::vector<std::string> sortedVec, const std::string &clientName){
    Client *client = clients[clientName];

    //send vec size
    prepSize(sortedVec.size(), client);

    //send vec
    ssize_t written = write(client->sockfd, &sortedVec, sortedVec.size());
    if(written != sortedVec.size()){
        print_error("write", errno);
    }

}



//// DB modify

void Server::registerClient(std::string &name) {
    //todo

    if (!isLegalGroupOrClientName(name)){
        //todo err how does this err look
    }
    int sockfd = nullptr; //todo get the sockfd

    Client newClient{name, sockfd}; //todo is this valid creation (scope?)

    clients.insert(ClientPair(name, &newClient));  //todo is this valid creation (scope?)

    // print success on server
    printf("%s: Connected Successfully.\n", name);

    //todo should client print automatically, or should report sucess?

}

//// DB queries

bool Server::isClient(std::string &name) {
    return((bool)clients.count(name)); // (count is zero (false) if not there.
}

bool Server::isGroup(std::string &name){
    return((bool)groups.count(name)); // (count is zero (false) if not there.
}


//// request handling

void Server::createGroup(Command c) {

    // ensure group name legal & unique (not taken)
    if(!isLegalGroupOrClientName(c.name)){
        //print failure on server
        print_create_group(true, false, c.sender, c.name);
        //report failure to client
        successToClient(false, c.sender);
    }

    // make set of clients (allowing duplicates) and ensuring all clients exists
    Group newGroup;

    newGroup.name = c.name;


    // add each client
    for(std::string strName : c.clients){
        // if a client not in group
        if(newGroup.groupMembers.count(strName)){


            // ensure client exists in server
            if(!isClient(strName)){
                //print failure on server
                print_create_group(true, false, c.sender, c.name);
                //report failure to client
                successToClient(false, c.sender);
            }

            //add it to group
            newGroup.groupMembers.insert(ClientPair(strName, clients.at(strName)));
        }
    }

    // add sender to set even if unspecified
    newGroup.groupMembers.insert(ClientPair(c.sender, clients.at(c.sender)));

    // ensure group has at least 2 members (including creating client)
    if(newGroup.groupMembers.size() < 2){

        //print failure on server
        print_create_group(true, false, c.sender, c.name);
        //report failure to client
        successToClient(false, c.sender);
    }

    // add this group to DB
    groups.emplace(GroupPair(newGroup.name, &newGroup));


    //print success on server
    print_create_group(true, true, c.sender, c.name);
    //report success to client
    successToClient(true, c.sender);

}

void Server::send(Command c) {

    //// if name in client
    if (isClient(c.name)) {

        //// ensure recipient is not sender
        if(c.name == c.sender){
            // notify sender of failure
            successToClient(false, c.sender);
        }

        //// send to client

        std::string message = c.sender + ": " + c.message;

        // message the recipient client
        MessageToClient(message, c.name);

        // notify sender of success
        successToClient(true,c.sender);

    }
        //// if name in groups
    else if(isGroup(c.name)){

        //// ensure caller is in this group
        if(!groups.count(c.name)){
            // notify sender of failure
            successToClient(false, c.sender);
        }

        //// send to all in group except caller
        for(ClientPair & pair : groups.at(c.name)->groupMembers){
            // if not sender
            if(pair.first != c.sender){
                std::string message = c.sender + ": " + c.message;
                MessageToClient(message, pair.first);
            }
        }

        // notify sender of success
        successToClient(true,c.sender);

    }else{
        //// else error
        // notify sender of failure
        successToClient(false, c.sender);
    }

}

void Server::who(Command c) {
    //// order and return names
    std::vector<std::string> namesVec;

    // get all names
    for(ClientPair & pair : clients){
        namesVec.push_back(pair.first);
    }

    std::sort(namesVec.front(), namesVec.back());

    //send list to printing
    whoToClient(namesVec, c.sender);


}

void Server::clientExit(Command c){

    // remove sender from all groups
    for(GroupPair & pair : groups){

        // remove sender from members of group (if he is there)
        Group *group = pair.second;
        group->groupMembers.erase(c.sender);
    }

    //print success to server
    print_exit(true,c.sender);
    // send success to client
    successToClient(true, c.sender);

    // remove sender from server (after reported success)
    clients.erase(c.sender);
}

//// name legality

bool Server::isLegalGroupOrClientName(std::string &name){
    // ensure alphanumeric only and name not taken.
    return(isAlNumString(name) && !isClient(name) && !isGroup(name));
}


bool Server::isAlNumString(std::string &str){
    for(char c: str){
        if(!isalnum(c)) return false;
    }
    return true;
}


//// ===============================  Helper Functions ============================================

//// input checking
int parsePortNum(int argc, char **argv){

    //// check args
    if (argc != 2) {
        print_server_usage();  //todo server shouldnt crash upon receiving illegal requests?
        exit(1);
    }

    int portNumber = atoi(argv[1]);

    if (portNumber < 0 || portNumber > 65535) {
        print_server_usage();
        exit(1);
    }

    return portNumber;
}


//// errors

/**
 * Checks for failure of library functions, and handling them when they occur.
 * @param retVal
 * @param funcName Name of function
 * @param successVal value given for success (default is 0)
 */
void errCheck(int &retVal, const std::string &funcName, int successVal = 0) {

    // if no failure, return
    if (retVal == successVal) return;

    // set prefix
    print_error(funcName, errno); // todo J is this what is meant by error number?

    // exit
    exit(FUCK);
}
//todo N: maybe will chaging the errcheck to just print the error.
//todo N: errors can be -1 / 0 / nullprt

//// =============================== Main Function ================================================

int main(int argc, char *argv[]) {

    //// --- Init  ---
    int portNumber = parsePortNum(argc, argv);

    //// init Server
    Server server((unsigned short)portNumber);  // todo J is conversion ok? maybe cast inside parse

    //// --- Setup  ---
    //// create socket
    //// bind to an ip adress and port

    //// --- Wait  ---
    //// listen
    //// accept
    server.selectPhase();

    //// --- Get Request  ---
    //// read
    //// parse

    //// --- Process  ---
    //// write
    //// close

    //// --- Repeat  ---
    //todo disconnect clients, clear memory, and exit(0)
}