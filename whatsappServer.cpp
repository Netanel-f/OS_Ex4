
#include <sys/socket.h>
#include <list>
#include <map>
#include <iostream>
#include "whatsappio.h"

// Todo: check sucess, listed for EXIT,

//// ============================   Constants =====================================================
static const int MAX_CONCURRENT_CLIENTS = 10;

//// ===========================   Typedefs & Structs =============================================

// client
struct Client
{
    std::string name;
    int sockfd;
};


//// ===========================   Global Variables ===============================================
//todo J there should probably be none for thread safety

//// ============================  Forward Declarations ===========================================
bool isClient(std::string& name, void* clientRegistry);
void registerClient(std::string& name, void* clientRegistry);


//// =============================== Main Function ================================================

int main(int argc, char *argv[])
{
    //// --- Init  ---
        //// check args
        //// init DAST's


    //// --- Setup  ---
        //// create socket
        //// bind to an ip adress and port

    //// --- Wait  ---
        //// listen
        //// accept

    //// --- Get Request  ---
        //// read
        //// parse

    //// --- Process  ---
        //// write
        //// close

    //// --- Repeat  ---
}
//// ===============================  Helper Functions ============================================


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


