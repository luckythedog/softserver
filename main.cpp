/******************************************************************************
 * Name: SoftServer
 * Version: Build 3
 * User interface API: Gtk+ v2. or v3.?
 * Description:
 *   General purpose server software, specializing in mobile applications development
 *   To take away the server-side scripting of projects is the goal of SoftServer.
 *****************************************************************************/
/*GTK and GDK library*/
#include <gtk/gtk.h>
#include <gdk/gdk.h>
/*C and C++ libraries*/
#include <stdlib.h>
#include <iostream>
#include <string> //I give in, let's use strings..
#include <queue>
#include <fstream>
#include <cstring>
/*Database libraries*/
#include <sqlite3.h>
/*Networking libraries*/
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
/*SoftServer libraries*/
#include "analytics.h"
/*Misc libraries*/
#include "pugixml.hpp"
/*defines*/
#define DEBUGGING true
#define CPU_DELAY 50
#define POLL_DELAY 50
#define FLOW_DELAY 25
#define CONSOLE_DELAY 25
#define MAXIMUM_CONNECTIONS_FROM_SAME_IP 8


#define NOTIFICATION_COLOR "\033[2;36m"
#define ERROR_COLOR "\033[2;31m"
#define WARNING_COLOR "\033[2;33m"
#define ESCAPE_COLOR "\033[0m"



#define BUILD_NUM 5
/*Structs*/


enum soft_account_status
{
    inactive, //not logged in
    active //logged in
};
enum soft_client_status
{
    offline,
    not_logged_in,
    online,
    server_fill

};

struct soft_account
{
    soft_account_status status;
    unsigned int uiDatabaseID; //Just Database ID for now
    soft_account()  //inactive constructor
    {
        status = inactive;
        uiDatabaseID = 0; //nothing
    }
};
struct soft_client
{
    soft_client_status status;
    unsigned int uiPosition; //position in array
    unsigned int uiSocketTCP; //tcp
    unsigned int uiSocketUDP; //udp this is for server backfill
    sockaddr_storage AddressUDP; //server's handling of Address UDP
    soft_account AccountInfo;

    /*For master socket's back fill client*/
    soft_client(soft_client_status _status, unsigned int _uiSocketTCP, unsigned int _uiSocketUDP)
    {
        status = _status;
        uiSocketTCP = _uiSocketTCP;
        uiSocketUDP = _uiSocketUDP;
    };
    /*For new clients just joining*/
    soft_client(unsigned int _uiSocketTCP, sockaddr_storage _AddressUDP)
    {
        status = not_logged_in;

        uiSocketTCP = _uiSocketTCP;
        AddressUDP = _AddressUDP;
    };
    bool set_account(soft_account _AccountInfo)
    {
        AccountInfo = _AccountInfo;

        return true;
    };

};

enum console_msg_type
{
    error,
    warning,
    notification,
    motd,
    important_offline,
    important_online
};
struct console_msg
{
    char time[16];
    console_msg_type type;
    std::string strMessage;
    std::string from_who;
};
struct init_args
{
    //Take from XML then use the data here
    bool bSuccess; //returns errors here
    unsigned int uiMaxQueue;
    unsigned int uiPortTCP;
    unsigned int uiPortUDP;
};
/*Global variables*/
unsigned int start_stop_button_ls_id; //stupid GTK+ lsid = last signal id
bool bServerOnline;
bool bConsoleActive;
sqlite3 *accounts_db;
sqlite3 *area_db;
std::queue<console_msg> qConsoleMessages;
bool stop_serv();
unsigned int uiMasterSocketTCP;
unsigned int uiMasterSocketUDP;
struct sockaddr_in SBindTCPAddress;
struct sockaddr_in SBindUDPAddress;
/*Threads*/
pthread_t console_thread;
pthread_t listen_thread;
pthread_t flow_thread;
pthread_t save_thread;
bool bThreadActive[3];
/*Listen thread necessities*/
std::vector<pollfd> rgSockets; //complete socket
std::vector<soft_client> rgClients; //These are parallel
std::vector<soft_account> rgAccounts; //This is not parallel.
std::queue<soft_account> qSaveAccounts; //These are accounts that are backed up to be saved. No pointers. This data is taken after the player has logged off
std::vector<sockaddr_storage> rgAddressWatch;
/*GTK globals*/
GtkWidget *status_msg;
GtkWidget *fixed_location;
GtkWidget *box_location;
GtkWidget *start_and_stop_button;


