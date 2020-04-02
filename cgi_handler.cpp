#include <iostream>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

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
using namespace std;

//unique_ptr<future<void>> asy;

void cgi_handler(std::string path)
{
    int cgiInput[2];
    int cgiOutput[2];
    int status;
    char *inputData = {"Hello world"};
    pid_t cpid;
    char c;

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
        printf("this is parent process\n");

        //close unused fd
        close(cgiOutput[1]);
        close(cgiInput[0]);

        // send the message to the CGI program
        write(cgiInput[1], inputData, strlen(inputData));

        while (read(cgiOutput[0], &c, 1) > 0)
        {
            // output the message to terminal

            write(STDOUT_FILENO, &c, 1);
        }

        send(STDOUT_FILENO, "\n", 1, 0);

        // connection finish
        close(cgiOutput[0]);
        close(cgiInput[1]);

        waitpid(cpid, &status, 0);
    }
}

int main()
{

    auto asy1 = (async(std::launch::async, [&]() {
        cout << "1" << endl;
        cgi_handler("static/cgi_demo.cgi");
        cout << "2" << endl;
    }));
    auto asy2 = (async(std::launch::async, [&]() {
        cout << "3" << endl;
        cgi_handler("static/cgi_demo.cgi");
        cout << "4" << endl;
    }));
}