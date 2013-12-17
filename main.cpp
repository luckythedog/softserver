/*************************
 * Soft Server w/ GUI Build 2
 * Description: General purpose server software
 * Project website: http://www.github.com/luckythedog/softserver/
 * Graphical User Interface API: GTK+
 *************************/
/*GTK library*/
#include "gtk/gtk.h"
/*Standard libraries*/
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <poll.h>
#include <errno.h>
#include <cstdlib>
#include <queue>

#ifndef _WIN32
/*These libraries will work for both MAC OS X and Linux*/
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
/*Adding compatibility for Windows Sockets*/
#include <winsocks.h>
#endif

/*Defines*/
#define DEBUG_ON_CONSOLE 1
#define DEFAULT_PORT 6666
#define REFRESH_RATE 100 //Lowering this may increase speeds but drag on your CPU

#define USERNAME_MAX_LENGTH 50
#define DISPLAY_MAX_LENGTH 50
#define PASSWORD_MAX_LENGTH 50
#define EMAIL_MAX_LENGTH 100

#define MESSAGE_MAX_LENGTH 150

typedef unsigned int UINT;
/*************************************************************************
Soft_Type_Error and the structure Soft_Error_Message are used for server admin-
strators error messages. Clients will NEVER see this!
*************************************************************************/
enum Soft_Type_Console {
        Error,
        Notification
};
struct Soft_Console_Message {
        Soft_Type_Console Type;
        char chMessage[50];
};

/*************************************************************************
This is a general packet. Anything used for communication between server
and client will inherit this.
*************************************************************************/
enum Soft_Type_Packet {
        Private_Message,

        Secure_Register,
        Secure_Login,
        Secure_Register_With_Facebook,
        Secure_Login_With_Facebook,

        Secure_Message
};

struct Soft_Packet {
        Soft_Type_Packet Type;
        unsigned int uiPacketSize;
};

/*************************************************************************
Soft_Client struct is the most basic client there is. There is a difference between
socket and client. Client is when you have logged in, but we still use socket to
send and receive messages. Attributes are a part of a client.
*************************************************************************/
enum Soft_Type_Client_Status {
        Online,
        Busy,
        Offline,
        Not_Logged_In,
        Server_Fill
};
struct Soft_Client_Attributes {
        char chName[25];
        char chValue[25];
};
struct Soft_Client {
        unsigned int uiSizeOfClientPacket; //We will need variable Sizes of Soft_Client Packet since we got customizable attributes
        Soft_Type_Client_Status Status;
        unsigned int uiDatabaseID;
        unsigned int uiSocket;
        char chAddress[30];
        int uiPort;
        std::vector<Soft_Client_Attributes> rgAttributes;
};
/*************************************************************************
These are private messages that are sent from player to player, it has the sender's
database ID and receiver's ID
*************************************************************************/
struct Soft_Private_Message : Soft_Packet {
        unsigned int uiDatabaseIDSender;
        unsigned int uiDatabaseIDReceiver;
        char chMessage[MESSAGE_MAX_LENGTH];
};

/*************************************************************************
This is Soft Server's secure Login and Registration. More social integrations will be
implemented.
*************************************************************************/
struct Soft_Secure_Register : Soft_Packet {
        char chUsername[USERNAME_MAX_LENGTH];
        char chPassword[PASSWORD_MAX_LENGTH];
        char chEmail[EMAIL_MAX_LENGTH];
};

struct Soft_Secure_Login : Soft_Packet  {
        char chEmail[EMAIL_MAX_LENGTH]; //Only requires email to log in
        char chPassword[DISPLAY_MAX_LENGTH];
};

struct Soft_Secure_Register_With_Facebook  : Soft_Packet  {
        unsigned int uiFacebookID; //We may need more since we want to integrate Facebook too
};

struct Soft_Secure_Login_With_Facebook  : Soft_Packet {
        unsigned int uiFacebookID;
};

struct Soft_Secure_Message : Soft_Packet {

        char chMessage[50]; //Invalid Login, Already registered, everything that fits the above.
};

/*************************************************************************
These are queue structs
*************************************************************************/
struct Soft_Queue_Secure {
        Soft_Type_Packet Method;
        char chUsername[USERNAME_MAX_LENGTH];
        char chPassword[PASSWORD_MAX_LENGTH];
        char chEmail[EMAIL_MAX_LENGTH];
        unsigned int uiFacebookID;
        Soft_Client *sClient;
};
struct Soft_Queue_Save {
        unsigned int uiPosition;
        unsigned int uiSocket;
        Soft_Client *sClient;
};




