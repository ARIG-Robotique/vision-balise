#ifndef MULTISOCKETHELPER
#define MULTISOCKETHELPER

#include <sys/select.h>
#include "common.h"

class MultiSocketHelper {

private:
    fd_set readfds;
    unsigned short port;
    unsigned short max_clients;
    int master_socket;
    int* client_sockets;

public:
    MultiSocketHelper(unsigned short port = 8686, unsigned short max_clients = 4);
    ~MultiSocketHelper();

    void init();
    void end();

    JsonQuery waitQuery();
    void sendResponse(int socket, JsonResult response);
};


#endif //MULTISOCKETHELPER
