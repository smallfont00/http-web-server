#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "utils/Request.hpp"
#include "utils/Response.hpp"
#include "utils/blob_reader.hpp"
#include "utils/linux_dependency.hpp"

using namespace std;

class Server {
   private:
    int server_socket;
    std::vector<std::pair<std::string, function<void(Request<> &, Response &)>>> actions;

   public:
    void Use(
        std::string path, std::function<void(Request<> &, Response &)> action = [](Request<> &, Response &) {}) {
        if ((!path.empty()) && (path[0] == '/')) path = path.substr(1);
        actions.push_back(make_pair(path, action));
    }

    void Use(std::function<void(Request<> &, Response &)> action = [](Request<> &, Response &) {}) {
        Use("/*", action);
    }

    int Listen(std::string port, std::function<void()> action) {
        if (create_server_socket(port) == -1) {
            cout << "Quit" << endl;
            return -1;
        }

        action();

        while (true) {
            int clientSocket = create_client_socket();

            if (clientSocket == -1) {
                cerr << "not accept" << endl;
                continue;
            }

            while (true) {
                auto req = Request(clientSocket);
                auto res = Response(clientSocket);

                if (req.Recv() <= 0) {
                    break;
                };

                for (auto &[path, action] : actions) {
                    if ((path == "*") || (path == req.path())) {
                        action(req, res);
                    }
                }

                if (res.Send() <= 0) {
                    break;
                };
                //close(clientSocket);
                break;
            }
            close(clientSocket);
        }

        close(this->server_socket);

        return 0;
    }

   private:
    int create_client_socket() {
        // Wait for a connection
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        int clientSocket = accept(this->server_socket, (sockaddr *)&client, &clientSize);

        char host[NI_MAXHOST];     // Client's remote name
        char service[NI_MAXSERV];  // Service (i.e. port) the client is connect on

        memset(host, 0, NI_MAXHOST);  // same as memset(host, 0, NI_MAXHOST);
        memset(service, 0, NI_MAXSERV);

        if (getnameinfo((sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            cout << host << " connected on port " << service << endl;
        } else {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << host << " connected on port " << ntohs(client.sin_port) << endl;
        }

        return clientSocket;
    }

    int create_server_socket(std::string port) {
        struct addrinfo hints, *ai, *p;

        int sockfd;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        int rv;
        if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0) {
            cerr << "selectserver: " << gai_strerror(rv) << endl;
            return -1;
        }

        int yes = 1;
        for (p = ai; p != NULL; p = p->ai_next) {
            this->server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (this->server_socket < 0) {
                continue;
            }

            setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

            if (bind(this->server_socket, p->ai_addr, p->ai_addrlen) < 0) {
                close(this->server_socket);
                continue;
            }
            break;
        }

        if (p == NULL) {
            cerr << "Can't create a socket! Quitting" << endl;
            return -1;
        }

        freeaddrinfo(ai);

        if (listen(this->server_socket, SOMAXCONN)) {
            cerr << "Can't listen" << endl;
            return -1;
        }
        return this->server_socket;
    }
};
