
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <map>
#include <iostream>
#include <errno.h>
#include "whatsappio.h"

// Todo: check sucess, listed for EXIT,

//// ============================   Constants =====================================================
static const int MAX_QUEUE = 10;
static const int MAX_HOST_NAME_LEN = 30;
static const int MAX_GROUP_NAME_LEN = 30;
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
  std::string caller;
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
    bool isLegalClientName(std::string &name);
    bool isLegalGroupName(std::string &name);
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
    //todo
}

void Server::serverStdInput(){
    //todo
}

void Server::handleClientRequest(){
    commandStr = NULL; //todo read
    // todo clear old command?
    parse_command(commandStr, c.type, c.name, c.message, c.clients);
    c.caller = "get caller"; //todo
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
            //todo
            break;
    }


};

//// DB modify

void Server::registerClient(std::string &name) {
    //todo

    if (!isLegalClientName(name)){
        //todo err
    }
    int sockfd = nullptr; //todo

    Client newClient{name, sockfd}; //todo is this valid creation (scope?)

    clients.insert(ClientPair(name, &newClient));  //todo is this valid creation (scope?)
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

    //// ensure group name legal & unique (not taken)
    if(!isLegalGroupName(c.name)){
        //todo err
    }

    //// make set of clients (allowing duplicates) and ensuring all clients exists
    Group newGroup;

    newGroup.name = c.name;


    // add each client
    for(std::string strName : c.clients){
        // if a client not in group
        if(newGroup.groupMembers.count(strName)){


            //ensure client exists in server
            if(!isClient(strName)){
                //todo err
            }

            //add it to group
            newGroup.groupMembers.insert(ClientPair(strName, clients.at(strName)));
        }
    }

    //// add caller to set even if unspecified
    newGroup.groupMembers.insert(ClientPair(c.caller, clients.at(c.caller)));

    //// ensure group has at least 2 members (including creating client)
    if(newGroup.groupMembers.size() < 2){
        //todo err  J (we dont print anything. client should also check this)
        // todo J  if client checks we might not even need this

    }

    //// add this group to DB
    groups.emplace(GroupPair(newGroup.name, &newGroup));


    /// and trigger output, both server and client
    print_create_group(0, 0, NULL, NULL); //todo client

}

void Server::send(Command c) {

    //// if name in client
    if (isClient(c.name)) {

        //// ensure recipient is not sender
        if(c.name == c.caller){
            //todo err
        }

        //// send to client
        print_send(); // todo
    }
        //// if name in groups
    else if(isGroup(c.name)){

        //// ensure caller is in this group
        if(!groups.count(c.name)){
            //todo err
        }

        //// send to all in group except caller
        for(ClientPair & pair : groups.at(c.name)->groupMembers){
            // if not caller
            if(pair.first != c.caller){
                //todo print send
            }
        }
    }

    //// else error
    //todo err

}

void Server::who(Command c) {
    //// order and return names
    std::vector<std::string> namesVec;

    // get all names
    for(ClientPair & pair : clients){
        namesVec.push_back(pair.first);
    }

    // todo send list to printing
    print_who_client(); //todo print


}

void Server::clientExit(Command c){
    //todo

    // remove caller from all groups
    for(GroupPair & pair : groups){

        // remove caller from members of group (if he is there)
        Group *group = pair.second;
        group->groupMembers.erase(c.caller);
    }

    // remove caller from server
    clients.erase(c.caller);

    // todo print success to server and client
    print_exit();
}

//// name legality

bool Server::isLegalClientName(std::string &name){
    // ensure alphanumeric only and name not taken.
    return(isAlNumString(name) && !isClient(name));
}

bool Server::isLegalGroupName(std::string &name){
    // ensure alphanumeric only and name not taken.
    return(isAlNumString(name) && !isGroup(name));
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
    print_error(funcName, retVal); // todo J is this what is meant by error number?

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