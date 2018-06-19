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
bool isNameValid(std::string * name);
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
    void connectToServer();
    void selectPhase();

private:
    void handleServerReply();
    void handleClientRequest(std::string * userInput);
    bool validateGroupCreation(Command * command, std::string * validateCmd);
    void writeToServer(const std::string& command);
    int readFromServer(char * buf, int n);

};


/**
 * Constructing client object - including creating and connecting the socket for server.
 * @param clientName validated client name
 * @param port validated port number
 * @param server ip address
 */
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
        close(sockfd);
        exit(1);
    }
    if (sockfd > STDIN_FILENO) {
        maxSockfd = sockfd;
    } else {
        maxSockfd = STDIN_FILENO;
    }
}

void ClientObj::connectToServer() {
    std::string cmd = "connect ";   //todo fix parse_command to support CONNECT
    cmd += this->clientName;
    writeToServer(cmd);
}


void ClientObj::handleServerReply() {
//    uint64_t msgSize;
    char buffer[4]; //todo supporting 4digits length of msg
    bzero(buffer, 4);
    readFromServer(buffer, 4);
//
    int msgSize = atoi(buffer);
    char incomingMsg[msgSize];
    readFromServer(incomingMsg, msgSize);

    Command sReply;
    parse_response(incomingMsg, sReply.type, sReply.name, sReply.message, sReply.clients);

    bool replyResult = (strcmp(sReply.message) == "T");

    // send T/F
    // create_group T/F <group_name>
    // Who <ret_client_name_separated_by_commas_without_spaces>
    // exit T/F
    // connect T/F/D

    switch (sReply.type) {
        case CREATE_GROUP:
            print_create_group(false, replyResult, nullptr, sReply.name);
            break;

        case SEND:
            print_send(false, replyResult, nullptr, nullptr, nullptr);
            break;

        case WHO:
            print_who_client(replyResult, sReply.clients);
            break;

        case EXIT:
            //todo handle
            break;

        case CONNECT:
            if (replyResult) {
                print_connection();
            } else if (strcmp(sReply.message) == "D"){
                print_dup_connection();
            } else {
                print_fail_connection();
            }
            break;

        case INVALID:
            print_invalid_input();
            break;

    }
}



void ClientObj::handleClientRequest(std::string * userInput) {
    Command command;
    parse_command(*userInput, command.type, command.name, command.message, command.clients);
    std::string validateCmd;

    switch (command.type) {
        case CREATE_GROUP:
            if (this->validateGroupCreation(&command, &validateCmd)) {
                writeToServer(validateCmd);
            }
            break;

        case SEND:
            if ((command.name != this->clientName) && isNameValid(&(command.name))) {
                writeToServer(command.command);
            }
            break;

        case WHO:
            writeToServer(command.command);
            break;

        case EXIT:
            writeToServer(command.command);
            break;

        case INVALID:
        case CONNECT:

    print_invalid_input();

    }
}

bool ClientObj::validateGroupCreation(Command * command, std::string *validateCmd) {
    // check if name of group is valid,
    (*validateCmd) += "create_group ";
    if(isNameValid(&(command->name))) {
        (*validateCmd) += command->name;
        (*validateCmd) += " ";
        if (!command->clients.empty()) {

            // deleting duplicated clients name.
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

            return foundOthersUsers;
        }
    }   // if arrived here - one of the names is invalid
    return false;
}

void ClientObj::writeToServer(const std::string& command) {
    size_t cmdlen = command.length();
    ssize_t wrote = write(this->sockfd, &command, cmdlen);
    if (wrote < 0) {
        print_error("write", errno);
    }
}

int ClientObj::readFromServer(char * buf, int n) {
    int bcount = 0; /* counts bytes read */
    int br = 0; /* bytes read this pass */

    while (bcount < n) { /* loop until full buffer */
        br = read(this->sockfd, buf, n-bcount);
        if (br > 0) {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            print_error("read", errno);
        }
    }
    return(bcount);
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

    FD_SET(this->sockfd, &clientsfds);  //Sets the bit for the file descriptor fd in the file descriptor set fdset.
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
            getline(std::cin, userInput);
            handleClientRequest(&userInput);
        }

        if (FD_ISSET(this->sockfd, &readfds)) {
            handleServerReply();
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
    client.connectToServer();


    //// wait streams
    client.selectPhase();

}

bool isNameValid(std::string * name) {
    return !(name->length() > WA_MAX_NAME ||
            std::any_of(name->begin(), name->end(), !std::isalnum));
}

//// ==============================  Helper Functions =============================================