/*Global variables*/
bool bServerOnline;
unsigned int uiMasterSocket;
std::vector<Soft_Console_Message> rgConsoleMessages;
std::vector<pthread_t> rgThreads;

std::vector<pollfd> rgSockets; //Parallel sockets
std::vector<Soft_Client> rgClients; //Yes they are

/*Queues and their semaphores/mutexes*/
std::queue<Soft_Queue_Secure> qSecure;
sem_t smSecure;

std::queue<Soft_Queue_Save> qSave;
pthread_mutex_t muSave;



/*GTK Widgets*/
GtkWidget *window;

void console_msg(Soft_Type_Console type, const char msg[50])
{
        Soft_Console_Message ConsoleAlert;
        ConsoleAlert.Type = type;
        strcpy(ConsoleAlert.chMessage, msg);

        rgConsoleMessages.push_back(ConsoleAlert);
        if(DEBUG_ON_CONSOLE) {
                std::cout << msg << std::endl;
        }
}
void* save_func_thread(void*)
{
        /*Handles save sweeping*/
        while(bServerOnline) {
                if(!qSave.empty()) {
                        /*Verify that person is really offline*/
                        Soft_Queue_Save sRequest;
                        sRequest = qSave.front();

                        if(sRequest.sClient->Status == Offline) {
                                /*Let's verify if the position given to us is correct.*/
                                if(rgSockets[sRequest.uiPosition].fd == sRequest.uiSocket) {
                                        /*Save here in MYSQL*/

                                        /*Removal*/
                                        pthread_mutex_lock(&muSave);
                                        rgClients.erase(rgClients.begin()+sRequest.uiPosition);
                                        rgSockets.erase(rgSockets.begin()+sRequest.uiPosition);
                                        pthread_mutex_unlock(&muSave);
                                }
                        }
                        qSave.pop();
                }
        }
}

void send_secure_msg_to_sock(unsigned int uiSocket){
    Soft_Secure_Message sMessage;
    sMessage.Type = Secure_Message;
    strcpy(sMessage.chMessage, "You are already logged in");

    send(sRequest.sClient->uiSocket, (char*)&sMessage, sizeof(sMessage),0);
}

void* secure_func_thread(void*)
{
        /*Handles account registration and login while server is online*/
        while(bServerOnline) {
                /*Wait for semaphore*/
                sem_wait(&smSecure);
                if(!qSecure.empty() ) {
                        /*Let's get it loaded*/
                        Soft_Queue_Secure sRequest;
                        sRequest = qSecure.front();
                        /*Check to see if already logged in*/
                        if(sRequest.sClient->Status == Not_Logged_In) {
                                /*Work on sRequest now*/
                                switch (sRequest.Method) {
                                case Secure_Login:
                                        break;
                                case Secure_Login_With_Facebook:
                                        break;
                                case Secure_Register:
                                        break;
                                case Secure_Register_With_Facebook:
                                        break;

                                }
                        } else {
                            send_secure_msg_to_sock(sRequest.sClient->uiSocket);
                        }

                        /*Pop that shit!*/
                        qSecure.pop();
                }

        }
}


