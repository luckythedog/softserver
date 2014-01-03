#include "softobject.h"


softobject::softobject(){
}


char* softobject::serialize(){
}

bool softobject::set_name(char _name[50]){
    memcpy(name, _name, strlen(_name));
    return true;
}


