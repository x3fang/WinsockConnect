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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <utility>
#include <sstream>
#include <regex>
#include <functional>
#include "log.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <stdio.h>
#include <iostream>
#include <random>
#include <fstream>
#include <thread>
#include <time.h>
#include <condition_variable>
#include "MD5.h"
#include "fliterDLL.h"
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

#define STOP_HEALTH_CHECK 0
#pragma comment(lib, "ws2_32.lib")
#define ClientPluginClassNum 5
#define EXPORT __declspec(dllexport)
using std::atomic;
using std::bitset;
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
using std::pair;
using std::queue;
using std::string;
using std::thread;
using std::to_string;
using std::vector;
logNameSpace::Log prlog("");
std::regex only_number("\\d+");
#define lns logNameSpace
#define runPluginValue &ClientSocketQueue, &ServerSocketQueue, &ServerSEIDMap, &ClientSEIDMap, &ClientMap, &ServerQueueLock, &ClientQueueLock, &ClientMapLock, &pluginList, &pluginNameList, &funPluginNameList, &funPluginComdVerNameList
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
      bool operator==(const ClientSocketFlagStruct &e);
      bool operator==(const string &e);
      bool operator()(const ClientSocketFlagStruct &e, const string &e2) const;
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

      SEIDForSocketStruct();
      void getServerSocketLock();
      void releaseServerSocketLock();
      void getServerHealthySocketLock();
      void releaseServerHealthySocketLock();
      void getOtherValueLock();
      void releaseOtherValueLock();
      bool operator==(const string &e);
};

struct HealthyDataStruct
{
      string SEID;
      bool isServer;
};
struct EXPORT pluginStruct;
struct EXPORT pluginListStruct;
struct funPluginComdVerNameStruct;
struct EXPORT allInfoStruct
{
      logNameSpace::Log *prlog_;
      string SEID;
      string msg;
      vector<string> *msgVector;
      queue<SOCKET> *ClientSocketQueue, *ServerSocketQueue;
      map<string, SEIDForSocketStruct> *ServerSEIDMap, *ClientSEIDMap;
      vector<ClientSocketFlagStruct> *ClientMap;

      vector<std::shared_ptr<pluginListStruct>> *pluginList;
      vector<string> *pluginNameList;
      vector<string> *funPluginNameList;
      vector<funPluginComdVerNameStruct> *funPluginComdVerNameList;

      mutex *ServerQueueLock, *ClientQueueLock;
      atomic<bool> *ClientMapLock;
      SOCKET NowSocket;

      vector<void *> *VpluginFunList;
      allInfoStruct(string seid,
                    SOCKET Nsocket,
                    string msg,
                    queue<SOCKET> *ClientSocketQueue,
                    queue<SOCKET> *ServerSocketQueue,
                    map<string, SEIDForSocketStruct> *ServerSEIDMap,
                    map<string, SEIDForSocketStruct> *ClientSEIDMap,
                    vector<ClientSocketFlagStruct> *ClientMap,
                    mutex *ServerQueueLock,
                    mutex *ClientQueueLock,
                    atomic<bool> *ClientMapLock,
                    vector<std::shared_ptr<pluginListStruct>> *pluginList,
                    vector<string> *pluginNameList,
                    vector<string> *funPluginNameList,
                    vector<funPluginComdVerNameStruct> *funPluginComdVerNameList,
                    vector<void *> *VpluginFunList = nullptr);

      allInfoStruct(string seid,
                    SOCKET Nsocket,
                    vector<string> *msgVector,
                    queue<SOCKET> *ClientSocketQueue,
                    queue<SOCKET> *ServerSocketQueue,
                    map<string, SEIDForSocketStruct> *ServerSEIDMap,
                    map<string, SEIDForSocketStruct> *ClientSEIDMap,
                    vector<ClientSocketFlagStruct> *ClientMap,
                    mutex *ServerQueueLock,
                    mutex *ClientQueueLock,
                    atomic<bool> *ClientMapLock,
                    vector<std::shared_ptr<pluginListStruct>> *pluginList,
                    vector<string> *pluginNameList,
                    vector<string> *funPluginNameList,
                    vector<funPluginComdVerNameStruct> *funPluginComdVerNameList,
                    vector<void *> *VpluginFunList = nullptr);
      ~allInfoStruct();
};

#ifndef UNLOAD_PLUGIN
typedef int (*start_ptr)(void);
typedef int (*stop_ptr)(void);
typedef void (*set_isServerHealthyCheckClose_ptr)(bool);
typedef void (*set_isClientHealthyCheckClose_ptr)(bool);

// isStart: 默认开启状态
typedef bool (*RegisterPluginFun_ptr)(string pluginName,
                                      string runlineS,
                                      void (*startupfunPtr)(),
                                      void (*startfunPtr)(),
                                      void (*stopfunPtr)(),
                                      bool (*runfunPtr)(allInfoStruct *),
                                      bool isStart,
                                      map<string, string> (*cmdInfoGet)());
typedef bool (*DelPluginFun_ptr)(string pluginName);
typedef bool (*FindPluginFun_ptr)(string pluginName);
typedef bool (*RestPluginFun_ptr)(string pluginName,
                                  string runlineS,
                                  void (*startupfunPtr)(),
                                  void (*startfunPtr)(),
                                  void (*stopfunPtr)(),
                                  bool (*runfunPtr)(allInfoStruct *),
                                  bool isStart,
                                  map<string, string> (*cmdInfoGet)());
typedef bool (*StartPluginFun_ptr)(string pluginName);
typedef bool (*StopPluginFun_ptr)(string pluginName);
typedef void (*RunPluginFun_ptr)(allInfoStruct &, string);
typedef void (*RunFun_ptr)(allInfoStruct &, string);
typedef void (*startupFun_ptr)(void);
typedef void (*startFun_ptr)(void);
typedef void (*stopFun_ptr)(void);
typedef bool (*runFun_ptr)(allInfoStruct *);
#endif

#include "definehead.cpp"

#ifndef UNLOADSC_HEAD
#include "server.h"
#include "client.h"
#endif

#ifndef UNLOAD_PLUGIN
#include "plugin.h"
#endif

bool send_message(SOCKET &sock, const std::string &message);
bool receive_message(SOCKET &sock, std::string &message);
extern "C" void EXPORT sendError(logNameSpace::funLog &funlog, string seid, string nextDo);
extern "C" void EXPORT recvError(logNameSpace::funLog &funlog, string seid, string nextDo);
#endif // _DEFINEHEAD_H_