GtkWidget *console_text;
GtkTextBuffer *console_buffer;
GtkWidget *console_scroll;

GtkTextTag *console_format_normal;
GtkTextTag *console_format_error;
GtkTextTag *console_format_notification;
GtkTextTag *console_format_warning;
GtkTextTag *console_format_important_online;
GtkTextTag *console_format_important_offline;
GtkTextTag *console_format_motd;


struct thread_args
{
    SoftAnalytics *AnalyticsAccess;
    unsigned int uiNumber;
};
void change_status_msg(std::string _markup)
{
    gtk_label_set_markup(GTK_LABEL(status_msg), _markup.c_str());
}
void destroy(GtkWidget *widget, gpointer data)
{
    if(!bServerOnline) gtk_main_quit();
    if(bServerOnline) { bConsoleActive = false; stop_serv(); }

}
template<typename T>
std::string convert_to_str(T _num)  /*Made a little template to convert to str*/
{
    std::stringstream process;
    process << _num;
    return process.str(); //sstream is so messy looking
}
void dev_debug_msg(std::string _strMessage)
{
    if(DEBUGGING)
    {
        std::cout << "[Debug] " << _strMessage << std::endl;
    }
}

bool start_console_format()
{

    console_format_normal = gtk_text_buffer_create_tag(console_buffer, "cf1", "background", "#000000", "foreground", "#FFFFFF", NULL);
    console_format_error = gtk_text_buffer_create_tag(console_buffer, "cf2", "background", "#000000", "foreground", "#FF0000", NULL);
    console_format_notification = gtk_text_buffer_create_tag(console_buffer, "cf3", "background", "#000000", "foreground", "#02FFFF", NULL);
    console_format_warning = gtk_text_buffer_create_tag(console_buffer, "cf4", "background", "#000000", "foreground", "#FFFF00", NULL);
    console_format_important_online = gtk_text_buffer_create_tag(console_buffer, "cf5", "background", "#000000", "foreground", "#00FF00", "justification", GTK_JUSTIFY_RIGHT, NULL);
    console_format_important_offline = gtk_text_buffer_create_tag(console_buffer, "cf6", "background", "#000000", "foreground", "#FF0000", "justification", GTK_JUSTIFY_RIGHT, NULL);
    console_format_motd = gtk_text_buffer_create_tag(console_buffer, "cf7", "background", "#000000", "foreground", "#48D3FF", NULL);

    return true;
}

