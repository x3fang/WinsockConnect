#ifndef GLOBAL_DLL_H_
#define GLOBAL_DLL_H_

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
#include <bitset>
#include <stdio.h>
#include <iostream>
#include <random>
#include <fstream>
#include <thread>
#include <time.h>
#include <atomic>
#include <algorithm>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../MD5.h"
#include "../getCmd.h"
#include "fliterDLL.h"
#include "definehead.h"
#define EXPORT __declspec(dllexport)
// #define _DEBUG

#define lns logNameSpace
using std::atomic;
using std::cerr;
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

map<string, int>
    StringToIntInComd = {
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

extern "C" int EXPORT delForId(int ClientId)
{
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    SOCKET &ClientSocket = ClientSEIDMap[ClientMap[ClientId - 1].SEID].ServerSocket;
    auto funlog = prlog.getFunLog("delForId");
    if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state != ClientSocketFlagStruct::Use)
    {
        if (send_message(ClientSocket, "del"))
        {
            *funlog << "del ok" << lns::endl;
            *funlog << "delForId End" << lns::endl;
            ClientMapLock.exchange(false, std::memory_order_release);
            return 0;
        }
        else
        {
            *funlog << "del error" << lns::endl;
            *funlog << "delForId End" << lns::endl;
            ClientMapLock.exchange(false, std::memory_order_release);
            return 1;
        }
    }
    else if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state == ClientSocketFlagStruct::Use)
    {
        *funlog << "Is Use,can`t del" << lns::endl;
        *funlog << "delForId End" << lns::endl;
        ClientMapLock.exchange(false, std::memory_order_release);
        return 2;
    }
    *funlog << "unknown error" << lns::endl;
    *funlog << "delForId End" << lns::endl;
    ClientMapLock.exchange(false, std::memory_order_release);
    return -1;
}
extern "C" void EXPORT sendError(logNameSpace::funLog &funlog, string seid, string nextDo)
{
    funlog << "send error.SEID:"
           << seid
           << " error code:" << WSAGetLastError()
           << " next do: " << nextDo
           << lns::endl;
    return;
}
extern "C" void EXPORT recvError(logNameSpace::funLog &funlog, string seid, string nextDo)
{
    funlog << "recv error.SEID:"
           << seid
           << " error code:" << WSAGetLastError()
           << " next do: " << nextDo
           << lns::endl;
    return;
}
extern "C" int EXPORT showForSend(string seid, filter f, bool startIf = false, ClientSocketFlagStruct::states state = ClientSocketFlagStruct::NULLs)
{

    int status = 0;
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    int canShowClient = 0;
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    for (int i = 1; i <= ClientMap.size(); i++)
    {
        if (ServerSEIDMap[seid].isBack)
        {
            ClientMapLock.exchange(false, std::memory_order_release);
            status = -1;
            break;
        }
        if (!startIf || ClientMap[i - 1].state == state || f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)))
        {
            canShowClient++;
            string sendBuf = ClientMap[i - 1].ClientWanIp + " " + ClientMap[i - 1].ClientLanIp + " " + to_string(ClientMap[i - 1].ClientConnectPort) + " " + to_string(ClientMap[i - 1].state);
            if (!send_message(s, sendBuf))
            {
                ClientMapLock.exchange(false, std::memory_order_release);
                status = -2;
                break;
            }
        }
    }
    if (!send_message(s, "\r\n\r\nend\r\n\r\n"))
    {
        ClientMapLock.exchange(false, std::memory_order_release);
        status = -3;
    }
    ClientMapLock.exchange(false, std::memory_order_release);
    status = canShowClient;
    return status;
}
void EXPORT joinClient(string ClientWanIp, string ClientLanIp, string ClientPort, unsigned long long int OnlineTime, unsigned long long int OfflineTime, string SEID)
{
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    ClientSocketFlagStruct temp;
    temp.ClientWanIp = ClientWanIp;
    temp.ClientLanIp = ClientLanIp;
    temp.ClientConnectPort = atoi(ClientPort.c_str());
    temp.OnlineTime = OnlineTime;
    temp.OfflineTime = OfflineTime;
    temp.state = ClientSocketFlagStruct::states::Online;
    temp.SEID = SEID;
    vector<ClientSocketFlagStruct>::iterator itr = find(ClientMap.begin(), ClientMap.end(), temp);
    if (itr != ClientMap.end())
    {
        while (ClientMap[distance(ClientMap.begin(), itr)].state != ClientSocketFlagStruct::states::Online)
            ;
        closesocket(ClientSEIDMap[ClientMap[distance(ClientMap.begin(), itr)].SEID].ServerSocket);
        ClientMap[distance(ClientMap.begin(), itr)] = temp;
    }
    else
    {
        ClientMap.push_back(temp);
    }
    ClientMapLock.exchange(false, std::memory_order_release);
    return;
}
#endif
