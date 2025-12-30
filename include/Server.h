#ifndef SERVER_H
#define SERVER_H

struct Server {
    int id;           
    int current_load; 
    bool is_active; // NEW: Tracks if server is online or dead

    Server(int server_id) {
        id = server_id;
        current_load = 0;
        is_active = true; // Default is online
    }
};

#endif