void* connection_func_thread(void*)
{
        /*Mandatory variables for polling sockets*/
        int iPollActivity;
        pollfd* ptrPollSockets;

        /*Create a poll socket of the master socket*/
        pollfd master_socket;
        master_socket.fd = uiMasterSocket;
        master_socket.events = POLLIN;

        /*Create a blank client of the master socket to keep parallelism up*/
        Soft_Client master_client;
        master_client.Status = Server_Fill;
        master_client.uiSocket = uiMasterSocket;

        /*Push them into the vector*/
        rgSockets.push_back(master_socket);
        rgClients.push_back(master_client);


        while(bServerOnline) {
                ptrPollSockets = &rgSockets[0];
                iPollActivity = poll(ptrPollSockets, rgSockets.size(), -1);
                if(iPollActivity < 0) {
                        console_msg(Error, "Could not poll sockets");
                }
                /*See if anyone is trying to connect on Master Socket*/
                if(rgSockets[0].revents & POLLIN) {
                        int iClientSocket;
                        struct sockaddr_in SClientAddress;
                        int iClientAddrLen = sizeof(SClientAddress);
                        if((iClientSocket = accept(uiMasterSocket, (struct sockaddr*)&SClientAddress, (socklen_t*) &iClientAddrLen)) > -1) {
                                //Add new socket
                                pollfd add_socket;
                                add_socket.fd = iClientSocket;
                                add_socket.events = POLLIN;
                                //Add new blank client
                                Soft_Client add_client;
                                add_client.Status = Not_Logged_In;
                                add_client.uiSocket = iClientSocket;
                                strcpy(add_client.chAddress, inet_ntoa(SClientAddress.sin_addr));
                                add_client.uiPort = ntohs(SClientAddress.sin_port);
                                //Push them
                                pthread_mutex_lock(&muSave);
                                rgSockets.push_back(add_socket);
                                rgClients.push_back(add_client);
                                pthread_mutex_unlock(&muSave);
                        } else {
                                /*Error in reading in socket*/
                                console_msg(Error, "Could not accept client socket");
                        }

                }
                pthread_mutex_lock(&muSave);
                for(int i=1; i<rgSockets.size(); i++) {
                        /*Peek from client*/
                        if(rgSockets[i].revents & POLLIN) {
                                Soft_Packet sPeekPacket;
                                unsigned int uiLocalSocket = rgSockets[i].fd;
                                if((recv(uiLocalSocket , (char*)&sPeekPacket,sizeof(Soft_Packet), MSG_PEEK)) !=0) {
                                        /*Read in packet size*/
                                        char sReadPacket[sPeekPacket.uiPacketSize];
                                        read(uiLocalSocket, sReadPacket, sPeekPacket.uiPacketSize);

                                        /*If case matches, then typecast it*/
                                        switch(sPeekPacket.Type) {

                                        case Secure_Login:
                                                Soft_Secure_Login PacketA;
                                                memcpy(&PacketA, &sReadPacket, sizeof(sReadPacket));

                                                Soft_Queue_Secure QueueA;
                                                memcpy(&QueueA.Method, &PacketA.Type, sizeof(PacketA.Type));
                                                memcpy(&QueueA.chEmail, &PacketA.chEmail, sizeof(PacketA.chEmail));
                                                memcpy(&QueueA.chPassword, &PacketA.chPassword, sizeof(PacketA.chPassword));

                                                QueueA.sClient = &rgClients[i];
                                                qSecure.push(QueueA);

                                                break;

                                        case Secure_Login_With_Facebook:
                                                Soft_Secure_Login_With_Facebook PacketB;
                                                memcpy(&PacketB, &sReadPacket, sizeof(sReadPacket));

                                                Soft_Queue_Secure QueueB;
                                                memcpy(&QueueB.Method, &PacketB.Type, sizeof(PacketB.Type));
                                                memcpy(&QueueB.uiFacebookID, &PacketB.uiFacebookID, sizeof(PacketB.uiFacebookID));

                                                QueueB.sClient = &rgClients[i];
                                                qSecure.push(QueueB);
                                                break;

                                        case Secure_Register:
                                                Soft_Secure_Register PacketC;
                                                memcpy(&PacketC, &sReadPacket, sizeof(sReadPacket));

                                                Soft_Queue_Secure QueueC;
                                                memcpy(&QueueC.Method, &PacketC.Type, sizeof(PacketC.Type));
                                                memcpy(&QueueC.chUsername, &PacketC.chUsername, sizeof(PacketC.chUsername));
                                                memcpy(&QueueC.chEmail, &PacketC.chEmail, sizeof(PacketC.chEmail));
                                                memcpy(&QueueC.chPassword, &PacketC.chPassword, sizeof(PacketC.chPassword));

                                                QueueC.sClient = &rgClients[i];
                                                qSecure.push(QueueC);
                                                break;

                                        case Secure_Register_With_Facebook:
                                                Soft_Secure_Register_With_Facebook PacketD;
                                                memcpy(&PacketD, &sReadPacket, sizeof(sReadPacket));

                                                Soft_Queue_Secure QueueD;
                                                memcpy(&QueueD.Method, &PacketD.Type, sizeof(PacketD.Type));
                                                memcpy(&QueueD.uiFacebookID, &PacketD.uiFacebookID, sizeof(PacketD.uiFacebookID));

                                                QueueD.sClient = &rgClients[i];
                                                qSecure.push(QueueD);
                                                break;
                                        }

                                } else {
                                        rgClients[i].Status = Offline;
                                        Soft_Queue_Save DisconnectedPlayer;
                                        DisconnectedPlayer.sClient = &rgClients[i];
                                        DisconnectedPlayer.uiSocket = rgSockets[i].fd;
                                        DisconnectedPlayer.uiPosition = i;
                                        qSave.push(DisconnectedPlayer);
                                }
                        }
                }
                pthread_mutex_unlock(&muSave);


                poll(0,0,REFRESH_RATE);
        }
}


