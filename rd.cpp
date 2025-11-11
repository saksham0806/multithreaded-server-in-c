#include <iostream>
#include <unistd.h>
#include <string.h>

//libraries for creating network
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>

//libraries for multithreaded concurrency
#include <thread>
#include <chrono>

using namespace std;

void sleeper(){
    cout<<"started\n";
    cout<<this_thread::get_id<<endl;
    sleep(20);
    cout<<"finished";

}

int main(){
    thread test(sleeper);
    test.detach();
}