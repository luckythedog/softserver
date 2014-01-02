/*Standard libraries*/
#include <iostream>
#include <vector>
#include <cstring>

/*Sockets libraries*/
#ifndef _WIN32
/*These libraries will work for both MAC OS X and Linux*/
    #include <pthread.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <semaphore.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <poll.h>
#else
/*Adding compatibility for Windows Sockets*/
    #include <winsocks.h>
#endif

/*SoftServer objects*/
#include "softobject.h"


#ifndef ___SOFTSERVER
#define ___SOFTSERVER

enum conn_message{
    CONNECTION_SUCCESS,
    CONNECTION_DISCONNECT,
    INVALID_IP,
    INVALID_PORT,
    CONNECTION_FAIL
};
enum send_message{
    NOT_CONNECTED,
    SEND_FAILED,
    SEND_SUCCESS
};

enum receive_message{
     RECEIVE_SUCCESS
};
class softserver{

    public:
    softserver(); //Creates an empty SoftServer object
    softserver(char _address[16], unsigned int _port); //Creates a SoftServer object
    bool change_address(char* _address, unsigned int _port);
    bool set_api_key(char _key[128]); //Sets the api key
    conn_message start_connect(); //Returns a softserver message
    conn_message disconnect();
    send_message send_obj(softobject _object);
    receive_message receive_obj(softobject _object);

    private:
    pthread_t process_th_t;
    static void* process_th(void *args);

    struct sockaddr_in address_sockaddr;

    char api_key[128]; //Very important for network
    bool has_address;
    bool is_connected;
    bool has_api_key;
    char address[16]; //ip address
    unsigned int port;
    unsigned int client_socket;
};




#endif
