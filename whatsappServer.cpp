
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <map>
#include <iostream>
#include "whatsappio.h"

// Todo: check sucess, listed for EXIT,

//// ============================   Constants =====================================================
static const int MAX_QUEUE = 10;
static const int MAX_HOST_NAME_LEN = 30;
static const int MAX_GROUP_NAME_LEN = 30;


//// ===========================   Typedefs & Structs =============================================

typedef std::pair<std::string, Client *> ClientPair;
typedef std::pair<std::string, Group *> GroupPair;

// client
struct Client {
  std::string name;
  int sockfd;
};

// command
struct Command {
  command_type type;
  std::string name;
  std::string message;
  std::vector<std::string> clients;
  std::string caller;
};

// clients Group
struct Group {
  std::string name;
  std::map<std::string, Client *> groupMembers;
};

struct serverDB {
  std::string serverName;
  unsigned short serverPort;
  int welcomeSocket;

  std::map<std::string, Client *> clients;
  std::map<std::string, Group *> groups;

};


//// ===========================   Global Variables ===============================================
//todo J there should probably be none for thread safety

//// ============================  Forward Declarations ===========================================

//// server actions
void setupServer(serverDB *db, unsigned short portNumber);
void selectPhase(serverDB *serverData);
void connectNewClient(serverDB *db);
void serverStdInput();
void handleClientRequest(serverDB *serverData);

//// DB modify
void registerClient(std::string &name, serverDB *db);

//// DB queries
bool isClient(std::string &name, serverDB *db);
bool isGroup(std::string &name, serverDB *db);

//// request handling
void createGroup(Command c, serverDB *db);
void send(Command c, serverDB *db);
void who(Command c, serverDB *db);
void clientExit(Command c, serverDB *db);

//// name legality
bool isLegalClientName(std::string &name, serverDB *db);
bool isLegalGroupName(std::string &name, serverDB *db);
bool isAlNumString(std::string &str);

//// errors
void errCheck(int &returnVal, const std::string &funcName, int checkVal = 0);
//todo N: maybe will chaging the errcheck to just print the error.
//todo N: errors can be -1 / 0 / nullprt

//// =============================== Main Function ================================================

int main(int argc, char *argv[]) {
    //// --- Init  ---
    //// check args
    if (argc != 2) {
        printf("Usage: whatsappServer portNum\n");  //todo server shouldnt crash upon receiving illegal requests?
        exit(1);
    }

    int portNumber = atoi(argv[1]);

    if (portNumber < 0 || portNumber > 65535) {
        printf("Usage: whatsappServer portNum\n");
        exit(1);
    }

    //// init DAST's
    serverDB serverData;

    //// --- Setup  ---
    //// create socket
    //// bind to an ip adress and port

    setupServer(&serverData, portNumber);

    //// --- Wait  ---
    //// listen
    //// accept
    selectPhase(&serverData);

    //// --- Get Request  ---
    //// read
    //// parse

    //// --- Process  ---
    //// write
    //// close

    //// --- Repeat  ---
    //todo disconnect clients clear memory and exit(0)
}
//// ===============================  Helper Functions ============================================
// todo NEWWWWWWWWWWWWWWWWWWWW

//// server actions

