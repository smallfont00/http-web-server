#pragma once

#include <iostream>

#include "linux_dependency.hpp"

class Response {
   private:
    int clientSocket;
    Message msg;

   public:
    Response &status(int code) {
        std::string CODE;
        switch (code) {
            case 200:
                CODE = "HTTP/1.1 200 OK\n";
                break;
            case 404:
                CODE = "HTTP/1.1 404 Not Found\n";
                break;
            default:
                CODE = "HTTP/1.1 505 Internal Server Error\n";
                break;
        }
        msg["Code"] = std::move(CODE);
        return *this;
    }
    Response &content_type(std::string type) {
        msg["Content-Type"] = std::move("Content-Type: " + type + "\n");
        return *this;
    }
    Response &body(std::string body) {
        msg["Content-Length"] = "Content-Length: " + std::to_string(body.size()) + "\n";
        msg["Body"] = std::move(body);
        return *this;
    }
    Response &connection(bool open) {
        if (open) {
            msg["Connection"] = "Connection: keep-alive\n";
        } else {
            msg["Connection"] = "Connection: close\n";
        }
        return *this;
    }
    Response(int clientSocket) : clientSocket(clientSocket) {
        msg["Connection"] = "Connection: close\n";
    };
    int Send() {
        std::string str;

        str += msg["Code"];
        str += msg["Content-Type"];
        str += msg["Connection"];
        str += msg["Content-Length"];

        if (str.back() == '\n') str.back() = '\r';
        str += "\n\r\n";

        str += msg["Body"];

        return send(clientSocket, str.c_str(), str.size(), 0);
    }
    std::string &operator[](const std::string &key) {
        return msg[key];
    }
};