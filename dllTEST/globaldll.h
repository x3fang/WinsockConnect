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
#include "../MD5.h"
#include "../getCmd.h"
#include "../log.h"
#include "fliterDLL.h"
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

logNameSpace::Log prlog("");
bool send_message(SOCKET sock, const std::string &message);
bool receive_message(SOCKET sock, std::string &message);

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
bool EXPORT send_message(SOCKET sock, const std::string &message)
{
    std::ostringstream oss;
    oss << message.size() << "\r\n\r\n\r\n\r\n\r\n"
        << message; // 构建消息，包含长度和实际数据
    std::string formatted_message = oss.str();

    int total_sent = 0;
    int message_length = formatted_message.size();
    const char *data = formatted_message.c_str();

    while (total_sent < message_length)
    {
        int bytes_sent = send(sock, data + total_sent, message_length - total_sent, 0);
        if (bytes_sent == SOCKET_ERROR)
        {
            return false; // 发送失败
        }
        total_sent += bytes_sent;
    }
    return true; // 发送成功
}
bool EXPORT receive_message(SOCKET sock, std::string &message)
{
    std::string length_str;
    char buffer[16384] = {0};
    int received;

    // 首先读取长度部分，直到接收到 \r\n
    while (true)
    {
        received = recv(sock, buffer, 1, 0); // 每次读取一个字节
        if (received <= 0)
        {
            return false; // 连接断开或读取出错
        }
        if (buffer[0] == '\r')
        {
            // 继续读取\n
            received = recv(sock, buffer, 1, 0);
            if (received <= 0 || buffer[0] != '\n')
            {
                return false; // 格式错误
            }

            for (int i = 1; i <= 4; i++)
            {
                received = recv(sock, buffer, 1, 0);
                if (received <= 0 || buffer[0] != '\r')
                {
                    return false; // 格式错误
                }
                received = recv(sock, buffer, 1, 0);
                if (received <= 0 || buffer[0] != '\n')
                {
                    return false; // 格式错误
                }
            }
            break; // 读取到 \r\n，退出循环
        }
        length_str += buffer[0];
    }

    int data_length = std::stoi(length_str); // 转换长度字符串为整数
    message.resize(data_length);

    int total_received = 0;
    while (total_received < data_length)
    {
        received = recv(sock, &message[total_received], data_length - total_received, 0);
        if (received <= 0)
        {
            return false; // 连接断开或读取出错
        }
        total_received += received;
    }
    return true; // 接收成功
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
