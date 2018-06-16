
#include "whatsappio.h"



//// ============================   Constants =====================================================


//// ===========================   Typedefs & Structs =============================================

// client
struct Client
{
    std::string name;
    int sockfd;
};

//// ===========================   Global Variables ===============================================


//// ============================  Forward Declarations ===========================================


//// ============================== Main Function ================================================

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: whatsappServer clientName serverAddress serverPort\n");  //todo server shouldnt crash upon receiving illegal requests?
        exit(1);
    }

    char * clientName = argv[1];
    char * serverAddress = argv[2];
    int portNumber = atoi(argv[3]);

    //todo validate args!
    if (portNumber < 0 || portNumber> 65535){
        printf("Usage: whatsappServer clientName serverAddress serverPort\n");
        exit(1);
    }

    //// --- Setup  ---

    //// connect to specified port
    //// (Bind to server?)
    //// wait for Server

    //// --- Setup  ---
    //// --- Setup  ---
    //// --- Setup  ---
    //// --- Setup  ---
}


//// ==============================  Helper Functions =============================================


