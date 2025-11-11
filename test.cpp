#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>

using namespace std;

#define PORT 8080
#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 128

int main()
{

    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        throw runtime_error("tcp not created");
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(PORT);
    socklen_t addrlen = sizeof(server_address);

    // remove this as it forcefully uses the port
    int opt = 1;
    setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int bindStatus = bind(tcpSocket, (struct sockaddr *)&server_address, addrlen);
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
        int clientSocket = accept(tcpSocket, (struct sockaddr *)&server_address, &addrlen);
        if (clientSocket == -1)
        {
            throw runtime_error("connection not establisihed");
        }

        char buffer[BUFFER_SIZE];
        int recieved = 0;
        // while((recieved = recv(clientSocket,buffer,BUFFER_SIZE,0))>0){
        //     send(clientSocket,buffer,recieved,0);
        // }

        // while ((recieved = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0)
        // {

        //     cout << recieved << "  " << buffer << "\n\nfinished request\n\n";

        //     const char *response =
        //         "HTTP/1.1 200 OK\r\n"
        //         "Content-Type: text/html\r\n"
        //         "Content-Length: 13\r\n"
        //         "\r\n"
        //         "Hello, World!";

        //     send(clientSocket, response, strlen(response), 0);
        // }

        if ((recieved = recv(clientSocket, buffer, BUFFER_SIZE, 0)) == -1)
        {
            throw runtime_error("Client not connected");
        }

        istringstream requestStream(buffer);
        string method, uri, httpVersion;
        requestStream >> method >> uri >> httpVersion;

        if (method != "GET")
        {
            string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
            send(clientSocket, response.c_str(), response.length(), 0);
            close(clientSocket);
        }
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
        close(clientSocket);
    }
    close(tcpSocket);
}