void* console_thread_func(void*)
{
    /*Start logs*/

    while(bConsoleActive)
    {
        while(!qConsoleMessages.empty())
        {

            console_msg front_msg = qConsoleMessages.front();
            gdk_threads_enter();
            GtkTextIter console_end_iter;
            gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter); /*Get end of console*/

            if(front_msg.type != important_online && front_msg.type != important_offline && front_msg.type != motd){

                gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.from_who.c_str(), strlen(front_msg.from_who.c_str()), console_format_normal, NULL);
                gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter); /*Get end again*/

                gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, ": ", strlen(": "), console_format_normal, NULL);/*Formating*/
                gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter);
            }
            switch(front_msg.type){ //from_who uses console_format_normal

                case notification:
                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_notification, NULL);
                break;

                case error:
                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_error, NULL);
                break;

                case warning:
                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_warning, NULL);
                break;

                case important_online:

                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_important_online, NULL);

                break;

                case important_offline:

                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_important_offline, NULL);

                break;

                case motd:

                    gtk_text_buffer_insert_with_tags(console_buffer, &console_end_iter, front_msg.strMessage.c_str(), strlen(front_msg.strMessage.c_str()), console_format_motd, NULL);
                    gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter);
                                     gtk_text_buffer_insert(console_buffer, &console_end_iter, "\n", strlen("\n"));

                break;
            }


            gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter);
            gtk_text_buffer_insert(console_buffer, &console_end_iter, "\n", strlen("\n"));
            gtk_text_buffer_get_end_iter(console_buffer, &console_end_iter);
            gtk_text_view_scroll_to_iter((GtkTextView*)console_text, &console_end_iter, 0, 0, 0, 0);
            gdk_threads_leave();
            qConsoleMessages.pop(); //pop 'em
                    poll(0,0, CONSOLE_DELAY); /*Don't want it to spout all out and shit*/
        }


    }

    /*Clean up and save logs here*/
    return NULL;
}
bool activate_console()
{
    if(!bConsoleActive)
    {
        bConsoleActive = true;
        pthread_create(&console_thread, NULL, console_thread_func, NULL);
        return true;
    }
    return false;
}
void add_console_msg(std::string _from_who, console_msg_type _type, std::string _strMessage)
{
    console_msg new_message;
    new_message.type = _type;
    new_message.strMessage = _strMessage;

    snprintf(new_message.time, 16, "%lu", time(NULL));
    new_message.from_who = _from_who;
    qConsoleMessages.push(new_message);

    if(DEBUGGING == true) //Displays to dev console
    {
        if(_type == notification)
        {
            std::cout << _from_who << ": " << NOTIFICATION_COLOR << _strMessage<< ESCAPE_COLOR << std::endl;
        }
        if (_type == error)
        {
            std::cout << _from_who << ": "  << ERROR_COLOR  << _strMessage<< ESCAPE_COLOR << std::endl;
        }
        if(_type == warning)
        {
            std::cout << _from_who << ": "<<  WARNING_COLOR << _strMessage << ESCAPE_COLOR << std::endl;
        }

    }
}
void* save_thread_func(void *args)
{
    bThreadActive[2] = true;
    thread_args* my_args = (thread_args*) args;
    add_console_msg("[Save]",notification, "Thread has started");
    while(bServerOnline)
    {
        while(!qSaveAccounts.empty())
        {
            add_console_msg("[Save]",notification, "Saving account");
            /*They don't save at all. Use SQLite*/
            qSaveAccounts.pop();
            add_console_msg("[Save]", notification, "Account saved");
        }
        poll(0,0,CPU_DELAY);
    }

    my_args = NULL;
    add_console_msg("[Save]", notification, "Thread has exited successfully");
    bThreadActive[2] = false;
    return NULL;
}
void *flow_thread_func(void* args)
{
    bThreadActive[1] = true;
    thread_args* my_args = (thread_args*) args;
    add_console_msg("[Flow]",notification, "Thread has started");



    while(bServerOnline)
    {

        poll(0,0, FLOW_DELAY); //should rename it to something else so it'll work with both TCP and UDP
    }

    my_args = NULL; //Point my_args NULL
    add_console_msg("[Flow]",notification, "Thread has exited successfully");
    bThreadActive[1] = false;
    return NULL;
}

