#include "SocketHelper.h"

#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

SocketHelper::SocketHelper(string socketType, int backlogQueue) {
    if (socketType.length() == 0) {
        cerr << "Socket type ne peut être vide" << endl;
        throw invalid_argument("SocketHelper constructeur : socketType");
    }

    this->socketType = socketType;
    this->port = DEFAULT_PORT;
    this->backlogQueue = backlogQueue;
}

void SocketHelper::setPort(int port) {
    this->port = port;
}

void SocketHelper::setSocketFile(string socketFile) {
    if (socketFile.length() == 0) {
        cerr << "SocketFile ne peut être vide" << endl;
        throw invalid_argument("SocketHelper.setSocketFile : socketFile");
    }
    this->socketFile = socketFile;
}

bool SocketHelper::isInet() {
    return socketType.compare(SOCKET_INET) == 0;
}

bool SocketHelper::isUnix() {
    return socketType.compare(SOCKET_UNIX) == 0;
}

bool SocketHelper::isUnknown() {
    return !isInet() && !isUnix();
}

void SocketHelper::init() {
    spdlog::debug("Initialisation de la socket ...");
    if (this->isInet()) {
        initSocketInet();
    } else if (this->isUnix()) {
        initSocketUnix();
    } else {
        spdlog::error("Type de socket non supportée");
        throw invalid_argument("Type de socket non supportéé. Valid 'unix' et 'inet'");
    }

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    listen(serverSockfd, backlogQueue);

    // Default value without client are connected
    clientSockfd = DEFAULT_SOCKET_FD;
}

void SocketHelper::initSocketInet() {
    spdlog::info("Initialisation de la socket INET sur le port {}", this->port);

    struct sockaddr_in serv_addr_inet;

    // create a socket
    serverSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // socket(int domain, int type, int protocol)

    int iSetOption = 1;
    setsockopt(serverSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));

    bzero((char *) &serv_addr_inet, sizeof(serv_addr_inet)); // clear address structure

    /* setup the host_addr structure for use in bind call */
    serv_addr_inet.sin_family = AF_INET; // server byte order
    serv_addr_inet.sin_addr.s_addr = INADDR_ANY; // automatically be filled with current host's IP address
    serv_addr_inet.sin_port = htons(this->port); // convert short integer value for port must be converted into network byte order

    // bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    // bind() passes file descriptor, the address structure,
    // and the length of the address structure
    // This bind() call will bind the socket to the current IP address on port, portno
    if (bind(serverSockfd, (struct sockaddr *) &serv_addr_inet, sizeof(serv_addr_inet)) < 0) {
        spdlog::error("Erreur sur bind() INET");
        throw bad_function_call();
    }
}

void SocketHelper::initSocketUnix() {
    if (this->socketFile.length() == 0) {
        spdlog::error("Nom de fichier de socket unix vide");
        throw invalid_argument("Nom de fichier de socket unix vide");
    }

    removeSocketFile();

    spdlog::info("Initialisation de la socket UNIX sur le fichier {}", this->socketFile);

    struct sockaddr_un serv_addr_un;

    // create a socket
    serverSockfd = socket(AF_UNIX, SOCK_STREAM, 0); // socket(int domain, int type, int protocol)
    bzero((char *) &serv_addr_un, sizeof(serv_addr_un)); // clear address structure

    /* setup the host_addr structure for use in bind call */
    serv_addr_un.sun_family = AF_UNIX; // server byte order
    strcpy(serv_addr_un.sun_path, this->socketFile.c_str());

    // bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    // bind() passes file descriptor, the address structure,
    // and the length of the address structure
    // This bind() call will bind the socket to the current IP address on port, portno
    if (bind(serverSockfd, (struct sockaddr *) &serv_addr_un, sizeof(serv_addr_un)) < 0) {
        spdlog::error("Erreur sur bind() UNIX");
        throw bad_function_call();
    }
}

