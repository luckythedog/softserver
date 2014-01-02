/*This is the main object of SoftServer*/
#include <iostream>
#include <vector>
#ifndef ___SOFTOBJECT
#define ___SOFTOBJECT
enum softserver_typepacket{
    OBJECT,
    MESSAGE
};

class softobject{
    public:
    softobject(); //Create's an empty softobject to send
    softserver_typepacket type;




    private:
    unsigned int client_socket;


};


#endif