void *listen_thread_func(void* args)  //TCP's thread
{
    bThreadActive[0] = true;
    thread_args* my_args = (thread_args*) args;
    add_console_msg("[Listen]",notification, "Thread has started");


    add_console_msg("[Listen]",notification, "Doing safe cleanup of Sockets and Clients");
    if(rgSockets.size() > 0)
    {
        rgSockets.clear();
    }
    if(rgClients.size() > 0 )
    {
        rgClients.clear();
    }
    if(rgAccounts.size() > 0)
    {
        rgAccounts.clear();
    }


    pollfd MasterSocket;
    MasterSocket.fd = uiMasterSocketTCP;
    MasterSocket.events = POLLIN;
    rgSockets.push_back(MasterSocket);

    soft_client MasterClient(server_fill, uiMasterSocketTCP, uiMasterSocketUDP);
    rgClients.push_back(MasterClient);

    soft_account MasterAccount;
    MasterClient.set_account(MasterAccount);
    rgAccounts.push_back(MasterAccount);



    int PollActivity;
    pollfd* ptrPollSockets;
    add_console_msg("[Listen]",notification, "Poll is ready to read incoming connections");

    while(bServerOnline)
    {

        ptrPollSockets = &rgSockets[0];
        while(bServerOnline)
        {
            PollActivity = poll(ptrPollSockets, rgSockets.size(), POLL_DELAY);
            if(PollActivity !=0) break;
        }
        if(PollActivity < 0)
        {
            perror("tcp-poll");
            add_console_msg("[Listen]",warning, "Could not poll sockets");
        }
        if(rgSockets[0].revents & POLLIN)  //Server received a connection
        {
            unsigned int uiNewSocket;
            struct sockaddr_storage SNewClientAddr;
            socklen_t iNewClientAddr_Size = sizeof SNewClientAddr;
            if((uiNewSocket = accept(uiMasterSocketTCP, (struct sockaddr*)&SNewClientAddr, &iNewClientAddr_Size)) < 0)
            {
                perror("tcp-accept");
                add_console_msg("[Listen]", warning, "Failed to accept client");
            }
            else
            {

                add_console_msg("[Listen]", notification, "A client has successfully connected"); //Remove this later

                /*Create a pollfd for new socket*/
                pollfd NewSocket;
                NewSocket.fd = uiNewSocket;
                NewSocket.events = POLLIN;
                rgSockets.push_back(NewSocket);
                /*Create a new client for socket*/
                soft_client NewClient(uiNewSocket, SNewClientAddr); //takes TCP socket and udp address and port

                soft_account InactiveAccount; //Creates a blank account with inactivityand databaseID equals to 0
                NewClient.set_account(InactiveAccount); //This does not increment accounts logged in. You must LOG in to do so.
                rgClients.push_back(NewClient);
                rgAccounts.push_back(InactiveAccount);
                change_status_msg("<span foreground='green'><b>Online</b></span> <b>"+convert_to_str(rgSockets.size()-1)+"</b> user(s) connected");
                /*Add to Address Watchlist*/
                rgAddressWatch.push_back(SNewClientAddr);
            }
        }
        for(unsigned int i=1; i<rgSockets.size(); i++)
        {
            if(i != rgClients[i].uiPosition) rgClients[i].uiPosition = i; //Take uiPosition
            if(rgSockets[i].revents & POLLIN)
            {
                int read_val;
                char chBuffer[50];
                if((read_val = (recv(rgSockets[i].fd, chBuffer, 50, 0))) !=0)
                {



                    /*Players get linked to an account here*/

                }
                else
                {

                    /*Client has disconnected*/
                    add_console_msg("[Listen]", notification, "A client has disconnected");
                    rgClients[i].status = offline; //He will be taken care of in the next step. To keep order
                }
            }
        }
        /*Sweep disconnected players*/
        for(unsigned int i=1; i<rgClients.size(); i++)
        {
            if(rgClients[i].status == offline)
            {
                if(rgClients[i].AccountInfo.status == active)
                {
                    /*Save the account so far*/
                    add_console_msg("[Listen]",notification, "Client's account was pushed to the save queue");
                    qSaveAccounts.push(rgAccounts[i]); //Put in save queue
                }

                close(rgClients[i].uiSocketTCP); //Closes their TCP socket

                rgSockets.erase(rgSockets.begin()+i);
                rgClients.erase(rgClients.begin()+i); // then remove
                rgAccounts.erase(rgAccounts.begin()+i); //remove accounts whether inactive or not

                add_console_msg("[Listen]",notification, "Client and socket have been removed from polling");
                change_status_msg("<span foreground='green'><b>Online</b></span> <b>"+convert_to_str(rgSockets.size()-1)+"</b> user(s) connected");
            }

        }
        poll(0,0,CPU_DELAY); //Reduce CPU usage

    }
    ptrPollSockets = NULL;
    my_args = NULL;
    add_console_msg("[Listen]",notification, "Thread has exited successfully");
    bThreadActive[0] = false;
    return NULL;
}
bool check_if_all_threads_running()  //Checks if all the threads are running
{
    bool bAllRunning = true;
    for(unsigned int i=0; i<sizeof(bThreadActive)/sizeof(bool); i++)
    {
        if(bThreadActive[i] == false) bAllRunning = false;
    }

    return bAllRunning;
}

