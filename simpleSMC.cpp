// #include "saveDatadll.h"
#include "plugin.h"
extern "C" bool EXPORT Connect(allInfoStruct *info)
{
      auto *ServerSEIDMap = (*info).ServerSEIDMap;
      auto *ClientMap = (*info).ClientMap;
      auto *ClientSEIDMap = (*info).ClientSEIDMap;
      string seid = (*info).SEID;
      std::shared_ptr<logNameSpace::funLog> funlog;
      try
      {
            funlog = ((*info).prlog_)->getFunLog("Connect");
      }
      catch (const std::exception &e)
      {
            cout << e.what() << endl;
            return false;
      }
      (*(*info).prlog_) << "OK" << lns::endl;
      *funlog << "OK" << lns::endl;
      SOCKET &s = (*ServerSEIDMap)[seid].ServerSocket;
      string recvBuf;
      std::lock_guard<std::mutex>
          lockSEID((*ServerSEIDMap)[seid].ServerSocketLock);

      while (1)
      {
            recvBuf.clear();
            {
                  std::lock_guard<std::mutex>
                      lockSEID((*ServerSEIDMap)[seid].OtherValueLock);
                  if ((*ServerSEIDMap)[seid].isBack)
                  {
                        *funlog << "Server is exit" << lns::endl;
                        break;
                  }
            }

            filter temp;
            try
            {
                  if (showForSend(info,
                                  seid,
                                  temp,
                                  true,
                                  ClientSocketFlagStruct::Online) < 0)
                  {
                        *funlog << "error code:" << WSAGetLastError()
                                << lns::endl
                                << "error line:"
                                << __LINE__
                                << lns::endl;
                        break;
                  }
            }
            catch (const std::exception &e)
            {
                  *funlog << "Server recv error "
                          << "error code:" << e.what() << lns::endl
                          << "error line:" << __LINE__ << lns::endl;
                  break;
            }
            bool state = false;
            state = receive_message(s, recvBuf);
            if (!state)
            {
                  *funlog << "Server recv error "
                          << "error code:" << WSAGetLastError()
                          << " error line:" << __LINE__ << lns::endl
                          << " error file path:" << __FILE__ << lns::endl;
                  break;
            }
            else if (state > 0)
            {
                  if (recvBuf == "\r\nexit\r\n")
                  {
                        *funlog << "Server exit Connect " << lns::endl;
                        break;
                  }
                  else if (recvBuf == "\r\nnext\r\n")
                  {
                        *funlog << "show Client List" << lns::endl;
                        continue;
                  }
                  int setClientId = atoi(recvBuf.c_str());
                  *funlog << "Server choose ID:" << setClientId << lns::endl;
                  int ClientIndex = -1;
                  for (int i = 0; i < (*ClientMap).size(); i++)
                  {
                        if ((*ClientMap)[i].state ==
                            ClientSocketFlagStruct::Online)
                        {
                              setClientId--;
                              if (setClientId == 0)
                              {
                                    setClientState(&(*ClientMap)[i], ClientSocketFlagStruct::Use);
                                    ClientIndex = i;
                                    break;
                              }
                        }
                  }

                  if (ClientIndex != -1)
                  {
                        ClientSocketFlagStruct *ClientInfo_CM =
                            &((*ClientMap)[ClientIndex]); // ClientInfo From ClientMap
                        SEIDForSocketStruct *ClientInfo_CSM =
                            &((*ClientSEIDMap)[ClientInfo_CM->SEID]); // ClientInfo From ClientSEIDMap
                        SOCKET *ClientSocket = &((*ClientInfo_CSM).ServerSocket);
                        string *ClientSEID = &((*ClientInfo_CM).SEID);

                        string buf, buf2, temp;
                        {
                              *funlog << "ClientIndex in ClientMap:" << ClientIndex << lns::endl;

                              *funlog << "Client SEID:"
                                      << (*ClientSEID) << lns::endl;

                              *funlog << "Server start to connect Client"
                                      << lns::endl;

                              std::lock_guard<std::mutex>
                                  lockSEIDForClient((*ClientInfo_CSM).ServerSocketLock);

                              std::lock_guard<std::mutex>
                                  lockOtherValueForClient((*ClientInfo_CSM).OtherValueLock);
                              if (!send_message(s, "\r\n\r\nsec\r\n\r\n"))
                              {
                                    *funlog << "send to Server error "
                                            << "error code:" << WSAGetLastError() << lns::endl
                                            << "error line:" << __LINE__ << lns::endl;
                                    setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                    break;
                              }
                              {
                                    bool failButNext = false;
                                    if (!send_message(
                                            (*ClientSocket),
                                            "\r\nstart\r\n"))
                                    {
                                          sendError(*funlog, (*ClientSEID),
                                                    "break.For Client. Error line:" + __LINE__);
                                          setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                          if (!send_message(s, "\r\n\r\nfail\r\n\r\n"))
                                          {
                                                sendError(*funlog, seid, "break");
                                          }
                                          break;
                                    }
                                    if (!receive_message((*ClientInfo_CSM)
                                                             .ServerSocket,
                                                         buf2))
                                    {
                                          recvError(*funlog, (*ClientSEID),
                                                    "next.For Client. Error line:" + __LINE__);
                                          failButNext = true;
                                    }
                                    buf2 += temp;
                                    *funlog << buf2 << lns::endl;
                                    *funlog << "failButNext:" << failButNext << lns::endl;
                                    if (failButNext)
                                    {
                                          if (!send_message(s, "\r\n\r\nfailn\r\n\r\n"))
                                          {
                                                sendError(*funlog, (*ClientSEID),
                                                          "break.For Server. Error line:" + __LINE__);

                                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                                send_message((*ClientInfo_CSM)
                                                                 .ServerSocket,
                                                             "\r\nexit\r\n");
                                                setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                                break;
                                          }
                                    }
                                    if (!send_message(s, buf2))
                                    {
                                          *funlog << "send to Server error "
                                                  << "error code:" << WSAGetLastError() << lns::endl
                                                  << "error line:" << __LINE__ << lns::endl;
                                          sendError(*funlog, (*ClientSEID),
                                                    ("break.For Server. Error line:" + __LINE__));
                                          send_message((*ClientInfo_CSM)
                                                           .ServerSocket,
                                                       "\r\nexit\r\n");
                                          setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                          send_message(s, "\r\n\r\nfail\r\n\r\n");
                                          break;
                                    }
                                    *funlog << "Server connect init ok" << lns::endl;
                              }
                        }
                        while (1)
                        {
                              if ((*ServerSEIDMap)[seid].isBack)
                              {
                                    *funlog << "Server is exit" << lns::endl;
                                    send_message((*ClientInfo_CSM)
                                                     .ServerSocket,
                                                 "\r\nexit\r\n");
                                    setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                    break;
                              }
                              buf.clear();
                              buf2.clear();
                              bool state = false;
                              {
                                    state = receive_message(s, buf);
                              }
                              if (!state)
                              {
                                    recvError(*funlog, (*ClientSEID), "break.For server. Error line:" + __LINE__);
                                    send_message((*ClientInfo_CSM)
                                                     .ServerSocket,
                                                 "\r\nexit\r\n\r\n");
                                    setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                    break;
                              }
                              else if (state > 0)
                              {
                                    std::lock_guard<std::mutex> lockSEIDForClient((*ClientInfo_CSM).ServerSocketLock);
                                    std::lock_guard<std::mutex> lockOtherValueForClient((*ClientInfo_CSM).OtherValueLock);
                                    if (buf == "\r\nfexit\r\n")
                                    {
                                          if (!send_message((*ClientSocket), "\r\nexit\r\n") ||
                                              !receive_message((*ClientSocket), buf2))
                                          {
                                                sendError(*funlog, (*ClientSEID), "break.For Client. Error line:" + __LINE__);
                                                recvError(*funlog, (*ClientSEID), "break.For Client. Error line:" + __LINE__);

                                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                                break;
                                          }
                                          setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                          *funlog << "Server exit connect" << lns::endl;
                                          break;
                                    }
                                    else
                                    {
                                          if (!send_message((*ClientSocket), buf.c_str()))
                                          {
                                                sendError(*funlog, (*ClientSEID), "break.For Client. Error line:" + __LINE__);
                                                setClientState(ClientInfo_CM, ClientSocketFlagStruct::Offline);
                                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                                break;
                                          }
                                    }
                                    if (!receive_message((*ClientSocket), buf2) || // From Client
                                        !send_message(s, buf2))                    // To Server
                                    {
                                          recvError(*funlog, (*ClientSEID), "break.For Client. Error line:" + __LINE__);
                                          sendError(*funlog, (*ClientSEID), "break.For Server. Error line:" + __LINE__);
                                          send_message((*ClientInfo_CSM)
                                                           .ServerSocket,
                                                       "\r\nexit\r\n");
                                          setClientState(ClientInfo_CM, ClientSocketFlagStruct::Online);
                                          send_message(s, "\r\n\r\nfail\r\n\r\n");
                                          break;
                                    }
                              }
                              else
                              {
                                    *funlog << "Server unkonwn error" << lns::endl;
                              }
                        }
                  }
            }
            else
            {
                  if (!send_message(s, "\r\n\r\nfail\r\n\r\n"))
                  {
                        *funlog << "send to Server error "
                                << "error code:" << WSAGetLastError() << lns::endl
                                << "error line:" << __LINE__ << lns::endl;
                        break;
                  }
                  *funlog << "Can`t find client" << lns::endl;
            }
      }
      // break;
      *funlog << "Connect End" << lns::endl;
      return true;
}
extern "C" void EXPORT del(allInfoStruct *info)
{
      auto *ServerSEIDMap = &((*info).ServerSEIDMap);
      string seid = (*info).SEID;
      SEIDForSocketStruct *ServerInfo_SSM = &(*(*ServerSEIDMap))[seid];
      auto funlog = (*(*info).prlog_).getFunLog("del");
      SOCKET *s = &((*(*ServerSEIDMap))[seid].ServerSocket);
      string recvBuf;
      std::lock_guard<std::mutex> lock((*ServerInfo_SSM).ServerSocketLock);
      while (1)
      {
            {
                  std::lock_guard<std::mutex> lock((*ServerInfo_SSM).OtherValueLock);
                  if ((*ServerInfo_SSM).isBack)
                  {
                        *funlog << "Server is exit" << lns::endl;
                        break;
                  }
            }
            filter temp;
            if (showForSend(info, seid, temp) < 0)
            {
                  *funlog << "showForSend error" << lns::endl;
                  break;
            }
            bool state = false;
            {
                  state = receive_message(*s, recvBuf);
            }
            if (!state)
            {
                  *funlog << "Server Socket error "
                          << "error code:" << WSAGetLastError() << lns::endl;
                  break;
            }
            else if (state > 0)
            {
                  if (strcmp(recvBuf.c_str(), "\r\nexit\r\n") == 0)
                  {
                        *funlog << "Server exit del" << lns::endl;
                        break;
                  }
                  else if (strcmp(recvBuf.c_str(), "\r\nnext\r\n") == 0)
                  {
                        continue;
                  }
                  int setClientId = 0;
                  try
                  {
                        setClientId = atoi(recvBuf.c_str());
                  }
                  catch (const std::exception &e)
                  {
                        *funlog << "atoi error:" << e.what() << "\n"
                                << "error line:" << __LINE__ << "\n"
                                << "error file:" << __FILE__ << lns::endl;
                        break;
                  }
                  receive_message(*s, recvBuf);

                  bool delsec = false;
                  std::shared_ptr<thread> waitthread;
                  std::pair<bool, std::shared_ptr<thread>> delsecWait = {false, waitthread};
                  int res = delForId(info, setClientId, (recvBuf.find("Y") != string::npos), &delsecWait);
                  string sendMsg;
                  if (res == 0)
                  {
                        sendMsg = "\r\n\r\nok\r\n\r\n";
                  }
                  else if (res == 2)
                  {
                        send_message(*s, "\r\nwait\r\n");
                        if (recvBuf.find("Y") != string::npos)
                        {
                              while (!delsecWait.first)
                                    ;
                              sendMsg = "\r\n\r\nok\r\n\r\n";
                        }
                        else
                        {
                              sendMsg = "\r\n\r\nUse\r\n\r\n";
                        }
                  }
                  else
                  {
                        sendMsg = "\r\n\r\nUnError\r\n\r\n";
                  }
                  if (!send_message(*s, sendMsg))
                  {
                        *funlog << "send to Server error "
                                << "error code:" << WSAGetLastError() << lns::endl;
                        break;
                  }
            }
      }
      *funlog << "del End" << lns::endl;
}
extern "C" bool EXPORT show(allInfoStruct *info)
{
      auto *ServerSEIDMap = (*info).ServerSEIDMap;
      string seid = (*info).SEID;
      auto funlog = ((*info).prlog_)->getFunLog("show");
      std::lock_guard<std::mutex> lock((*ServerSEIDMap)[seid].ServerSocketLock);
      SOCKET *s = &(*ServerSEIDMap)[seid].ServerSocket;
      string recvBuf;
      while (1)
      {
            filter temp;
            {
                  std::lock_guard<std::mutex> lock((*ServerSEIDMap)[seid].OtherValueLock);
                  if ((*ServerSEIDMap)[seid].isBack)
                  {
                        *funlog << "Server is exit" << lns::endl;
                        break;
                  }
            }
            if (showForSend(info, seid, temp) < 0)
            {
                  *funlog << "showForSend error" << lns::endl;
                  break;
            }
            bool state = false;
            state = receive_message(*s, recvBuf);
            if (!state)
            {
                  *funlog << "Server Socket error "
                          << "error code:" << WSAGetLastError() << lns::endl;
                  break;
            }
            else if (recvBuf == "\r\nnext\r\n")
            {
                  continue;
            }
            else if (recvBuf == "\r\nexit\r\n")
            {
                  *funlog << "Server exit show " << lns::endl;
                  break;
            }
      }
      *funlog << "show End" << lns::endl;
      return true;
}
// extern "C" void EXPORT cmod(allInfoStruct &info)
// {
//     auto &ServerSEIDMap = (*info).ServerSEIDMap;
//     auto &ClientMap = (*info).ClientMap;
//     string seid = info.SEID;
//     vector<string> &cmods = info.msgVector;
//     int cmodsNum = cmods.size();
//     //[del,cmd,show][all,]
//     auto funlog = ((*info).prlog_).getFunLog("cmod");
//     SOCKET &s = ServerSEIDMap[seid].ServerSocket;
//     filter f;
//     {
//     addRule_error:
//         if (!send_message(s, "\r\ncmd error\r\n"))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//         }
//         *funlog << "Server cmd error" << lns::endl;
//         *funlog << "cmod End" << lns::endl;
//         return;
//     }
//     std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].ServerSocketLock);
//     if (strcmp(cmods[1].c_str(), "del") == 0 && cmodsNum >= 3)
//     {
//         {
//             std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//             if (ServerSEIDMap[seid].isBack)
//             {
//                 *funlog << "Server is exit" << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         {
//             if (!send_message(s, "\r\ndel\r\n"))
//             {
//                 *funlog << "send to Server error "
//                         << "error code:" << WSAGetLastError() << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         for (int i = 2; i <= cmodsNum; i += 3)
//         {
//             {
//                 std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//                 if (ServerSEIDMap[seid].isBack)
//                 {
//                     *funlog << "Server is exit" << lns::endl;
//                     *funlog << "cmod End" << lns::endl;
//                     return;
//                 }
//             }
//             switch (StringToIntInComd[cmods[i]])
//             {
//             case 5:
//                 if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 6:
//                 if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 7:
//                 if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 8:
//                 if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             }
//         }
//         int sectClient = 0;
//         for (int i = 1; i <= ClientMap.size(); i++)
//         {
//             if (f.matching((*ClientMap)[i - 1].ClientWanIp,
//                            (*ClientMap)[i - 1].ClientLanIp,
//                            to_string((*ClientMap)[i - 1].ClientConnectPort)) &&
//                 (*ClientMap)[i - 1].state != ClientSocketFlagStruct::Use)
//             {
//                 sectClient++;
//                 delForId(i);
//             }
//         }
//         if (ServerSEIDMap[seid].isBack)
//         {
//             *funlog << "Server exit" << lns::endl;
//             *funlog << "cmod End" << lns::endl;
//             return;
//         }
//         if (!send_message(s, "\r\nok\r\n") ||
//             !send_message(s, to_string(sectClient).c_str()) ||
//             !send_message(s, to_string(ClientMap.size()).c_str()))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//         }
//     }
//     else if (strcmp(cmods[1].c_str(), "cmd") == 0 && cmodsNum >= 4)
//     {
//         {
//             std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//             if (ServerSEIDMap[seid].isBack)
//             {
//                 *funlog << "Server is exit" << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         if (!send_message(s, "\r\ncmd\r\n"))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//             *funlog << "cmod End" << lns::endl;
//             return;
//         }
//         int cmdstart = 0;
//         // 一条指令
//         // cmd [port [!=,==,>,<,>=,<=] [port]] [wanip [!=,==] [ip]] [lanip [!=,==] [ip]] [all] "指令"
//         for (int i = 2; i <= cmodsNum && cmods[i][0] != '"'; i += 3, cmdstart = i)
//         {
//             {
//                 std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//                 if (ServerSEIDMap[seid].isBack)
//                 {
//                     *funlog << "Server is exit" << lns::endl;
//                     *funlog << "cmod End" << lns::endl;
//                     return;
//                 }
//             }
//             if (cmods[i][0] == '"')
//             {
//                 break;
//             }
//             switch (StringToIntInComd[cmods[i]])
//             {
//             case 5:
//                 if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 6:
//                 if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 7:
//                 if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//             case 8:
//                 if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             }
//         }
//         if (cmdstart > cmodsNum) // 判断指令格式是否标准
//         {
//             {
//                 std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//                 if (ServerSEIDMap[seid].isBack)
//                 {
//                     *funlog << "Server is exit" << lns::endl;
//                     *funlog << "cmod End" << lns::endl;
//                     return;
//                 }
//             }
//             if (!send_message(s, "\r\ncmd error\r\n"))
//             {
//                 *funlog << "send to Server error "
//                         << "error code:" << WSAGetLastError() << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//             *funlog << "Server cmd error" << lns::endl;
//             *funlog << "cmod End" << lns::endl;
//             return;
//         }
//         string sendBuf;
//         int sectClient = 0;
//         // 合并指令
//         /*
//         之前的存储模式：按空格分割
//         */
//         for (int i = cmdstart; i <= cmodsNum; i++)
//         {
//             sendBuf += cmods[i] + " ";
//         }
//         sendBuf = sendBuf.substr(1, sendBuf.length() - 2); // 去掉头尾的 ' " '
//         sendBuf += "\r\nexit\r\n";
//         {
//             while (ClientMapLock.exchange(true, std::memory_order_acquire))
//                 ; // 加锁
//             for (int i = 1; i <= ClientMap.size(); i++)
//             {
//                 if (f.matching((*ClientMap)[i - 1].ClientWanIp, (*ClientMap)[i - 1].ClientLanIp, to_string((*ClientMap)[i - 1].ClientConnectPort)) && (*ClientMap)[i - 1].state != ClientSocketFlagStruct::Use)
//                 {
//                     *funlog << "Server cmd match one" << lns::endl;
//                     sectClient++;
//                     if (!send_message((*ClientSEIDMap)[(*ClientMap)[i - 1].SEID].ServerSocket, sendBuf)) // 发送指令至Client
//                     {
//                         *funlog << "send to Client error "
//                                 << "error code:" << WSAGetLastError() << lns::endl;
//                         if (!send_message(s, "\r\ncmd error\r\n"))
//                         {
//                             *funlog << "send to Server error "
//                                     << "error code:" << WSAGetLastError() << lns::endl;
//                             ClientMapLock.exchange(false, std::memory_order_release);
//                             return;
//                         }
//                     }
//                 }
//             }
//             ClientMapLock.exchange(false, std::memory_order_release);
//         }
//         {
//             std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//             if (ServerSEIDMap[seid].isBack)
//             {
//                 *funlog << "Server is exit" << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         if (!send_message(s, "\r\nok\r\n") ||
//             !send_message(s, to_string(sectClient).c_str()) ||
//             !send_message(s, to_string(ClientMap.size()).c_str()))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//         }
//     }
//     else if (strcmp(cmods[1].c_str(), "show") == 0 && cmodsNum >= 3)
//     {
//         {
//             std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//             if (ServerSEIDMap[seid].isBack)
//             {
//                 *funlog << "Server is exit" << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         if (!send_message(s, "\r\nsee\r\n"))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//             *funlog << "cmod End" << lns::endl;
//             return;
//         }
//         for (int i = 2; i <= cmodsNum && (cmodsNum - i) >= 2; i += 3)
//         {
//             switch (StringToIntInComd[cmods[i]])
//             {
//             case 5:
//                 if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 6:
//                 if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 7:
//                 if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             case 8:
//                 if (!f.addRule(filter::ruleDataType::all, NULL, NULL))
//                 {
//                     goto addRule_error;
//                 }
//                 break;
//             default:
//                 goto addRule_error;
//             }
//         }
//         int showClient = showForSend(info, seid, f);
//         {
//             std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
//             if (ServerSEIDMap[seid].isBack)
//             {
//                 *funlog << "Server is exit" << lns::endl;
//                 *funlog << "cmod End" << lns::endl;
//                 return;
//             }
//         }
//         if (!send_message(s, "\r\nok\r\n") ||
//             !send_message(s, to_string(showClient).c_str()) ||
//             !send_message(s, to_string(ClientMap.size()).c_str()))
//         {
//             *funlog << "send to Server error "
//                     << "error code:" << WSAGetLastError() << lns::endl;
//         }
//     }
//     *funlog << "cmod End" << lns::endl;
//     return;
// }