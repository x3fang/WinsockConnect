#ifndef GLOBAL_H_
#define GLOBAL_H_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif
#if __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#define SOCKET int
#define INVALID_SOCKET -1
#define BOOL bool
#define Sleep sleep
#define SOCKET_ERROR -1
#define closesocket close
#define WSAGetLastError() errno
#define SOCKADDR sockaddr
#endif
#include <stdio.h>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <thread>
#include <time.h>
#include <atomic>
#include <algorithm>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "MD5.h"
#include "getCmd.h"
#define _DEBUG
using std::atomic;
using std::cin;
using std::condition_variable;
using std::cout;
using std::endl;
using std::fstream;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::map;
using std::mutex;
using std::ofstream;
using std::queue;
using std::string;
using std::thread;
using std::to_string;
using std::vector;
typedef struct ClientSocketFlagStruct
{
    string SEID;
    string ClientWanIp;
    string ClientLanIp;
    unsigned long long int OnlineTime, OfflineTime;
    int ClientConnectPort;
    enum states
    {
        NULLs = 0,
        Online = 1,
        Offline = 2,
        Use = 3
    };
    states state;
    bool operator==(const ClientSocketFlagStruct &e)
    {
        return (this->ClientLanIp == e.ClientLanIp && this->ClientWanIp == e.ClientWanIp);
    }
};
typedef struct SEIDForSocketStruct
{
    SOCKET socketH;
    bool isSocketExit;
    bool isSEIDExit;
    bool isBack;
    bool isUse;
    string SEID;
    SOCKET ServerSocket;
    mutex isSocketExitLock;
    mutex ServerSocketLock;
    mutex ServerHealthySocketLock;
    mutex OtherValueLock;
    std::condition_variable cv;
    atomic<bool> serverSocketLock;
    atomic<bool> serverHealthySocketLock;
    atomic<bool> otherValueLock;

    SEIDForSocketStruct()
    {
        SEID.clear();
        ServerSocket = INVALID_SOCKET;
        socketH = INVALID_SOCKET;
        isSEIDExit = false;
        isSocketExit = false;
        isBack = false;
        isUse = false;
        serverSocketLock.exchange(false, std::memory_order_relaxed);
        serverHealthySocketLock.exchange(false, std::memory_order_relaxed);
        otherValueLock.exchange(false, std::memory_order_relaxed);
    }
    void getServerSocketLock()
    {
        while (serverSocketLock.exchange(true, std::memory_order_acquire))
            ;
    }
    void releaseServerSocketLock()
    {
        serverSocketLock.exchange(false, std::memory_order_release);
    }
    void getServerHealthySocketLock()
    {
        while (serverHealthySocketLock.exchange(true, std::memory_order_acquire))
            ;
    }
    void releaseServerHealthySocketLock()
    {
        serverHealthySocketLock.exchange(false, std::memory_order_release);
    }
    void getOtherValueLock()
    {
        while (otherValueLock.exchange(true, std::memory_order_acquire))
            ;
    }
    void releaseOtherValueLock()
    {
        otherValueLock.exchange(false, std::memory_order_release);
    }
};
struct HealthyDataStruct
{
    string SEID;
    bool isServer;
};

map<string, int> StringToIntInComd = {
    {"run", 1},
    {"show", 2},
    {"del", 3},
    {"cmod", 4},

    {"port", 5},
    {"wanip", 6},
    {"lanip", 7},
    {"all", 8}};
atomic<bool> HealthyLock(false);
queue<HealthyDataStruct> HealthyQueue;
int ServerPort;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
WSADATA wsaData;
#endif
SOCKET ListenSocket;
sockaddr_in service;
queue<SOCKET> ClientSocketQueue, ServerSocketQueue;
map<string, SEIDForSocketStruct> ServerSEIDMap, ClientSEIDMap;
vector<ClientSocketFlagStruct> DataSaveArry;
vector<ClientSocketFlagStruct> ClientMap;
string password;
bool dataIsChange = false;
vector<thread> ServerRSThreadArry, ClientRSThreadArry;
atomic<bool> ClientMapLock(false);

mutex ServerQueueLock, ClientQueueLock;
condition_variable Queuecv;

bool ischange = false;
int PassDataInit = 0;
int initServer(SOCKET &, sockaddr_in &, int);
string StringTime(time_t);
void Connect(string, vector<string>, int);
int delForId(int);
void del(string, vector<string>, int);
void show(string, vector<string>, int);
void cmod(string, vector<string>, int);
string createSEID(SOCKET, string);
void joinClient(string, string, string, unsigned long long int, unsigned long long int, string);
void ServerRS(SOCKET);
void ClientRS(SOCKET);
void ServerConnect();
void ClientConnect();
void saveData();
void dataSave();
void passData();
void loadData();
bool send_message(SOCKET sock, const std::string &message);
bool receive_message(SOCKET sock, std::string &message);
int main(int, char **);

#endif
