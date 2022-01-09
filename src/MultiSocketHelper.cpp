#include "MultiSocketHelper.h"

#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

MultiSocketHelper::MultiSocketHelper(unsigned short port, unsigned short max_clients) {
    this->port = port;
    this->max_clients = max_clients;

    master_socket = -1;
    client_sockets = (int *) malloc(sizeof(int) * max_clients);
    for (unsigned short i = 0; i < max_clients; i++) {
        client_sockets[i] = 0;
    }
}

MultiSocketHelper::~MultiSocketHelper() {
    end();
    free(client_sockets);
}

void MultiSocketHelper::init() {
    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        spdlog::error("Erreur création socket");
        throw runtime_error("Erreur création socket");
    }

    //set master socket to allow multiple connections
    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    //type of socket created
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->port);

    //bind the socket to localhost port
    if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        spdlog::error("Erreur sur bind()");
        throw runtime_error("Erreur sur bind()");
    }

    //try to specify maximum pending connections for the master socket
    if (listen(master_socket, this->max_clients) < 0) {
        spdlog::error("Erreur sur listen()");
        throw runtime_error("Erreur sur listen()");
    }

    spdlog::info("Server listening on port {}", this->port);
}

void MultiSocketHelper::end() {
    // close clients
    for (unsigned short i = 0; i < max_clients; i++) {
        if (client_sockets[i] != 0) {
            close(client_sockets[i]);
            client_sockets[i] = 0;
        }
    }

    // close server
    if (master_socket != -1) {
        close(master_socket);
        master_socket = -1;
    }
}

JsonQuery MultiSocketHelper::waitQuery() {
    char buffer[256];
    struct sockaddr_in address{};
    socklen_t addrlen = sizeof(address);

    while (true) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        int max_sd = master_socket;

        //add child sockets to set
        for (unsigned short i = 0; i < max_clients; i++) {
            //socket descriptor
            int sd = client_sockets[i];

            //if valid socket descriptor then add to read list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            //highest file descriptor number, need it for the select function
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        //wait indefinitely for an activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);

        if ((activity < 0) && (errno != EINTR)) {
            spdlog::warn("Erreur sur select()");
        }

        //If something happened on the master socket then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            int new_socket;
            if ((new_socket = accept(master_socket, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
                spdlog::error("Erreur sur accept()");
                throw runtime_error("Erreur sur accept()");
            }

            //add new socket to array of sockets
            bool found = false;
            for (unsigned short i = 0; i < max_clients; i++) {
                //if position is empty
                if (client_sockets[i] == 0) {
                    spdlog::info("Nouvelle connection {}:{}", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    client_sockets[i] = new_socket;
                    found = true;
                    break;
                }
            }

            if (!found) {
                spdlog::warn("Connection rejetée {}:{}", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                JsonResult r;
                r.status = RESPONSE_ERROR;
                r.errorMessage = "Nombre maximum de clients atteint";
                sendResponse(new_socket, r);
                close(new_socket);
            }
        }

        //else its some IO operation on some other socket
        for (unsigned short i = 0; i < max_clients; i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing, and also read the incoming message
                if (read(sd, buffer, 255) == 0) {
                    //Somebody disconnected, get his details and print
                    getpeername(sd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
                    spdlog::info("Client déconnecté {}:{}", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_sockets[i] = 0;

                } else {
                    JsonQuery q;
                    q.socket = sd;
                    // Controle que le premier caractère n'est pas null, \r ou \n
                    if (buffer[0] != 0 && buffer[0] != 10 && buffer[0] != 13) {
                        try {
                            json jsonValue = json::parse(buffer);
                            spdlog::trace("Requête client : {}", jsonValue.dump(2));
                            q.action = jsonValue["action"];
                            q.data = jsonValue["data"];
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
            }
        }
    }

}

void MultiSocketHelper::sendResponse(int socket, JsonResult response) {
    json r;
    r["status"] = response.status;
    r["action"] = response.action;
    r["errorMessage"] = response.errorMessage;
    r["data"] = response.data;

    spdlog::trace("Réponse client : {}", r.dump(2));

    ostringstream outStr;
    outStr << r.dump() << endl;

    send(socket, outStr.str().c_str(), outStr.str().length(), 0);
}
