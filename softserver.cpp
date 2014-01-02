#include "softserver.h"


softserver::softserver(){
    port = 0;
    has_address = false;
    is_connected = false;
    has_api_key = false;
}

softserver::softserver(char _address[16], unsigned int _port){
    memcpy(address, _address, sizeof(_address));
    port = _port;
    has_address = true;
    is_connected = false;
    has_api_key = false;
}

bool softserver::change_address(char* _address, unsigned int _port){
    memcpy(address, _address, sizeof(_address));
    port = _port;
    has_address = true;


    return true;
}

bool softserver::set_api_key(char _key[128]){
    memcpy(api_key, _key, sizeof(_key));
    has_api_key = true;
    return true;
}

void *softserver::process_th(void* args){
    softserver *properties = (softserver*) args;
    pollfd client_socket_poll;
    client_socket_poll.fd = properties->client_socket;
    client_socket_poll.events = POLLIN;
    pollfd* client_socket_poll_ptr;
    int poll_activity;
    while(properties->is_connected){
        client_socket_poll_ptr = &client_socket_poll;
        poll_activity = poll(client_socket_poll_ptr, 1, -1);
        if(client_socket_poll.revents & POLLIN){
            char msg_peek[250];
            if(recv(properties->client_socket, msg_peek, sizeof(msg_peek), MSG_PEEK)){
                softobject* temp_obj = (softobject*)&msg_peek;
                if(temp_obj->type ==OBJECT){
                    properties->receive_obj(*temp_obj); //Send temp_obj for them to handle*/
                }
            }
        }
    }
}

conn_message softserver::start_connect(){
    if(!is_connected && has_api_key){

        memset(&address_sockaddr, 0, sizeof(address_sockaddr));
        address_sockaddr.sin_family = AF_INET;
        address_sockaddr.sin_port = htons(port);
        address_sockaddr.sin_addr.s_addr = INADDR_ANY;
        client_socket = socket(AF_INET, SOCK_STREAM, 0);


        if(connect(  client_socket, (struct sockaddr*)&address_sockaddr, sizeof(address_sockaddr)  ) <0){
            return CONNECTION_FAIL;
        }
        pthread_create(&process_th_t, NULL, &softserver::process_th, (softserver*) this);
        pthread_tryjoin_np(process_th_t, NULL);
        is_connected = true;
        return CONNECTION_SUCCESS;
    }
}
conn_message softserver::disconnect(){
    if(is_connected){
        /*Why the fuck can I not do the close command?*/
        memset(&address_sockaddr, 0, sizeof(address_sockaddr));
        return CONNECTION_DISCONNECT;
    }
}

send_message softserver::send_obj(softobject _object){
    if(!is_connected){
    return NOT_CONNECTED;
    }
    /*This is where encryption occurs*/
    char* encryption_buffer = new char [sizeof(_object)];
    memcpy(&encryption_buffer, &_object, sizeof(_object));
    for(int i=0; i<sizeof(_object); i++){
        /*Just simple XOR encryption for now*/
        for(int b=0; b<strlen(api_key); b++){
            encryption_buffer[i] ^= api_key[b];
        }
    }
    if(send(client_socket, encryption_buffer, sizeof(_object), 0) <0){
        delete encryption_buffer;
        return SEND_FAILED;
    }
    delete encryption_buffer;
    return SEND_SUCCESS;
}

receive_message softserver::receive_obj(softobject _object){ /*This function will be overloaded by the user*/
            /*USE EVERYTHING ON OBJECT*/
}