bool check_if_all_threads_completed()  //Checks if all threads stopped
{
    bool bAllStopped = true;
    for(unsigned int i=0; i<sizeof(bThreadActive)/sizeof(bool); i++)
    {
        if(bThreadActive[i] == true) bAllStopped = false;
    }

    return bAllStopped;
}
bool load_area_db()
{
    add_console_msg("[Database]", notification,"Loading database/area.db");
    int result_loading_db = sqlite3_open("database/area.db", &area_db);

    if (result_loading_db)
    {
        add_console_msg("[Database]", error,"Could not load database/area.db");
        return false;
    }
    else
    {
        add_console_msg("[Database]", notification,"Area database has successfully loaded");
        return true;
    }
}
bool load_accounts_db()
{
    add_console_msg("[Database]", notification,"Loading database/accounts.db");
    int result_loading_db = sqlite3_open("database/accounts.db", &accounts_db);

    if (result_loading_db)
    {
        add_console_msg("[Database]", error,"Could not load database/accounts.db");
        return false;
    }
    else
    {
        add_console_msg("[Database]", notification,"Accounts database has successfully loaded");
        return true;
    }

}

init_args load_external_files()
{
    /*Load server settings from XML*/
    add_console_msg("[Server]", notification,"Loading settings/server.xml");
    init_args args;
    args.bSuccess = true; //Successful
    args.uiPortTCP = 65535; //take port and maybe ip address.
    args.uiPortUDP = 65535;
    args.uiMaxQueue = 129; //unlimited

    add_console_msg("[Server]", notification, "is running on TCP Port "+convert_to_str(args.uiPortTCP));
    add_console_msg("[Server]", notification, "is running on UDP Port "+convert_to_str(args.uiPortUDP));
    add_console_msg("[Server]", notification, "has a maximum queue backlog of "+convert_to_str(args.uiMaxQueue));
    if (args.uiPortTCP < 0 || args.uiPortTCP > 65535)
    {
        add_console_msg("[Server]", error, "TCP Port must be between 0 and 65535");
        args.bSuccess = false; //False and quit
    }
    if( args.uiPortUDP < 0 || args.uiPortUDP > 65535)
    {
        add_console_msg("[Server]", error, "UDP Port must be between 0 and 65535");
        args.bSuccess = false;
    }
    if(!args.bSuccess)  return args;
    if(args.uiMaxQueue < 0 || args.uiMaxQueue > SOMAXCONN)
    {
        add_console_msg("[Server]", warning, "Max queue backlog must be in between 0 and " + convert_to_str(SOMAXCONN));
        add_console_msg("[Server]",warning,"Setting it to 0 by default (unlimited)");
        args.uiMaxQueue = 0;
    }
    /*Load database after correct settings*/
    while(!load_accounts_db()); /*Loads sqlite database*/

    while(!load_area_db());

    return args;
}
bool init_serv()
{
    if(!bServerOnline)  //Make sure it is offline.
    {
        activate_console();
        gtk_widget_set_sensitive(start_and_stop_button, false); /*Gray it out*/

        init_args xml_args = load_external_files();
        if(!xml_args.bSuccess)
        {
            gtk_widget_set_sensitive(start_and_stop_button, true);
            add_console_msg("[Server]",error,"has ended init_serv() process");
            return false; //End server start up right away
        }

        bool bErrorFree = true;

        for(unsigned int i=0; i<sizeof(bThreadActive)/sizeof(bool); i++) //Start all threads false
        {
            bThreadActive[i] = false;

        }

        uiMasterSocketTCP = socket(AF_INET, SOCK_STREAM, 0);
        if(uiMasterSocketTCP < 0)
        {
            perror("tcp-socket");
            add_console_msg("[Init]",error,"Could not initialize TCP socket");
            return false;
        }

        uiMasterSocketUDP = socket(AF_INET, SOCK_DGRAM, 0);
        if(uiMasterSocketUDP < 0)
        {
            perror("udp-socket");
            add_console_msg("[Init]",error,"Could not initialize UDP socket");
            return false;
        }

        memset(&SBindTCPAddress, 0, sizeof(SBindTCPAddress)); //Memset to make sure
        memset(&SBindUDPAddress, 0, sizeof(SBindUDPAddress)); //Memset to make sure

        SBindTCPAddress.sin_family = AF_INET;
        SBindUDPAddress.sin_family = AF_INET;
        SBindTCPAddress.sin_port = htons(xml_args.uiPortTCP);
        SBindUDPAddress.sin_port = htons(xml_args.uiPortUDP);
        SBindTCPAddress.sin_addr.s_addr = INADDR_ANY;
        SBindUDPAddress.sin_addr.s_addr = INADDR_ANY;



        if(bind(uiMasterSocketTCP, (struct sockaddr*) &SBindTCPAddress, sizeof SBindTCPAddress) < 0)
        {
            perror("tcp-bind"); //Perrors are only for us to debug
            add_console_msg("[Init]",error, "Could not bind TCP"); //These are for the end-user
            bErrorFree = false;
            return false;
        }
        if(bind(uiMasterSocketUDP, (struct sockaddr*) &SBindUDPAddress, sizeof SBindUDPAddress)  < 0)
        {
            perror("udp-bind");
            add_console_msg("[Init]",error, "Could not bind UDP");
            bErrorFree = false;
            return false;
        }
        if(listen(uiMasterSocketTCP, xml_args.uiMaxQueue) < 0)
        {
            perror("tcp-listen");
            add_console_msg("[Init]",error, "Could not listen on TCP socket");
            bErrorFree = false;
            return false;
        }

        bServerOnline = bErrorFree;
        if(bErrorFree)
        {


            /*listen thread*/
            thread_args listen_thread_args;
            pthread_create(&listen_thread, NULL, listen_thread_func, &listen_thread_args);

            /*flow thread*/
            thread_args flow_thread_args;
            pthread_create(&flow_thread, NULL, flow_thread_func, &flow_thread_args);

            thread_args save_thread_args;
            pthread_create(&save_thread, NULL, save_thread_func, &save_thread_args);

            while(true)
            {
                poll(0,0, CPU_DELAY);
                if(check_if_all_threads_running())
                {
                    gtk_widget_set_sensitive(start_and_stop_button, true); /*ungray*/
                    gtk_button_set_label((GtkButton*)start_and_stop_button, "Stop");
                    g_signal_handler_disconnect(start_and_stop_button, start_stop_button_ls_id);
                    start_stop_button_ls_id = g_signal_connect_after(start_and_stop_button, "released", G_CALLBACK(stop_serv), NULL);
                    change_status_msg("<span foreground='green'><b>Online</b></span> <b>0</b> user(s) connected");
                    add_console_msg("[Server]", important_online, "[Server is now online]");
                    break;
                }

            }


            return true;
        }
    }
    return false;
}

