#include <iostream>
#include <unistd.h>
#include <string.h>

// libraries for creating network
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>

// libraries for multithreaded concurrency
#include <thread>
#include <chrono>
#include <sys/syscall.h>

using namespace std;

#define PORT 8080
#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 128

void configureServerAdress(struct sockaddr_in &serverAddress)
{
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PORT);
}

void handleGET(int clientSocket, string uri)
{
    string filepath = "./public" + uri;
    if (uri == "/")
    {
        filepath = "public/index.html";
    }

    ifstream file(filepath, ios::binary);
    if (!file.is_open())
    {
        const char *response =
            "HTTP/1.1 404 NOT FOUND\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "File not found";
        send(clientSocket, response, strlen(response), 0);
    }
    else
    {
        ostringstream ss;
        ss << file.rdbuf();
        string content = ss.str();
        string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            to_string(content.size()) + "\r\n"
                                        "\r\n";

        send(clientSocket, header.c_str(), header.size(), 0);
        send(clientSocket, content.c_str(), content.size(), 0);
    }
}

void handleClientSocket(int clientSocket)
{
    // debugging
    pid_t tid = syscall(SYS_gettid);
    cout << "Thread started with TID: " << tid << endl;

    char buffer[BUFFER_SIZE];
    int recieved = 0;
    recieved = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if ((recieved) == -1)
    {
        throw runtime_error("Client not connected");
    }
    else if (recieved == 0)
    {
        cout << "connection terminated";
    }
    
    istringstream requestStream(buffer);
    string method, uri, httpVersion, host;
    requestStream >> method >> uri >> httpVersion;
    cout<<buffer;

    cout << host;
    // if (method != "GET")
    // {
    //     string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
    //     send(clientSocket, response.c_str(), response.length(), 0);
    //     close(clientSocket);
    // }
    if(method == "GET"){
        handleGET(clientSocket,uri);
    }

    close(clientSocket);
}

int main()
{

    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        throw runtime_error("tcp not created");
    }

    struct sockaddr_in serverAddress;
    configureServerAdress(serverAddress);
    socklen_t addrlen = sizeof(serverAddress);

    // remove this as it forcefully uses the port
    int opt = 1;
    setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int bindStatus = bind(tcpSocket, (struct sockaddr *)&serverAddress, addrlen);
    if (bindStatus == -1)
    {
        throw runtime_error("binding failed");
    }

    int listenStatus = listen(tcpSocket, MAX_CONNECTIONS);
    if (listenStatus == -1)
    {
        throw runtime_error("port listening error");
    }

    while (true)
    {
        cout << "waiting" << endl;
        int clientSocket = accept(tcpSocket, (struct sockaddr *)&serverAddress, &addrlen);
        cout << "accepted\n";
        if (clientSocket == -1)
        {
            throw runtime_error("connection not establisihed");
        }

        // cout<<"restarted\n";             //why this run 3 times
        // handleClientSocket(clientSocket);
        thread clientThread(handleClientSocket, clientSocket);
        clientThread.detach();
        cout << "thread detached\n";
    }
    close(tcpSocket);
    exit(0);
}