void setupServer(serverDB *db, unsigned short portNumber) {

    char serverName[MAX_HOST_NAME_LEN + 1];
    int welcomeSocket;
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent *hostEnt;

//    int retVal = gethostname(serverName, MAX_HOST_NAME_LEN);
//    errCheck(retVal, "gethostname");
    bzero(&sa,sizeof(struct sockaddr_in));
    hostEnt = gethostbyname(serverName);
    if (hostEnt == nullptr) {
//        errCheck("gethostbyname");    //todo need to handle error
    }

    memset(&sa, 0,sizeof(sa));
    sa.sin_family = AF_INET;
    memcpy(&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_port = htons(portNumber);

    welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
    errCheck(welcomeSocket, "socket");

    int retVal = bind(welcomeSocket, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
    errCheck(retVal, "bind");

    listen(welcomeSocket, MAX_QUEUE);

    *db = {serverName, portNumber, welcomeSocket};

}

void selectPhase(serverDB * serverData) {
    fd_set clientsfds;
    fd_set readfds; //Represent a set of file descriptors.
    FD_ZERO(&clientsfds);   //Initializes the file descriptor set fdset to have zero bits for all file descriptors

    FD_SET(serverData->welcomeSocket, &clientsfds);  //Sets the bit for the file descriptor fd in the file descriptor set fdset.


    FD_SET(STDIN_FILENO, &clientsfds);
    int retVal;
    while (true) {
        readfds = clientsfds;
        retVal = select(MAX_QUEUE+1, &readfds, nullptr, nullptr, nullptr);
        errCheck(retVal, "select");
        //todo terminate server and return -1;

        //Returns a non-zero value if the bit for the file descriptor fd is set in the file descriptor set pointed to by fdset, and 0 otherwise
        if (FD_ISSET(serverData->welcomeSocket, &readfds)) {
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

void connectNewClient(serverDB *db){
    //todo
}

void serverStdInput(){
    //todo
}

void handleClientRequest(serverDB *serverData) {
    const std::string command = NULL; //todo read
    Command c;
    parse_command(command, c.type, c.name, c.message, c.clients);
    c.caller = "get caller"; //todo
    switch (c.type) { //todo
        case CREATE_GROUP:createGroup(c, serverData);
            break;

        case SEND:send(c, serverData);
            break;

        case WHO:who(c, serverData);
            break;

        case EXIT:clientExit(c, serverData);
            break;

        case INVALID:
            //todo
            break;
    }
};

//// DB modify

void registerClient(std::string &name, serverDB *db) {
    //todo

    if (!isLegalClientName(name, db)){
        //todo err
    }
    int sockfd = nullptr; //todo

    Client newClient{name, sockfd}; //todo is this valid creation (scope?)

    db->clients.insert(ClientPair(name, &newClient));  //todo is this valid creation (scope?)
}

//// DB queries

bool isClient(std::string &name, serverDB *db) {
    return((bool)db->clients.count(name)); // (count is zero (false) if not there.
}

bool isGroup(std::string &name, serverDB *db){
    return((bool)db->groups.count(name)); // (count is zero (false) if not there.
}


//// request handling

void createGroup(Command c, serverDB * db) {

    //// ensure group name legal & unique (not taken)
    if(!isLegalGroupName(c.name, db)){
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
            if(!isClient(strName, db)){
                //todo err
            }

            //add it to group
            newGroup.groupMembers.insert(ClientPair(strName, db->clients.at(strName)));
        }
    }

    //// add caller to set even if unspecified
    newGroup.groupMembers.insert(ClientPair(c.caller, db->clients.at(c.caller)));

    //// ensure group has at least 2 members (including creating client)
    if(newGroup.groupMembers.size() < 2){
        //todo err  J (we dont print anything. client should also check this)
        // todo J  if client checks we might not even need this

    }

    //// add this group to DB
    db->groups.emplace(GroupPair(newGroup.name, &newGroup));


    /// and trigger output, both server and client
    print_create_group(0, 0, NULL, NULL); //todo client

}

void send(Command c, serverDB *db) {

    //// if name in client
    if (isClient(c.name, db)) {

        //// ensure recipient is not sender
        if(c.name == c.caller){
            //todo err
        }

        //// send to client
        print_send(); // todo
    }
        //// if name in groups
    else if(isGroup(c.name, db)){

        //// ensure caller is in this group
        if(!db->groups->count(c.name)){
            //todo err
        }

        //// send to all in group except caller
        for(ClientPair & pair : db->groups.at(c.name)->groupMembers){
            // if not caller
            if(pair.first != c.caller){
                //todo print send
            }
        }
    }

    //// else error
    //todo err

}

void who(Command c,  serverDB *db) {
    //// order and return names
    std::vector<std::string> namesVec;

    // get all names
    for(ClientPair & pair : db->clients){
        namesVec.push_back(pair.first);
    }

    // todo send list to printing
    print_who_client(); //todo print

}

void clientExit(Command c, serverDB *db){
    //todo

    // remove caller from all groups
    for(GroupPair & pair : db->groups){

        // remove caller from members of group (if he is there)
        Group *group = pair.second;
        group->groupMembers.erase(c.caller);
    }

    // remove caller from server
    db->clients.erase(c.caller);

    // todo print sucess to server and client
    print_exit();
}

//// name legality

bool isLegalClientName(std::string &name, serverDB *db){
    // ensure alphanumeric only and name not taken.
    return(isAlNumString(name) && !isClient(name, db));
}

bool isLegalGroupName(std::string &name, serverDB *db){
    // ensure alphanumeric only and name not taken.
    return(isAlNumString(name) && !isGroup(name, db));
}

bool isAlNumString(std::string &str){
    for(char c: str){
        if(!isalnum(c)) return false;
    }
    return true;
}

//// errors

/**
 * Checks for failure of library functions, and handling them when they occur.
 */
void errCheck(int &returnVal, const std::string &funcName, int checkVal = 0) {

    // if no failure, return
    if (returnVal == checkVal) return;

    // set prefix
    print_error(funcName, returnVal); // todo J is this what is meant by error number?

    // exit
    exit(-1);
}
//todo N: maybe will chaging the errcheck to just print the error.
//todo N: errors can be -1 / 0 / nullprt

