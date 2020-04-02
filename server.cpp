#include <iostream>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include <functional>
#include <utility>
#include <string>
#include <unordered_map>
#include <regex>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <stdlib.h>
#include <filesystem>
#include <memory>

#include <chrono>
#include <future>
#include <thread>

namespace fs = std::filesystem;
using namespace std;

typedef std::unordered_map<std::string, std::string> Message;

std::string cgi_handler(std::string path, std::string query = "", std::string body = "")
{
    int cgiInput[2];
    int cgiOutput[2];
    int status;
    char *inputData = {"Hello world"};
    pid_t cpid;
    char c;

    char buffer[4096] = {0};

    /* Use pipe to create a data channel betweeen two process
         'cgiInput'  handle  data from 'host' to 'CGI'
         'cgiOutput' handle data from 'CGI' to 'host'*/
    if (pipe(cgiInput) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(cgiOutput) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* Creates a new process to execute cgi program */
    if ((cpid = fork()) < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /*child process*/
    if (cpid == 0)
    {
        printf("this is child process\n");

        //close unused fd
        close(cgiInput[1]);
        close(cgiOutput[0]);

        //redirect the output from stdout to cgiOutput
        dup2(cgiOutput[1], STDOUT_FILENO);

        //redirect the input from stdin to cgiInput
        dup2(cgiInput[0], STDIN_FILENO);

        //after redirect we don't need the old fd
        close(cgiInput[0]);
        close(cgiOutput[1]);

        /* execute cgi program
               the stdout of CGI program is redirect to cgiOutput
               the stdin  of CGI program is redirect to cgiInput
         */
        execlp(path.c_str(), path.c_str(), NULL);
        exit(0);
    }

    /*parent process*/
    else
    {
        std::string input = "";
        if (query != "")
            input += query + "\n";
        if (body != "")
            input += body + "\n";

        printf("this is parent process\n");

        //close unused fd
        close(cgiOutput[1]);
        close(cgiInput[0]);

        // send the message to the CGI program

        write(cgiInput[1], input.c_str(), input.size());

        read(cgiOutput[0], buffer, sizeof(buffer));

        // connection finish
        close(cgiOutput[0]);
        close(cgiInput[1]);

        waitpid(cpid, &status, 0);

        return string(buffer);
    }
}

std::string read_file(std::string path)
{
    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

class SimpleHttpParser
{
private:
public:
    Message parser(string str)
    {

        const std::regex DOUBLE_LINE("\r\n\r\n");
        const std::regex SINGLE_LINE("\n");
        const std::regex SPACE("\\s+");
        const std::regex QUESTION("\\?");
        Message msg;
        std::vector<std::string> v1(std::sregex_token_iterator(str.begin(), str.end(), DOUBLE_LINE, -1),
                                    std::sregex_token_iterator());

        if (v1[0] == "")
        {
            return msg;
        }

        std::vector<std::string> v2(std::sregex_token_iterator(v1[0].begin(), v1[0].end(), SINGLE_LINE, -1),
                                    std::sregex_token_iterator());

        std::vector<std::string> v3(std::sregex_token_iterator(v2[0].begin(), v2[0].end(), SPACE, -1),
                                    std::sregex_token_iterator());

        std::vector<std::string> v4(std::sregex_token_iterator(v3[1].begin(), v3[1].end(), QUESTION, -1),
                                    std::sregex_token_iterator());
        msg["Query"] = "";
        msg["Body"] = "";
        msg["Method"] = v3[0];

        msg["Path"] = v4[0];

        if (v4.size() == 2)
        {
            msg["Query"] = v4[1];
        }

        if (v1.size() == 2)
        {
            msg["Body"] = v1[1];
        }

        for (int i = 1; i < v2.size(); i++)
        {
            auto offset = v2[i].find_first_of(':');
            auto first = v2[i].substr(0, offset);
            auto second = v2[i].substr(offset + 2, v2[i].size() - offset);
            msg[first] = second;
        }
        return msg;
    }
};

class Request
{
};
class Response
{
};

template <typename HttpParserType>
class Server : HttpParserType
{
private:
    int server_socket;

public:
    void Get(
        std::string path, std::function<void(Request &, Response &)> action = [](Request &, Response &) {})
    {
    }

    void Post(
        std::string path, std::function<void(Request &, Response &)> action = [](Request &, Response &) {})
    {
    }

    void Static(
        std::string path, std::function<void(Request &, Response &)> action = [](Request &, Response &) {})
    {
    }

    int Listen(uint16_t port, std::function<void()> action)
    {
        action();

        create_server_socket(port);

        char buf[4096];

        while (true)
        {
            memset(buf, 0, 4096);

            int clientSocket = create_client_socket();

            if (clientSocket == -1)
            {
                cerr << "not accept" << endl;
            }
            // Wait for client to send data
            int bytesReceived = -1;

            while (true)
            {
                cout << "receiving..." << endl;

                bytesReceived = recv(clientSocket, buf, 4096, 0);

                cout << "ok!" << endl;

                if (bytesReceived == -1)
                {
                    cerr << "request error" << endl;
                    break;
                }

                if (bytesReceived == 0)
                {
                    cout << "close connection" << endl;
                    break;
                }

                cout << bytesReceived << endl;

                auto msg = HttpParserType::parser(buf);

                //cout << "Method: " << msg["Method"] << endl;
                //cout << "Path: " << msg["Path"] << endl;

                if (msg["Path"] == "/")
                {
                    msg["Path"] = "index.html";
                }

                std::error_code target_error;
                auto static_path = fs::canonical("static/");
                auto target_path = fs::canonical("static/" + msg["Path"], target_error);

                if (target_error.value())
                {
                    cerr << target_error.message() << " : " << msg["Path"] << endl;
                    std::string str = "HTTP/1.1 404 Not Found\n\n";
                    str += "Connection: close\n";
                    send(clientSocket, str.c_str(), str.size(), 0);
                    //close(clientSocket);
                    break;
                }

                if (target_path.string().find_first_of(static_path.string()) != 0)
                {
                    cerr << "outside the static path" << endl;
                    //close(clientSocket);
                    break;
                }
                auto extension = target_path.extension().string();

                std::string content_type = "";
                std::string body = "";

                if (extension == ".jpg")
                {
                    content_type = "image/jpeg";
                    body = read_file(target_path);
                }
                else if (extension == ".html")
                {
                    content_type = "text/html";
                    body = read_file(target_path);
                }
                else if (extension == ".cgi")
                {
                    const auto method = msg["Method"];
                    body = cgi_handler(target_path, msg["Query"], msg["Body"]);
                }
                // Echo message back to client
                std::string content_length = "Content-Length";
                content_length += (body.size());

                std::string str = "";
                str += "HTTP/1.1 200 OK\n";
                str += "Content-Type:" + content_type + "\n";
                str += "Connection: close\n";
                str += content_length + "\n";

                str += "\n";
                str += body;

                //cout << str << endl;

                send(clientSocket, str.c_str(), str.size(), 0);
                //close(clientSocket);
                break;
            }
            close(clientSocket);
        }

        close(this->server_socket);

        return 0;
    }

private:
    int create_client_socket()
    {
        // Wait for a connection
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        int clientSocket = accept(this->server_socket, (sockaddr *)&client, &clientSize);

        char host[NI_MAXHOST];    // Client's remote name
        char service[NI_MAXSERV]; // Service (i.e. port) the client is connect on

        memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
        memset(service, 0, NI_MAXSERV);

        if (getnameinfo((sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
        {
            cout << host << " connected on port " << service << endl;
        }
        else
        {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << host << " connected on port " << ntohs(client.sin_port) << endl;
        }

        return clientSocket;
    }

    int create_server_socket(uint16_t port)
    {
        this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (this->server_socket == -1)
        {
            cerr << "Can't create a socket! Quitting" << endl;
            return -1;
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

        bind(this->server_socket, (sockaddr *)&hint, sizeof(hint));

        // Tell Winsock the socket is for listening
        listen(this->server_socket, SOMAXCONN);
        return this->server_socket;
    }
};

void test_http_parser()
{
    SimpleHttpParser f;

    auto msg = f.parser(read_file("test_case.txt"));

    cout << msg["Path"] << endl;
}

int main()
{
    // test_http_parser();
    Server<SimpleHttpParser> app;
    app.Listen(54000, []() -> void {
        cout << "Fuck??" << endl;
    });
}
