#include "definehead.h"
allInfoStruct::~allInfoStruct()
{
      this->msgVector = nullptr;
      this->ServerQueueLock = nullptr;
      this->ClientQueueLock = nullptr;
      this->ClientSocketQueue = nullptr;
      this->ServerSocketQueue = nullptr;
      this->ServerSEIDMap = nullptr;
      this->ClientSEIDMap = nullptr;
      this->ClientMap = nullptr;
      this->ClientMapLock = nullptr;
      this->pluginList = nullptr;
      this->pluginNameList = nullptr;
      this->funPluginNameList = nullptr;
      this->funPluginComdVerNameList = nullptr;
      this->VpluginFunList = nullptr;
}
allInfoStruct::allInfoStruct(string seid,
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
                             vector<void *> *VpluginFunList)
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
      this->prlog_ = &prlog;
      this->ClientMapLock = ClientMapLock;
      this->pluginList = pluginList;
      this->pluginNameList = pluginNameList;
      this->funPluginNameList = funPluginNameList;
      this->funPluginComdVerNameList = funPluginComdVerNameList;
      this->VpluginFunList = VpluginFunList;
}
allInfoStruct::allInfoStruct(string seid,
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
                             vector<void *> *VpluginFunList)
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
      this->ClientMapLock = ClientMapLock;
      this->pluginList = pluginList;
      this->pluginNameList = pluginNameList;
      this->funPluginNameList = funPluginNameList;
      this->funPluginComdVerNameList = funPluginComdVerNameList;
      this->VpluginFunList = VpluginFunList;
}

SEIDForSocketStruct::SEIDForSocketStruct()
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
void SEIDForSocketStruct::getServerSocketLock()
{
      while (serverSocketLock.exchange(true, std::memory_order_acquire))
            ;
}
void SEIDForSocketStruct::releaseServerSocketLock()
{
      serverSocketLock.exchange(false, std::memory_order_release);
}
void SEIDForSocketStruct::getServerHealthySocketLock()
{
      while (serverHealthySocketLock.exchange(true, std::memory_order_acquire))
            ;
}
void SEIDForSocketStruct::releaseServerHealthySocketLock()
{
      serverHealthySocketLock.exchange(false, std::memory_order_release);
}
void SEIDForSocketStruct::getOtherValueLock()
{
      while (otherValueLock.exchange(true, std::memory_order_acquire))
            ;
}
void SEIDForSocketStruct::releaseOtherValueLock()
{
      otherValueLock.exchange(false, std::memory_order_release);
}
bool SEIDForSocketStruct::operator==(const string &e)
{
      return (this->SEID == e);
}
bool ClientSocketFlagStruct::operator==(const ClientSocketFlagStruct &e)
{
      return (this->ClientLanIp == e.ClientLanIp &&
              this->ClientWanIp == e.ClientWanIp);
}
bool ClientSocketFlagStruct::operator()(const ClientSocketFlagStruct &e, const string &e2) const
{
      return (e2 == e.SEID);
}
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