bool stop_serv()
{
    if(bServerOnline)  //Make sure it actually is online
    {
        gtk_widget_set_sensitive(start_and_stop_button, false);
        bServerOnline = false; //Make sure the server is actually off
        add_console_msg("[Main]",notification,"Waiting for all threads to complete");
        while(!check_if_all_threads_completed());
         /*Wait for safety!*/
        add_console_msg("[Main]",notification,"All threads have exited safely");

        for(unsigned int i=0; i<rgClients.size(); i++)
        {
            if(i==0) close(rgClients[0].uiSocketUDP);
            close(rgClients[i].uiSocketTCP);
        }
         /*Wait for safety!*/
        add_console_msg("[Main]",notification,"All sockets (including server's) have been closed");
        /*Back up all players here*/
        add_console_msg("[Main]",notification,"Pushing all accounts to save queue");
        for(unsigned int i=0; i<rgAccounts.size(); i++)
        {
            if(rgAccounts[i].status == active)
            {
                qSaveAccounts.push(rgAccounts[i]);
            }
        }

        add_console_msg("[Main]",notification,"Waiting for saving queue to finish");
        while(!qSaveAccounts.empty()); //Block until qSaveAccounts has been all worked upon
         /*Wait for safety!*/

        /*Cleaning up for next time init*/
        add_console_msg("[Main]",notification,"Cleaning up existing list of sockets, clients, and accounts");
        rgAccounts.clear();
         /*Wait for safety!*/
        rgClients.clear();
         /*Wait for safety!*/
        rgSockets.clear(); //Clear these parallel mother fuckers!

        /*Close databases*/
        add_console_msg("[Database]", notification,"Closing database/accounts.db");
        sqlite3_close(accounts_db);
        accounts_db = NULL;
        add_console_msg("[Database]", notification,"Accounts database has successfully closed");
         /*Wait for safety!*/
        add_console_msg("[Database]", notification,"Closing database/area.db");
        sqlite3_close(area_db);
        accounts_db = NULL;
        add_console_msg("[Database]", notification,"Area database has successfully closed");

        gtk_button_set_label((GtkButton*)start_and_stop_button, "Start");
        g_signal_handler_disconnect(start_and_stop_button, start_stop_button_ls_id);
        start_stop_button_ls_id = g_signal_connect_after(start_and_stop_button, "released", G_CALLBACK(init_serv), NULL);

        add_console_msg("[Main]",notification,"Server has stopped successfully");
        change_status_msg("<span foreground='red'><b>Offline</b></span> The server has stopped successfully");

        add_console_msg("[Server]", important_offline, "[Server is now offline]");
        poll(0,0,25);
        gtk_widget_set_sensitive(start_and_stop_button, true);
        bConsoleActive = false; //Turn off console
        return true;
    }
    return false;
}

