#define CONSOLE_MAX_MESSAGE_LENGTH 50
/************************************************************************************
                  SERVER RELATED STRUCTURES AND ENUMERATION TYPES
These are used for server debugging or just server management in general. These will
never be seen by the end-user (of the SoftServer product) or the clients (the clients of
the end-users).
************************************************************************************/
enum SOFT_Debug_ConsoleMessage_Type{
    Error,
    Alert,
    Notification
};
struct SOFT_Debug_ConsoleMessage{
    SOFT_Debug_ConsoleMessage_Type type;
    char message[CONSOLE_MAX_MESSAGE_LENGTH];
};
/************************************************************************************
              NETWORKING RELATED STRUCTURES AND ENUMERATION TYPES
These are used for networking between client and server. These are the things that will
be seen by end-users and clients of the end-users.
************************************************************************************/
enum SOFT_Packet_Type{ //This is the general packet type
    Packet_AccountLogin,
    Packet_AccountRegister,
    Packet_AccountLoginFacebook,
    Packet_AccountRegisterFacebook
};
struct SOFT_Packet{
    SOFT_Packet_Type type;
};

