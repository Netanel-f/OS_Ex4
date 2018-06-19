#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include "whatsappio.h"



//// ============================   Constants =====================================================


//// ===========================   Typedefs & Structs =============================================

// client
struct Client   //todo remove
{
    std::string name;
    int sockfd;
};

// command
struct Command
{
    std::string command;
    command_type type;
    std::string name;
    std::string message;
    std::vector<std::string> clients;
};

//// ============================  Forward Declarations ===========================================
//bool validateClientRequest(std::string * userInput);
//bool validateGroupCreation(Command * command, std::string * senderName);
//bool validateSend(Command * command, std::string * senderName);
bool isNameValid(std::string * name);
//void requestCreateGroup(Command * command, std::string * senderName);
//void requestSend(Command * command, std::string * senderName);
//void requestWho(Command * command);
void requestExit();
void validateMainArgc(int argc, char **argv);
void writeToSocket(int sockfd, const std::string& command);
std::string parseClientName(char * name);
unsigned short parseClientPort(char * port);


// Client class

class ClientObj {
    std::string clientName;
    int sockfd;
    int maxSockfd;

public:

    //// C-tor
    explicit void ClientObj(const std::string &clientName, unsigned short port, char * server);

    //// client actions
    void selectPhase();

private:
    void handleClientRequest(std::string * userInput);
    bool validateGroupCreation(Command * command, std::string * validateCmd);
    bool validateSend(Command * command);
//    void requestCreateGroup(Command * command);
//    void requestSend(Command * command);
//    void requestWho(Command * command);
//    void requestExit();
};

ClientObj::ClientObj(const std::string &clientName, unsigned short port, char * server) {
    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent * hostEnt;
    bzero(&sa, sizeof(struct sa));

    hostEnt = gethostbyname(server);
    if (hostEnt == nullptr) {
        print_error("gethostbyname", errno);
        exit(1);
    }

    memset(&sa, 0,sizeof(sa));
    memcpy((char *)&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_family = hostEnt->h_addrtype;
    sa.sin_port = htons(port);

    sockfd = socket(hostEnt->h_addrtype, SOCK_STREAM, 0);
    if (sockfd < 0) {
        print_error("socket", errno);
        exit(1);
    }

    int retVal = connect(sockfd, (struct sockaddr*)&sa, sizeof(sa));
    if (retVal < 0) {
        print_error("connect", errno);
        //todo close socket
    }
    if (sockfd > STDIN_FILENO) {
        maxSockfd = sockfd;
    } else {
        maxSockfd = STDIN_FILENO;
    }
    //todo send name to actually register.
    print_connection(); //todo check first server reply
}

void ClientObj::handleClientRequest(std::string * userInput) {
    Command command;
    parse_command(*userInput, command.type, command.name, command.message, command.clients);
    std::string validateCmd;

    switch (command.type) {
        case CREATE_GROUP:
            if (this->validateGroupCreation(&command, &validateCmd)) {
                //todo write validatedCmd
            } else { print_invalid_input(); }
            break;

        case SEND:
            if (this->validateSend(&command)) {
                //todo write command.command
            } else { print_invalid_input(); }
            break;

        case WHO:
            //todo write
            break;

        case EXIT:
            //todo write
            //exit

        case INVALID:
            print_invalid_input();
            break;
    }
}

bool ClientObj::validateGroupCreation(Command * command, std::string *validateCmd) {
    // check if name of group is valid,
    (*validateCmd) += "create_group ";
    if(isNameValid(&(command->name))) {
        (*validateCmd) += command->name;
        (*validateCmd) += " ";
        if (!command->clients.empty()) {
            std::sort(command->clients.begin(), command->clients.end());
            auto last = std::unique(command->clients.begin(), command->clients.end());
            command->clients.erase(last, command->clients.end());

            if (command->clients.empty()) {
                return false;
            }

            bool foundOthersUsers = false;
            for (auto &clientName : command->clients) {
                if (!isNameValid(&clientName)) {
                    return false;
                }
                if (!(clientName == this->clientName)) {
                    foundOthersUsers = true;
                    (*validateCmd) += clientName;
                    (*validateCmd) += " ";
                }
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
    return (command->name != this->clientName) && isNameValid(&(command->name));
    // if arrived here - one of the names is invalid
}

//void ClientObj::requestCreateGroup(Command * command){
//    if (validateGroupCreation(command)) {
//        //todo send request to server
//        return;
//    } else {
//        print_invalid_input();
//    }
//}
//
//void ClientObj::requestSend(Command * command) {
//    if (validateSend(command)){
//        //todo send request to server
//        return;
//    } else {
//        print_invalid_input();
//    }
//}
//
//void ClientObj::requestWho(Command * command){
//    //todo send who request
//}
//void ClientObj::requestExit(){
//    //todo send exit request to server and clear client memort.
//
//}

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
        print_client_usage();
        exit(1);
    }
    return clientName;
}

unsigned short parseClientPort(char * port) {
    int portNumber = atoi(port);

    //todo validate args!
    if (portNumber < 0 || portNumber > 65535) {
        print_client_usage();
        exit(1);
    }
    return (unsigned short)portNumber;
}

void ClientObj::selectPhase() {
    fd_set clientsfds;
    fd_set readfds; //Represent a set of file descriptors.
    FD_ZERO(&clientsfds);   //Initializes the file descriptor set fdset to have zero bits for all file descriptors

    FD_SET(this->sockfd,
           &clientsfds);  //Sets the bit for the file descriptor fd in the file descriptor set fdset.

    FD_SET(STDIN_FILENO, &clientsfds);

    int retVal;
    while (true) {
        readfds = clientsfds;
        retVal = select(maxSockfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (retVal == -1) {
            print_error("select", errno);
            exit(1);
        } else if (retVal == 0) {
            continue;
        }
        //Returns a non-zero value if the bit for the file descriptor fd is set in the file descriptor set pointed to by fdset, and 0 otherwise

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            //msg from stdin
            std::string userInput;
            getline(std::cin, userInput);//todo 22:29
            handleClientRequest(&userInput);
        }
    }
}

int main(int argc, char *argv[])
{
    //// --- Setup  ---
    validateMainArgc(argc);
    std::string clientName = parseClientName(argv[1]);
    unsigned short clientPort = parseClientPort(argv[3]);

    // Constructing client - create socket, connect socket.
    ClientObj client = ClientObj(clientName, clientPort, argv[2]);

    //// wait streams
    client.selectPhase();

}

bool isNameValid(std::string * name) {
    return !(name->length() > WA_MAX_NAME ||
            std::any_of(name->begin(), name->end(), !std::isalnum));
}

//// ==============================  Helper Functions =============================================


