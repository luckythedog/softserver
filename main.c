/*************************
 * Soft Server w/ GUI Build 1
 * Description: General purpose server software
 * Project website: http://www.github.com/luckythedog/softserver/
 * Graphical User Interface API: GTK+
 *************************/
/*Necessary libraries*/
#include <stdlib.h>
#include <gtk/gtk.h>

#include <pthread.h>

/*Check if Windows or not! Mac OS X works on same BSD Unix sockets as Linux*/
#ifndef WIN32
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>
#else
#include <winsocks2.h>
#endif

#define PORT 6666

/*Global variables*/
unsigned short int uiMasterSocket;
struct sockaddr_in SMasterAddress;

/*Global GTK widgets*/
GtkWidget *status_label;
GtkWidget *start_serv_button;
GtkWidget *menu_box;
GtkWidget *port_textbox;
gboolean bOnline = FALSE;
gboolean bStopProcess = FALSE;


gboolean init_serv(unsigned int_port);

gboolean stop_serv() {
    /*Clear everything out*/
    close(uiMasterSocket);
    memset(&SMasterAddress,0,sizeof(SMasterAddress));

    bOnline = FALSE;
    gtk_button_set_label(GTK_BUTTON(start_serv_button),"Start");
    gtk_label_set(GTK_LABEL(status_label),"Server has been stopped.");
    g_signal_connect(start_serv_button,"clicked", G_CALLBACK(init_serv), NULL);

}
gboolean init_serv(unsigned int _port) {
    if(!bOnline) {
        gboolean bSuccessful = TRUE;
        /*Initialize Master Socket*/
        if(uiMasterSocket = socket(AF_INET, SOCK_STREAM, 0)< 0) {
            gtk_label_set(GTK_LABEL(status_label),"Could not start socket.");
            bSuccessful = FALSE;
        }

        /*Initialize Master Address struct*/
        memset(&SMasterAddress,0,sizeof(SMasterAddress));
        struct sockaddr_in SMasterAddress;
        SMasterAddress.sin_port = htons(_port);
        SMasterAddress.sin_family = AF_INET;
        SMasterAddress.sin_addr.s_addr = INADDR_ANY;

        if(bind(uiMasterSocket, (struct sockaddr*)&SMasterAddress, sizeof(SMasterAddress)) < 0) {

            gtk_label_set(GTK_LABEL(status_label),"Could not bind socket.");
            bSuccessful = FALSE;
        }

        if(listen(uiMasterSocket,0)< 0) {
            gtk_label_set(GTK_LABEL(status_label),"Could not listen on socket.");
            bSuccessful = FALSE;
        }



        /*Set server status online*/
        if(bSuccessful) {
            gtk_button_set_label(GTK_BUTTON(start_serv_button),"Stop");
            gtk_label_set(GTK_LABEL(status_label),"Server is now online!");
            g_signal_connect(start_serv_button,"clicked", G_CALLBACK(stop_serv), NULL);
            bOnline = TRUE;
        }


    }

}

void* connectionThread(void* nothing) {
    while(bOnline) {
    }
}

int main(int argc, char* argv[]) {
    /*GTK Init*/
    gtk_init(&argc, &argv);

    /*Initialize window*/
    GtkWidget *window;

    /*GTK Window setup*/
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window,"destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    /*Widgets setup*/
    status_label = gtk_label_new("Server has not be initiated.");

    start_serv_button = gtk_button_new_with_label("Start");
    g_signal_connect(start_serv_button, "clicked", G_CALLBACK(init_serv), NULL);

    port_textbox = gtk_entry_new_with_max_length(5);
    gtk_entry_set_width_chars(GTK_ENTRY(port_textbox), 5);



    menu_box = gtk_fixed_new();


    /*Add widgets to window*/
    gtk_container_add(GTK_CONTAINER(window), menu_box);
    gtk_fixed_put(GTK_FIXED(menu_box), start_serv_button, 350,165);
    gtk_fixed_put(GTK_FIXED(menu_box), status_label, 5,175);
    gtk_fixed_put(GTK_FIXED(menu_box),port_textbox, 350,135);



    /*testing threads*/
    pthread_t t1;
    pthread_create(&t1, NULL, connectionThread, NULL);
    pthread_tryjoin_np(t1, NULL);


    /*Show window and run main loop*/
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
