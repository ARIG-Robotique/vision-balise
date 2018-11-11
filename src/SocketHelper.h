#ifndef SOCKETHELPER_H
#define SOCKETHELPER_H

#include <netinet/in.h>
#include <sys/un.h>
#include "common.h"

#define SOCKET_UNIX     "unix"
#define SOCKET_INET     "inet"

using namespace std;

class SocketHelper {

public:
    SocketHelper(string socketType, int backlogQueue = 5);

    void init();

    void waitConnection();

    JsonQuery getQuery(void);
    void sendResponse(JsonResult response);

    void end();

    bool isUnix();
    bool isInet();
    bool isUnknown();
    void setPort(int port);
    void setSocketFile(string socketFile);

private:
    static const int DEFAULT_SOCKET_FD = -1;
    static const int DEFAULT_PORT = 8686;

    string socketType, socketFile;
    int serverSockfd, clientSockfd, port, backlogQueue;

    socklen_t clilen;
    struct sockaddr_in cli_addr_in;
    struct sockaddr_un cli_addr_un;

    void initSocketInet();
    void initSocketUnix();
    void waitConnectionInet();
    void waitConnectionUnix();
    void closeClient(bool force = false);
    void closeServer();
    void removeSocketFile();
};

#endif // SOCKETHELPER_H