void SocketHelper::waitConnection() {
    closeClient();

    spdlog::debug("Attente de connexion sur la socket ...");
    if (this->isInet()) {
        waitConnectionInet();
    } else if (this->isUnix()) {
        waitConnectionUnix();
    } else {
        spdlog::error("Type de socket non supportée");
        throw invalid_argument("Type de socket non supportée. Valid 'unix' et 'inet'");
    }
}

void SocketHelper::waitConnectionInet() {
    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr_in);

    // This accept() function will write the connecting client's address info
    // into the the address structure and the size of that structure is clilen.
    // The accept() returns a new socket file descriptor for the accepted connection.
    // So, the original socket file descriptor can continue to be used
    // for accepting new connections while the new socker file descriptor is used for
    // communicating with the connected client.
    clientSockfd = accept(serverSockfd, (struct sockaddr *) &cli_addr_in, &clilen);
    if (clientSockfd < 0) {
        spdlog::error("Erreur sur accept() INET");
        throw bad_function_call();
    }

    spdlog::info("Connection INET avec le client {}:{}", inet_ntoa(cli_addr_in.sin_addr), ntohs(cli_addr_in.sin_port));
}

void SocketHelper::waitConnectionUnix() {
    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr_un);

    // This accept() function will write the connecting client's address info
    // into the the address structure and the size of that structure is clilen.
    // The accept() returns a new socket file descriptor for the accepted connection.
    // So, the original socket file descriptor can continue to be used
    // for accepting new connections while the new socker file descriptor is used for
    // communicating with the connected client.
    clientSockfd = accept(serverSockfd, (struct sockaddr *) &cli_addr_un, &clilen);
    if (clientSockfd < 0) {
        spdlog::error("Erreur sur accept() UNIX");
        throw bad_function_call();
    }

    spdlog::info("Connection UNIX avec le client sur le fichier {}", socketFile);
}

JsonQuery SocketHelper::getQuery(void) {
    char buffer[256];
    bzero(buffer, 256);
    if (read(clientSockfd, buffer, 255) < 0) {
        spdlog::error("Erreur de lecture read()");
    }

    JsonQuery q;
    // Controle que le premier caractère n'est pas null, \r ou \n
    if (buffer[0] != 0 && buffer[0] != 10 && buffer[0] != 13) {
        try {
            json jsonValue = json::parse(buffer);
            spdlog::debug("Requête client : {}", jsonValue.dump(2));
            q.action = jsonValue["action"];
            q.datas = jsonValue["datas"];
        } catch (const exception & e) {
            spdlog::error("Erreur de lecture du JSON : {}", e.what());
            q.action = DATA_UNPARSABLE;
        }
    } else {
        spdlog::error("Requête vide du client");
        q.action = DATA_INVALID;
    }

    return q;
}

void SocketHelper::sendResponse(JsonResult response) {
    json r;
    r["status"] = response.status;
    r["action"] = response.action;
    r["errorMessage"] = response.errorMessage;
    r["datas"] = response.datas;

    spdlog::debug("Réponse client : {}", r.dump(2));

    ostringstream outStr;
    outStr << r.dump() << endl;

    send(clientSockfd, outStr.str().c_str(), outStr.str().length(), 0);
}

void SocketHelper::end() {
    closeClient(true);
    closeServer();

    if (isUnix()) {
        removeSocketFile();
    }
}

void SocketHelper::closeClient(bool force) {
    // Close the socket client if not équals the default value
    if (clientSockfd != DEFAULT_SOCKET_FD || force) {
        close(clientSockfd);
        clientSockfd = DEFAULT_SOCKET_FD;
    }
}

void SocketHelper::closeServer() {
    close(serverSockfd);
}

void SocketHelper::removeSocketFile() {
    // Suppression du fichier si il éxiste (technique rapide POSIX stat)
    struct stat buf;
    if (stat(socketFile.c_str(), &buf) == 0) {
        spdlog::debug("Le fichier socket {} existe, on le supprime", socketFile);
        // Suppression du fichier socket
        if (remove(socketFile.c_str()) != 0) {
            spdlog::error("Suppression du fichier échouée.");
        }
    }
}