static void init_serv (GtkWidget *widget, gpointer data)
{

        bool bErrorFree = true; //Check if it is error free.

        if(!bServerOnline) {
                /*Create Master Socket*/
                uiMasterSocket = socket(AF_INET, SOCK_STREAM, 0);
                if(uiMasterSocket < 0) {
                        console_msg(Error, "Socket has not been intialized");
                        bErrorFree = false;
                }

                /*Create structure for address*/
                struct sockaddr_in SMasterAddress;
                memset(&SMasterAddress, 0, sizeof(SMasterAddress));
                SMasterAddress.sin_port = htons(DEFAULT_PORT);
                SMasterAddress.sin_family = AF_INET;
                SMasterAddress.sin_addr.s_addr = INADDR_ANY;

                /*Bind socket and structure*/
                if(bind (uiMasterSocket, (struct sockaddr*)&SMasterAddress, sizeof(SMasterAddress) ) <0) {
                        console_msg(Error, "Could not bind socket");

                        bErrorFree = false;
                }


                /*Listen on socket*/
                if(listen(uiMasterSocket, 0)) {
                        console_msg(Error, "Could not listen on socket");

                        bErrorFree = false;
                }
                /*Semaphores and mutexes here*/
                if((sem_init(&smSecure, 0,0)) != 0) {
                        console_msg(Error, "Cannot start secure semaphore");
                        bErrorFree = false;

                }
                if(pthread_mutex_init(&muSave,NULL) != 0) {
                        console_msg(Error, "Cannot start save semaphore");
                        bErrorFree = false;
                }

                if(bErrorFree) {
                        /*Start threads otherwise display error messages*/
                        //Thread intialization here*/
                        pthread_t connection_thread;
                        rgThreads.push_back(connection_thread);
                        pthread_create(&rgThreads[0], NULL, connection_func_thread, NULL);

                        pthread_t secure_thread;
                        rgThreads.push_back(secure_thread);
                        pthread_create(&rgThreads[1], NULL, secure_func_thread, NULL);

                        pthread_t save_thread;
                        rgThreads.push_back(save_thread);
                        pthread_create(&rgThreads[2], NULL, save_func_thread, NULL);

                        /*Join all threads*/
                        for(int i=0; i<rgThreads.size(); i++) {
                                pthread_join(rgThreads[i], NULL);
                        }
                        console_msg(Notification, "Server is now online");

                } else {
                        console_msg(Error, "Server could not be started, Please try again");

                }

        }

}

static void stop_serv (GtkWidget *widget, gpointer data)
{
    if(bServerOnline){

        /*Should check if threads are finished*/
        for(int i=0;i<rgThreads.size(); i++){
            pthread_exit(&rgThreads[i]);
        }

        for(int i=0;i<rgSockets.size(); i++){
            close(rgSockets[i].fd);
        }

        rgThreads.clear();
        rgSockets.clear();
        rgClients.clear();
        while(!qSecure.empty()){
            qSecure.pop();
        }
        while(!qSave.empty()){
            qSave.pop();
        }
        sem_destroy(&smSecure);
        pthread_mutex_destroy(&muSave);

        bServerOnline = false;
        console_msg(Notification, "Server is now offline");
    }
}

int main(int argc, char* argv[])
{

        /*Make sure server is not online*/
        bServerOnline = false;

        /*Initialize window*/
        gtk_init(&argc, &argv);
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Window");
        g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit), NULL);

        /*No buttons, but if there were some -- this is what the start button should call*/
        init_serv(NULL, NULL);

        /*Show window and main loop*/
        gtk_widget_show(window);
        gtk_main();

        return 0;
}
