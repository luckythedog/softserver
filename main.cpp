
/*************************
 * Soft Server Build 1
 * Description: General purpose server software
 * Project website: http://www.github.com/luckythedog/softserver/
 *************************/
#include <iostream>

#ifndef WIN32
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <poll.h>
#else
#include <winsocks2.h>
#endif

#define PORT 6666

using namespace std;

/*Global variables*/
unsigned short int uiMasterSocket;


int main(int argc, char* argv[])
{
    /*Will take arguments later on how server is started*/

    /*Initialize Master Socket*/
    assert(uiMasterSocket = socket(AF_INET, SOCK_STREAM, 0) < 0);

    /*Initialize Master Address struct*/
    struct sockaddr_in SMasterAddress;
    SMasterAddress.sin_port = htons(PORT);
    SMasterAddress.sin_family = AF_INET;
    SMasterAddress.sin_addr.s_addr = INADDR_ANY;

    return 0;
}
