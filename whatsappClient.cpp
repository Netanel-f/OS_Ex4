
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include "whatsappio.h"



//// ============================   Constants =====================================================


//// ===========================   Typedefs & Structs =============================================

// client
struct Client
{
    std::string name;
    int sockfd;
};

// command
struct Command
{
    command_type type;
    std::string name;
    std::string message;
    std::vector<std::string> clients;
};

//// ============================  Forward Declarations ===========================================
bool validateGroupCreation(Command * command, std::string * senderName);
bool validateSend(Command * command, std::string * senderName);
bool isNameValid(std::string * name);
void requestCreateGroup(Command * command, std::string * senderName);
void requestSend(Command * command, std::string * senderName);
void requestWho(Command * command);
void requestExit();
void validateMainArgc(int argc, char **argv);
std::string parseClientName(char * name);
unsigned short parseClientPort(char * port);


// Client class

class ClientObj {
    std::string clientName;
    int sockfd;

public:

    //// C-tor
    explicit void ClientObj(const std::string &clientName, unsigned short port, char * server);

    //// client actions
private:
    bool validateGroupCreation(Command * command);
    bool validateSend(Command * command);
    void requestCreateGroup(Command * command);
    void requestSend(Command * command);
    void requestWho(Command * command);
    void requestExit();
};

ClientObj::ClientObj(const std::string &clientName, unsigned short port, char * server) {
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent * hostEnt;
    bzero(&sa, sizeof(struct sa));

    hostEnt = gethostbyname(server);
    if (hostEnt == nullptr) {
        print_error("gethostbyname", errno);
        //todo should we exit?
    }

    memset(&sa, 0,sizeof(sa));
    memcpy((char *)&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_family = hostEnt->h_addrtype;
    sa.sin_port = htons(port);

    sockfd = socket(hostEnt->h_addrtype, SOCK_STREAM, 0);
    if (sockfd < 0) {
        print_error("socket", errno);
        //todo should we exit?
    }

    int retVal = connect(sockfd, (struct sockaddr*)&sa, sizeof(sa));
    if (retVal < 0) {
        print_error("connect", errno);
        //todo close socket
    }
    //todo send name to actually register.
    print_connection(); //todo check first server reply
}

bool ClientObj::validateGroupCreation(Command * command) {
    // check if name of group is valid,
    if(isNameValid(&(command->name))) {
        if (!command->clients.empty()) {
            bool foundOthersUsers = false;
            for (auto &clientName : command->clients) {
                if (!isNameValid(&clientName)) { break; }
                if (!(clientName == this->clientName)) { foundOthersUsers = true; }
            }

            if (foundOthersUsers) {
                return true;
            }
        }
    }

    // if arrived here - one of the names is invalid
    return false;
}

bool ClientObj::validateSend(Command * command) {
    if (isNameValid(&(command->name))) {
        if (!(command->name == this->clientName)) {
            //todo send request to server
            return true;
        }
    }
    // if arrived here - one of the names is invalid
    return false;
}

void ClientObj::requestCreateGroup(Command * command){
    if (validateGroupCreation(command)) {
        //todo send request to server
        return;
    } else {
        print_invalid_input();
    }
}

void ClientObj::requestSend(Command * command) {
    if (validateSend(command)){
        //todo send request to server
        return;
    } else {
        print_invalid_input();
    }
}

void ClientObj::requestWho(Command * command){
    //todo send who request
}
void ClientObj::requestExit(){
    //todo send exit request to server and clear client memort.

}

//// ===========================   Global Variables ===============================================

//// ============================== Main Function ================================================

void validateMainArgc(int argc) {
    if (argc != 4) {
        print_server_usage();
        exit(1);
    }
}
std::string parseClientName(char * name) {

    std::string clientName = std::string(name);

    if (clientName.length() > WA_MAX_NAME ||
        std::any_of(clientName.begin(), clientName.end(), !std::isalnum)) {
        print_server_usage();
        exit(1);
    }
    return clientName;
}

unsigned short parseClientPort(char * port) {
    int portNumber = atoi(port);

    //todo validate args!
    if (portNumber < 0 || portNumber > 65535) {
        print_server_usage();
        exit(1);
    }
    return (unsigned short)portNumber;
}

int main(int argc, char *argv[])
{
    //// --- Setup  ---
    validateMainArgc(argc);
    std::string clientName = parseClientName(argv[1]);
    unsigned short clientPort = parseClientPort(argv[3]);
    ClientObj client = ClientObj(clientName, clientPort, argv[2]);

    //// connect to specified port
    //// (Bind to server?)
    //// wait for Server

    //// --- Setup  ---

}

bool isNameValid(std::string * name) {
    return !(name->length() > WA_MAX_NAME ||
            std::any_of(name->begin(), name->end(), !std::isalnum));
}

//// ==============================  Helper Functions =============================================


