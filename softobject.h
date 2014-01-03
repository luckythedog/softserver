/*This is the main object of SoftServer*/
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include "softattribute.h"
#include "softattributearray.h"
#include <map>
#ifndef ___SOFTOBJECT
#define ___SOFTOBJECT



enum softserver_typepacket{
    OBJECT,
    MESSAGE
};

class softobject{
    public:
    softobject(); //Create's an empty softobject to send. They will use this though.
    softobject(const char* _data); //Deserialize never ever

    bool set_name(char _name[50]);
    char* serialize(); //Serialize data.. Client or clientside api user will never use this EVER
    softserver_typepacket type;




    private:
    char name[50];
    unsigned int client_socket;



};


#endif
