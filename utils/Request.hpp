#pragma once

#include <iostream>

#include "linux_dependency.hpp"
#include "SimpleHttpParser.hpp"

template <typename HttpParserType = SimpleHttpParser>
class Request : HttpParserType {
   private:
    char buf[4096];
    int clientSocket;
    Message msg;

   public:
    Request(int clientSocket) : clientSocket(clientSocket){};
    std::string &path() {
        return msg["Path"];
    }
    std::string &query() {
        return msg["Query"];
    }
    std::string &body() {
        return msg["Body"];
    }
    std::string &content_type() {
        return msg["Content_Type"];
    }
    int Recv() {
        std::cout << "receiving..." << std::endl;
        int bytesReceived = recv(clientSocket, buf, 4096, 0);
        std::cout << "ok!" << std::endl;

        if (bytesReceived == -1) {
            std::cerr << "request error" << std::endl;
            return bytesReceived;
        }

        if (bytesReceived == 0) {
            std::cerr << "close connection" << std::endl;
            return bytesReceived;
        }

        buf[bytesReceived] = 0;

        msg = HttpParserType::parser(buf);

        msg["Path"] = msg["Path"].substr(1);
        return bytesReceived;
    }
    std::string &operator[](const std::string &key) {
        return msg[key];
    }
};