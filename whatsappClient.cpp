
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

//// ===========================   Global Variables ===============================================


//// ============================  Forward Declarations ===========================================
void setupClient(struct Client * clientData, char * mainArgs[]);
bool validateGroupCreation(Command * command, std::string * senderName);
bool validateSend(Command * command, std::string * senderName);
bool isNameValid(std::string * name);
void requestCreateGroup(Command * command, std::string * senderName);
void requestSend(Command * command, std::string * senderName);
void requestWho(Command * command);
void requestExit();

//// ============================== Main Function ================================================

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: whatsappServer clientName serverAddress serverPort\n");
        exit(1);
    }

    //// --- Setup  ---
    Client clientData;
    setupClient(&clientData, argv);

    //// connect to specified port
    //// (Bind to server?)
    //// wait for Server

    //// --- Setup  ---

}

//todo should we use bzero?
void setupClient(struct Client * clientData, char * mainArgs[]){
    std::string clientName = std::string(mainArgs[1]);

    if (clientName.length() > WA_MAX_NAME ||
            std::any_of(clientName.begin(), clientName.end(), !std::isalnum)) {
        printf("Usage: whatsappServer clientName serverAddress serverPort\n");
        exit(1);
    }

    int portNumber = atoi(mainArgs[3]);

    //todo validate args!
    if (portNumber < 0 || portNumber> 65535){
        printf("Usage: whatsappServer clientName serverAddress serverPort\n");
        exit(1);
    }


    struct sockaddr_in sa;  // sin_port and sin_addr must be in Network Byte order.
    struct hostent * hostEnt;
    bzero(&sa,sizeof(struct sockaddr_in));

    hostEnt = gethostbyname(mainArgs[2]);
    if (hostEnt== nullptr) {
//        errCheck("gethostbyname");    //todo need to handle error
    }

    memset(&sa, 0,sizeof(sa));
    memcpy(&sa.sin_addr, hostEnt->h_addr, hostEnt->h_length);
    sa.sin_family = hostEnt->h_addrtype;
    sa.sin_port = htons((u_short) portNumber);

    int sockfd;
    sockfd = socket(hostEnt->h_addrtype, SOCK_STREAM, 0);
    errCheck(sockfd, "socket"); //todo printing

    int retVal = connect(sockfd, (struct sockaddr*)&sa, sizeof(sa));
    errCheck(retVal,"connect"); //todo printing
    *clientData = {clientName, sockfd};
    //todo print connect succeeded.
}

bool validateGroupCreation(Command * command, std::string * senderName) {
    // check if name of group is valid,
    if(isNameValid(&(command->name))) {
        if (!command->clients.empty()) {
            bool foundOthersUsers = false;
            for (auto &clientName : command->clients) {
                if (!isNameValid(&clientName)) { break; }
                if (!(clientName == *senderName)) { foundOthersUsers = true}
            }

            if (foundOthersUsers) {
                return true;
            }
        }
    }

    // if arrived here - one of the names is invalid
    return false;
}

bool validateSend(Command * command, std::string * senderName) {
    if (isNameValid(&(command->name))) {
        if (!(command->name == *senderName)) {
            //todo send request to server
            return true;
        }
    }
    // if arrived here - one of the names is invalid
    return false;
}

void requestCreateGroup(Command * command, std::string * senderName) {
    if (validateGroupCreation(command, senderName)) {
        //todo send request to server
        return;
    } else {
        //todo handle error
    }
}

void requestSend(Command * command, std::string * senderName) {
    if (validateSend(command, senderName)){
        //todo send request to server
        return;
    } else {
        //todo handle error
    }
}

void requestWho(Command * command) {
    //todo send who request
}

void requestExit() {
    //todo send exit request to server and clear client memort.
}

bool isNameValid(std::string * name) {
    return !(name->length() > WA_MAX_NAME ||
            std::any_of(name->begin(), name->end(), !std::isalnum));
}

//// ==============================  Helper Functions =============================================


