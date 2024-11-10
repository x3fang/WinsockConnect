#ifndef _DEFINEHEAD_H_
#define _DEFINEHEAD_H_
#pragma once
#include <bitset>
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <WinSock2.h>
#include <algorithm>
#include "../log.h"
#define EXPORT __declspec(dllexport)
#define RUN_LINE_NUM 15
using std::atomic;
using std::bitset;
using std::map;
using std::mutex;
using std::queue;
using std::string;
using std::vector;
logNameSpace::Log prlog("");
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
struct EXPORT allInfoStruct
{
    logNameSpace::Log *prlog_;
    string SEID;
    string msg;
    vector<string> msgVector;
    queue<SOCKET> *ClientSocketQueue, *ServerSocketQueue;
    map<string, SEIDForSocketStruct> *ServerSEIDMap, *ClientSEIDMap;
    vector<ClientSocketFlagStruct> *ClientMap;
    mutex *ServerQueueLock, *ClientQueueLock;
    SOCKET NowSocket;
    allInfoStruct(string seid, SOCKET Nsocket, string msg = "")
    {
        SEID = seid;
        this->ClientSocketQueue = ClientSocketQueue;
        this->ServerSocketQueue = ServerSocketQueue;
        this->ServerSEIDMap = ServerSEIDMap;
        this->ClientSEIDMap = ClientSEIDMap;
        this->ClientMap = ClientMap;
        this->ServerQueueLock = ServerQueueLock;
        this->ClientQueueLock = ClientQueueLock;
        this->NowSocket = Nsocket;
        this->msg = msg;
        this->prlog_ = &prlog;
    }
    allInfoStruct(string seid, SOCKET Nsocket, vector<string> msgVector)
    {
        SEID = seid;
        this->ClientSocketQueue = ClientSocketQueue;
        this->ServerSocketQueue = ServerSocketQueue;
        this->ServerSEIDMap = ServerSEIDMap;
        this->ClientSEIDMap = ClientSEIDMap;
        this->ClientMap = ClientMap;
        this->ServerQueueLock = ServerQueueLock;
        this->ClientQueueLock = ClientQueueLock;
        this->NowSocket = Nsocket;
        this->msgVector = msgVector;
    }
    ~allInfoStruct()
    {
        this->ServerQueueLock = nullptr;
        this->ClientQueueLock = nullptr;
        this->ClientSocketQueue = nullptr;
        this->ServerSocketQueue = nullptr;
        this->ServerSEIDMap = nullptr;
        this->ClientSEIDMap = nullptr;
        this->ClientMap = nullptr;
    }
};
typedef int (*start_ptr)(void);
// isStart: 默认开启状态
typedef bool (*RegisterPlugin_ptr)(string pluginName,
                                   string runlineS,
                                   void (*startupfunPtr)(),
                                   void (*startfunPtr)(),
                                   void (*stopfunPtr)(),
                                   bool (*runfunPtr)(allInfoStruct &),
                                   bool isStart);
typedef bool (*DelPlugin_ptr)(string pluginName);
typedef bool (*FindPlugin_ptr)(string pluginName);
typedef bool (*RestPlugin_ptr)(string pluginName,
                               string runlineS,
                               void (*startupfunPtr)(),
                               void (*startfunPtr)(),
                               void (*stopfunPtr)(),
                               bool (*runfunPtr)(allInfoStruct &),
                               bool isStart);
typedef bool (*StartPlugin_ptr)(string pluginName);
typedef bool (*StopPlugin_ptr)(string pluginName);
typedef void (*RunPlugin_ptr)(allInfoStruct &, string);
typedef bool (*runFun_ptr)(allInfoStruct &);
#endif // _DEFINEHEAD_H_