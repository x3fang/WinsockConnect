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
#include "../../log.h"

#define STOP_HEALTH_CHECK 0

#pragma comment(lib, "ws2_32.lib")

#define ClientPluginClassNum 5
#define EXPORT __declspec(dllexport)
#define RUN_LINE_NUM 17
using std::atomic;
using std::bitset;
using std::map;
using std::mutex;
using std::pair;
using std::queue;
using std::string;
using std::vector;
logNameSpace::Log prlog("");
std::regex only_number("\\d+");
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
            return (this->ClientLanIp == e.ClientLanIp &&
                    this->ClientWanIp == e.ClientWanIp);
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
      allInfoStruct(string seid,
                    SOCKET Nsocket,
                    string msg,
                    queue<SOCKET> *ClientSocketQueue,
                    queue<SOCKET> *ServerSocketQueue,
                    map<string, SEIDForSocketStruct> *ServerSEIDMap,
                    map<string, SEIDForSocketStruct> *ClientSEIDMap,
                    vector<ClientSocketFlagStruct> *ClientMap,
                    mutex *ServerQueueLock,
                    mutex *ClientQueueLock)
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

      allInfoStruct(string seid,
                    SOCKET Nsocket,
                    vector<string> msgVector,
                    queue<SOCKET> *ClientSocketQueue,
                    queue<SOCKET> *ServerSocketQueue,
                    map<string, SEIDForSocketStruct> *ServerSEIDMap,
                    map<string, SEIDForSocketStruct> *ClientSEIDMap,
                    vector<ClientSocketFlagStruct> *ClientMap,
                    mutex *ServerQueueLock,
                    mutex *ClientQueueLock)
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
typedef bool (*RegisterPluginFun_ptr)(string pluginName,
                                      string runlineS,
                                      void (*startupfunPtr)(),
                                      void (*startfunPtr)(),
                                      void (*stopfunPtr)(),
                                      bool (*runfunPtr)(allInfoStruct *),
                                      bool isStart);
typedef bool (*DelPluginFun_ptr)(string pluginName);
typedef bool (*FindPluginFun_ptr)(string pluginName);
typedef bool (*RestPluginFun_ptr)(string pluginName,
                                  string runlineS,
                                  void (*startupfunPtr)(),
                                  void (*startfunPtr)(),
                                  void (*stopfunPtr)(),
                                  bool (*runfunPtr)(allInfoStruct *),
                                  bool isStart);
typedef bool (*StartPluginFun_ptr)(string pluginName);
typedef bool (*StopPluginFun_ptr)(string pluginName);
typedef void (*RunPluginFun_ptr)(allInfoStruct &, string);
typedef void (*RunFun_ptr)(allInfoStruct &, string);

typedef void (*startupFun_ptr)(void);
typedef void (*startFun_ptr)(void);
typedef void (*stopFun_ptr)(void);
typedef bool (*runFun_ptr)(allInfoStruct *);

void setClientState(ClientSocketFlagStruct *ClientSocketFlagStruct, ClientSocketFlagStruct::states state)
{
      ClientSocketFlagStruct->state = state;
      return;
}
void getFilesName(string path, vector<string> &files)
{
      // 文件句柄
      intptr_t hFile = 0;
      // 文件信息
      struct _finddata_t fileinfo;
      string p;
      if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
      {
            do
            {
                  // 如果是目录,迭代之
                  // 如果不是,加入列表
                  if ((fileinfo.attrib & _A_SUBDIR))
                  {
                        if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                              getFilesName(p.assign(path).append("\\").append(fileinfo.name), files);
                  }
                  else
                  {
                        files.push_back(path + "\\" + fileinfo.name);
                  }
            } while (_findnext(hFile, &fileinfo) == 0);
            _findclose(hFile);
      }
}
bool send_message(SOCKET &sock, const std::string &message)
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
bool receive_message(SOCKET &sock, std::string &message)
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
#endif // _DEFINEHEAD_H_