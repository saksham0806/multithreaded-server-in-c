#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <unordered_map>

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
#define BUFFER_SIZE 8192

// function forward declarations
void configureServerAdress(struct sockaddr_in &serverAddress);
void handleGET(int clientSocket, string uri);
int getContentLength(char buffer[BUFFER_SIZE]);
string getContent(char buffer[BUFFER_SIZE]);
void handlePOST(int clientSocket, char buffer[BUFFER_SIZE]);
void handleClientSocket(int clientSocket);
unordered_map<string, string> processBody(string body);

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

void handlePOST(int clientSocket, char buffer[BUFFER_SIZE])
{
    string body = getContent(buffer);
    unordered_map<string, string> formData = processBody(body);

    // debugging
    //  for (const auto &pair : formData)
    //  {
    //      cout << pair.first << " : " << pair.second << endl;
    //  }

    return;
}

int getContentLength(char buffer[BUFFER_SIZE])
{
    char *contentLengthString = strstr(buffer, "Content-Length");
    istringstream ContentLengthStream(contentLengthString);
    string a, b, c, d;
    ContentLengthStream >> a >> b >> c >> d;
    int ContentLength = stoi(b);
    // cout<<a<<b<<c<<d<<endl;
    return ContentLength;
}

string getContent(char buffer[BUFFER_SIZE])
{
    int contentLength = getContentLength(buffer);
    char *bodyStart = strstr(buffer, "\r\n\r\n");
    if (!bodyStart)
    {
        return "";
    }
    // cout<<body<<endl;
    bodyStart += 4;
    string body(bodyStart, contentLength);
    // cout<<body<<endl;
    return body;
}

unordered_map<string, string> processBody(string body)
{
    unordered_map<string, string> mp;
    string key = "", value = "";
    bool flag = true;
    for (int i = 0; i < body.size(); i++)
    {
        if (flag)
        {
            if (body[i] == '=')
            {
                flag = false;
                continue;
            }
            key += body[i];
        }
        else
        {
            if (body[i] == '&')
            {
                flag = true;
                mp[key] = value;
                key = "";
                value = "";
                continue;
            }
            else if (body[i] == '+')
            {
                value += " ";
                continue;
            }
            value += body[i];
        }
    }
    if (!key.empty())
        mp[key] = value;
    return mp;
}

void handleClientSocket(int clientSocket)
{
    // debugging
    // pid_t tid = syscall(SYS_gettid);
    // cout << "Thread started with TID: " << tid << endl;

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

    // cout<<buffer<<endl;

    if (method == "GET")
    {
        handleGET(clientSocket, uri);
    }
    else if (method == "POST")
    {
        handlePOST(clientSocket, buffer);
    }
    else
    {
        string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
        send(clientSocket, response.c_str(), response.length(), 0);
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
        cout << "waiting\n";
        int clientSocket = accept(tcpSocket, (struct sockaddr *)&serverAddress, &addrlen);
        cout << "accepted\n";
        if (clientSocket == -1)
        {
            throw runtime_error("connection not establisihed");
        }

        handleClientSocket(clientSocket);

        // thread clientThread(handleClientSocket, clientSocket);
        // clientThread.detach();
        // cout << "thread detached\n";
    }
    close(tcpSocket);
    exit(0);
}