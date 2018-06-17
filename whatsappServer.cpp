
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

// client
struct Client
{
    std::string name;
    int sockfd;
};

// client
struct Command
{
  command_type type;
  std::string name;
  std::string message;
  std::vector<std::string> clients;
};



// clients Group
struct clientsGroup
{
    std::string name;
    std::vector<Client *> * groupMembers;
};

struct serverDB
{
    std::string serverName;
    unsigned short serverPort;
    int welcomeSocket;

};
struct Context
{
    std::string name;
    int sockfd;
};


//// ===========================   Global Variables ===============================================
//todo J there should probably be none for thread safety

//// ============================  Forward Declarations ===========================================
bool isClient(std::string& name, serverDB * serverData);
bool isGroup(std::string& name, serverDB * serverData);
void registerClient(std::string& name, serverDB * serverData);
void errCheck(int &returnVal, const std::string &funcName, int checkVal=0);
//todo N: maybe will chaging the errcheck to just print the error.
//todo N: errors can be -1 / 0 / nullprt

void setupServer(serverDB * serverData, unsigned short portNumber);
void selectPhase(serverDB * serverData);
void connectNewClient(serverDB * serverData);
void serverStdInput();
void handleClientRequest();

void createGroup(Command c, serverDB * serverData);
void send(Command c, serverDB * serverData);
void who(Command c, serverDB * serverData);
void serverExit(Command c, serverDB * serverData);


//// =============================== Main Function ================================================

int main(int argc, char *argv[])
{
    //// --- Init  ---
    //// check args
    if (argc != 2) {
        printf("Usage: whatsappServer portNum\n");  //todo server shouldnt crash upon receiving illegal requests?
        exit(1);
    }

    int portNumber = atoi(argv[1]);

    if (portNumber < 0 || portNumber> 65535){
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

void setupServer(serverDB * serverData, unsigned short portNumber) {

    char serverName[MAX_HOST_NAME_LEN + 1];
    int welcomeSocket;
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent * hostEnt;

    int retVal = gethostname(serverName, MAX_HOST_NAME_LEN);
    errCheck(retVal, "gethostname");
    hostEnt = gethostbyname(serverName);
    if (hostEnt== nullptr) {
//        errCheck("gethostbyname");    //todo need to handle error
    }

    memset(&sa, 0,sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    memcpy(&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_port = htons(portNumber);

    welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
    errCheck(welcomeSocket, "socket");

    retVal = bind(welcomeSocket, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
    errCheck(retVal, "bind");

    listen(welcomeSocket, MAX_QUEUE);

    *serverData = {serverName, portNumber, welcomeSocket};

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

void init(){
        //todo
};

void establish(unsigned short portnum){
    //todo
};

void registerClient(std::string& name, serverDB * serverData){
        //todo
};

bool isClient(std::string& name, serverDB * serverData){
    //todo
};

bool isLegalClientName(std::string& name, serverDB * serverData){
    //todo
};

void handleClientRequest(serverDB * serverData){
        const std::string command = NULL; //todo read
        Command c;
        parse_command(command, c.type, c.name, c.message, c.clients);
        switch (c.type)
        { //todo
            case CREATE_GROUP:
                createGroup(c, serverData);
                break;
                
            case SEND:
                send(c, serverData);
                break;
                
            case WHO:
                who(c, serverData);
                break;
                
            case EXIT:
                serverExit(c, serverData);
                break;
                
            case INVALID:
                //todo
                break;
        }
};

void createGroup(Command c){
    //// ensure group name legal
    //// ensure group name unique

    //// make set of clients (allowing duplicates) and ensuring all clients exists
    //// add caller to set even if unspecified

    //// ensure group has at least 2 members (including creating client)

    //// add this group to DB,


    /// and trigger output, both server and client
    print_create_group(0,0,NULL,NULL); //todo

}

void send(Command c,serverDB * serverData){
    //// if name in client
    if(isClient(c.name, )){
        //// ensure recipient exists
        //// ensure recipient is not sender
        //// send to client
        print_send()
    }
    //// if name in groups
    else{
        //// ensure caller is in this group
        //// send to all in group except caller
    }

    //// else error

}

void who(Command c){

}

void serverExit(Command c){

}

//// ===============================  Error Function ==============================================

/**
 * Checks for failure of library functions, and handling them when they occur.
 */
void errCheck(int &returnVal, const std::string &funcName, int checkVal=0) {

    // if no failure, return
    if (returnVal == checkVal) return;

    // set prefix
    print_error(funcName, returnVal); // todo J is this what is meant by error number?

    // exit
    exit(1);
}