int main (int argc, char *argv[])
{
    /*init critical variables*/
    bServerOnline = false;
    bConsoleActive = false;

    /*init resources*/
    GtkWidget *window;


    /*init threads*/
    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    add_console_msg("[Lucky",motd, "SoftServer Build "+convert_to_str(BUILD_NUM)+"\nhttp://www.softserver.org\n\nAre you still using the trial version?\nDevelop an app using SoftServer and submit it to earn a license!");
    add_console_msg("[Main]",notification,"Initializing GTK+ v2.0");
    /*init GTK*/
    gtk_init(&argc, &argv);

    /*Window setup*/
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), std::string("SoftServer Build "+convert_to_str(BUILD_NUM)).c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), 550, 350);
    gtk_window_set_resizable(GTK_WINDOW(window), false);
    /*Console*/

    console_text = gtk_text_view_new(); //Start console up
    console_buffer = gtk_text_view_get_buffer((GtkTextView*)console_text);
    while(!start_console_format()); //Start up formats
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(console_text), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), false);
    gtk_widget_set_size_request(console_text, 595, 345);

    GdkColor console_color_bg;
    console_color_bg.red = 0;
    console_color_bg.green = 0;
    console_color_bg.blue = 0;

    gtk_widget_modify_base(console_text, GTK_STATE_NORMAL, &console_color_bg);





    /*Label setup*/
    status_msg = gtk_label_new(NULL);
    change_status_msg("SoftServer <i>Build "+convert_to_str(BUILD_NUM)+"</i>");
    start_and_stop_button = gtk_button_new_with_label("Start");

    fixed_location = gtk_fixed_new();
    gtk_fixed_put(GTK_FIXED(fixed_location), console_text,0,0);
    gtk_fixed_put(GTK_FIXED(fixed_location), status_msg, 5, 360);
    gtk_fixed_put(GTK_FIXED(fixed_location), start_and_stop_button, 550, 350);

    gtk_container_add(GTK_CONTAINER(window), fixed_location);

    /*Show widgets here*/
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);
    gtk_widget_show_all(window);


    add_console_msg("[Main]", notification,"Waiting for start button to be pressed");

    start_stop_button_ls_id = g_signal_connect_after(start_and_stop_button, "released", G_CALLBACK(init_serv), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);
    activate_console();
    /*Exit*/
    gtk_main();
    gdk_threads_leave();

    return 0;
}
