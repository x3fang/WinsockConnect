#include "client.h"
int showForSend(allInfoStruct *info, string seid, filter f, bool startIf, ClientSocketFlagStruct::states state)
{
      int status = 0;
      auto *ServerSEIDMap = (*info).ServerSEIDMap;
      auto *ClientMap = (*info).ClientMap;
      SOCKET *s = &(*ServerSEIDMap)[seid].ServerSocket;
      auto *ClientMapLock = ((*info).ClientMapLock);
      int canShowClient = 0;
      while ((*ClientMapLock).exchange(true, std::memory_order_acquire))
            ; // 加锁
      for (int i = 1; i <= (*ClientMap).size(); i++)
      {
            if ((*ServerSEIDMap)[seid].isBack)
            {
                  status = -1;
                  break;
            }
            if (!startIf ||
                (*ClientMap)[i - 1].state == state ||
                f.matching((*ClientMap)[i - 1].ClientWanIp,
                           (*ClientMap)[i - 1].ClientLanIp,
                           to_string((*ClientMap)[i - 1].ClientConnectPort)))
            {
                  canShowClient++;
                  string sendBuf = (*ClientMap)[i - 1].ClientWanIp +
                                   " " + (*ClientMap)[i - 1].ClientLanIp +
                                   " " + to_string((*ClientMap)[i - 1].ClientConnectPort) +
                                   " " + to_string((*ClientMap)[i - 1].state);
                  if (!send_message(*s, sendBuf))
                  {
                        status = -2;
                        break;
                  }
            }
      }
      if (!send_message(*s, "\r\n\r\nend\r\n\r\n"))
      {
            status = -3;
      }
      (*ClientMapLock).exchange(false, std::memory_order_release);
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
            int repetitions = std::count(ClientMap.begin(), ClientMap.end(), temp);
            temp.ClientWanIp += "(" + to_string(repetitions) + ")";
            temp.ClientLanIp += "(" + to_string(repetitions) + ")";
            ClientMap.push_back(temp);
      }
      else
      {
            ClientMap.push_back(temp);
      }
      ClientMapLock.exchange(false, std::memory_order_release);
      return;
}
void delForId_wait(SEIDForSocketStruct *ClientInfo_CSM, int ClientId, bool *issec)
{
      std::lock_guard<std::mutex> lock((*ClientInfo_CSM).OtherValueLock);
      std::lock_guard<std::mutex> lockS((*ClientInfo_CSM).ServerSocketLock);
      std::lock_guard<std::mutex> lockH((*ClientInfo_CSM).ServerHealthySocketLock);
      (*ClientInfo_CSM).isBack = false;
      (*ClientInfo_CSM).isSocketExit = true;
      send_message((*ClientInfo_CSM).socketH, "del");
      setClientExit(*ClientInfo_CSM);
      (*issec) = true;
      return;
}
extern "C" int EXPORT delForId(allInfoStruct *info, int ClientId, bool iswait, std::pair<bool, std::shared_ptr<thread>> *wait)
{
      while ((*(*info).ClientMapLock).exchange(true, std::memory_order_acquire))
            ; // 加锁
      SEIDForSocketStruct *ClientInfo_CSM = &((*(*info).ClientSEIDMap)[(*(*info).ClientMap)[ClientId - 1].SEID]);
      auto *ClientMap = &(*(*info).ClientMap);
      SOCKET *ClientSocket = &(*ClientInfo_CSM).ServerSocket;
      if ((*ClientSocket) != INVALID_SOCKET)
      {
            if ((*ClientMap)[ClientId - 1].state != ClientSocketFlagStruct::Use)
            {
                  std::lock_guard<std::mutex> lock((*ClientInfo_CSM).OtherValueLock);
                  std::lock_guard<std::mutex> lockS((*ClientInfo_CSM).ServerSocketLock);
                  std::lock_guard<std::mutex> lockH((*ClientInfo_CSM).ServerHealthySocketLock);
                  send_message((*ClientInfo_CSM).socketH, "del");
                  (*ClientInfo_CSM).isBack = false;
                  (*ClientInfo_CSM).isSocketExit = true;
                  (*(*info).ClientMapLock).exchange(false, std::memory_order_release);
                  return 0;
            }
            else if ((*ClientMap)[ClientId - 1].state == ClientSocketFlagStruct::Use)
            {
                  if (iswait)
                  {
                        std::shared_ptr<thread> t(new thread(delForId_wait, ClientInfo_CSM, ClientId, &(*wait).first));
                        (*wait).second = t;
                  }
                  (*(*info).ClientMapLock).exchange(false, std::memory_order_release);
                  return 2;
            }
      }
      (*(*info).ClientMapLock).exchange(false, std::memory_order_release);
      return -1;
}
void setServerExit(SEIDForSocketStruct &s)
{
      s.isBack = true;
      s.isSocketExit = true;
      ClientSEIDMap.erase(s.SEID);
      // 设置退出标志
      {
            std::lock_guard<std::mutex> lock(s.OtherValueLock);
      }
      // 关闭心跳套接字
      {
            std::lock_guard<std::mutex> lock(s.ServerHealthySocketLock);
            closesocket(s.socketH);
      }
      // 关闭连接套接字
      {
            std::lock_guard<std::mutex> lock(s.ServerSocketLock);
            closesocket(s.ServerSocket);
      }
      return;
}
void setClientExit(SEIDForSocketStruct &s)
{
      s.isBack = true;
      s.isSocketExit = true;
      ClientMap.erase(find_if(ClientMap.begin(),
                              ClientMap.end(),
                              std::bind(ClientSocketFlagStruct(), std::placeholders::_1, s.SEID)));
      ClientSEIDMap.erase(s.SEID);
      // 关闭心跳套接字
      closesocket(s.socketH);
      // 关闭连接套接字
      closesocket(s.ServerSocket);
      return;
}