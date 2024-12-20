#include "saveDatadll.h"
#include "plugin.h"

extern "C" bool EXPORT Connect(allInfoStruct &info)
{
    string seid = info.SEID;
    auto funlog = (*info.prlog_).getFunLog("Connect");
    *funlog << "OK";
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    std::lock_guard<std::mutex> lockSEID(ServerSEIDMap[seid].ServerSocketLock);
    while (1)
    {
        recvBuf.clear();
        {
            std::lock_guard<std::mutex> lockSEID(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                break;
            }
        }

        filter temp;
        if (showForSend(seid, temp, true, ClientSocketFlagStruct::Online) < 0)
        {
            cout << "1";
            break;
        }
        bool state = false;
        {
            state = receive_message(s, recvBuf);
            *funlog << (state ? "OK" : "Error");
        }
        if (!state)
        {
            cout << "2";
            *funlog << "Server recv error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            break;
        }
        else if (state > 0)
        {
            cout << "3";
            if (strcmp(recvBuf.c_str(), "\r\nexit\r\n") == 0)
            {

                *funlog << "Server exit Connect "
                        << "error code:" << WSAGetLastError() << lns::endl;
                break;
            }
            else if (strcmp(recvBuf.c_str(), "\r\nnext\r\n") == 0)
            {
                cout << "4";
                *funlog << "Server next Connect" << lns::endl;
                continue;
            }
            int setClientId = atoi(recvBuf.c_str());
            int ClientIndex = -1;
            for (int i = 0; i < ClientMap.size(); i++)
            {
                if (ClientMap[i].state == ClientSocketFlagStruct::Online)
                {
                    setClientId--;
                    if (setClientId == 0)
                    {
                        ClientMap[i].state = ClientSocketFlagStruct::Use;
                        ClientIndex = i;
                        break;
                    }
                }
            }

            if (ClientIndex != -1)
            {
                string buf, buf2, temp;
                {
                    *funlog << "ClientIndex:" << ClientIndex << lns::endl;
                    *funlog << "ClientMap[ClientIndex].SEID:" << ClientMap[ClientIndex].SEID << lns::endl;
                    *funlog << "Server start connect Client" << lns::endl;
                    std::lock_guard<std::mutex> lockSEIDForClient(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocketLock);
                    std::lock_guard<std::mutex> lockOtherValueForClient(ClientSEIDMap[ClientMap[ClientIndex].SEID].OtherValueLock);
                    if (!send_message(s, "\r\n\r\nsec\r\n\r\n"))
                    {
                        *funlog << "send to Server error "
                                << "error code:" << WSAGetLastError() << lns::endl;
                        ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                        break;
                    }
                    {
                        bool failButNext = false;
                        if (!send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "\r\nstart\r\n"))
                        {
                            sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Client");
                            ClientMap[ClientIndex].state = ClientSocketFlagStruct::Offline;
                            if (!send_message(s, "\r\n\r\nfail\r\n\r\n"))
                            {
                                sendError(*funlog, seid, "break");
                            }
                            break;
                        }
                        if (!receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2))
                        {
                            recvError(*funlog, ClientMap[ClientIndex].SEID, "next.For Client");
                            failButNext = true;
                        }
                        if (!send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "\r\n"))
                        {
                            sendError(*funlog, ClientMap[ClientIndex].SEID, "next.For Client");
                            failButNext = true;
                        }
                        if (!receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, temp))
                        {
                            recvError(*funlog, ClientMap[ClientIndex].SEID, "next.For Client");
                            failButNext = true;
                        }
                        buf2 += temp;
                        if (failButNext)
                        {
                            if (!send_message(s, "\r\n\r\nfailn\r\n\r\n"))
                            {
                                sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Server");
                                ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                break;
                            }
                        }
                        if (!send_message(s, buf2))
                        {
                            sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Server");
                            ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                            send_message(s, "\r\n\r\nfail\r\n\r\n");
                            break;
                        }
                        *funlog << "Server connect init ok" << lns::endl;
                    }
                }
                while (1)
                {
                    if (ServerSEIDMap[seid].isBack)
                    {
                        *funlog << "Server is exit" << lns::endl;
                        ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
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
                        recvError(*funlog, ClientMap[ClientIndex].SEID, "break.For server");
                        ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                        break;
                    }
                    else if (state > 0)
                    {
#ifdef _DEBUG
                        *funlog << "recv:" << buf << lns::endl;
#endif
                        std::lock_guard<std::mutex> lockSEIDForClient(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocketLock);
                        std::lock_guard<std::mutex> lockOtherValueForClient(ClientSEIDMap[ClientMap[ClientIndex].SEID].OtherValueLock);
                        if (strcmp(buf.c_str(), "\r\nfexit\r\n") == 0)
                        {
                            if (!send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "exit\r\n") ||
                                !receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2))
                            {
                                sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Client");
                                recvError(*funlog, ClientMap[ClientIndex].SEID, "break.For Client");
                                ClientMap[ClientIndex].state = ClientSocketFlagStruct::Offline;
                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                break;
                            }
                            ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                            *funlog << "Server exit connect" << lns::endl;
                            break;
                        }
                        else
                        {
                            if (!send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf.c_str()))
                            {
                                sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Client");
                                ClientMap[ClientIndex].state = ClientSocketFlagStruct::Offline;
                                send_message(s, "\r\n\r\nfail\r\n\r\n");
                                break;
                            }
                        }
                        if (!receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2) ||
                            !send_message(s, buf2))
                        {
                            recvError(*funlog, ClientMap[ClientIndex].SEID, "break.For Client");
                            sendError(*funlog, ClientMap[ClientIndex].SEID, "break.For Server");
                            ClientMap[ClientIndex].state = ClientSocketFlagStruct::Offline;
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
                        << "error code:" << WSAGetLastError() << lns::endl;
                break;
            }
            *funlog << "Can`t find client" << lns::endl;
        }
    }
    // break;
    *funlog << "Connect End" << lns::endl;
    return true;
}
extern "C" void EXPORT del(allInfoStruct &info)
{
    string seid = info.SEID;
    auto funlog = (*info.prlog_).getFunLog("del");
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].ServerSocketLock);
    while (1)
    {
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                break;
            }
        }
        filter temp;
        if (showForSend(seid, temp) < 0)
        {
            *funlog << "showForSend error" << lns::endl;
            break;
        }
        bool state = false;
        {
            state = receive_message(s, recvBuf);
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
            int setClientId = atoi(recvBuf.c_str());
            int res = delForId(setClientId);
            string sendMsg;
            if (res == 0)
            {
                sendMsg = "\r\n\r\nok\r\n\r\n";
            }
            else if (res == 2)
            {
                sendMsg = "\r\n\r\nUse\r\n\r\n";
            }
            else
            {
                sendMsg = "\r\n\r\nUnError\r\n\r\n";
            }
            if (!send_message(s, sendMsg))
            {
                *funlog << "send to Server error "
                        << "error code:" << WSAGetLastError() << lns::endl;
                break;
            }
        }
    }
    *funlog << "del End" << lns::endl;
}
extern "C" void EXPORT show(allInfoStruct &info)
{
    string seid = info.SEID;
    auto funlog = (*info.prlog_).getFunLog("show");
    std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].ServerSocketLock);
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    while (1)
    {
        filter temp;
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                break;
            }
        }
        if (showForSend(seid, temp) < 0)
        {
            *funlog << "showForSend error" << lns::endl;
            break;
        }
        bool state = false;
        {
            state = receive_message(s, recvBuf);
        }
        if (!state)
        {
            *funlog << "Server Socket error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            *funlog << "show End" << lns::endl;
            return;
        }
        else if (strcmp(recvBuf.c_str(), "\r\nclose\r\n") == 0)
        {
            *funlog << "Server exit show "
                    << "error code:" << WSAGetLastError() << lns::endl;
            *funlog << "show End" << lns::endl;
            return;
        }
    }
    *funlog << "show End" << lns::endl;
    return;
}
extern "C" void EXPORT cmod(allInfoStruct &info)
{
    string seid = info.SEID;
    vector<string> &cmods = info.msgVector;
    int cmodsNum = cmods.size();
    //[del,cmd,show][all,]
    auto funlog = (*info.prlog_).getFunLog("cmod");
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    filter f;
    {
    addRule_error:
        if (!send_message(s, "\r\ncmd error\r\n"))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
        }
        *funlog << "Server cmd error" << lns::endl;
        *funlog << "cmod End" << lns::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].ServerSocketLock);
    if (strcmp(cmods[1].c_str(), "del") == 0 && cmodsNum >= 3)
    {
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        {
            if (!send_message(s, "\r\ndel\r\n"))
            {
                *funlog << "send to Server error "
                        << "error code:" << WSAGetLastError() << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        for (int i = 2; i <= cmodsNum; i += 3)
        {
            {
                std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
                if (ServerSEIDMap[seid].isBack)
                {
                    *funlog << "Server is exit" << lns::endl;
                    *funlog << "cmod End" << lns::endl;
                    return;
                }
            }
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            }
        }
        int sectClient = 0;
        for (int i = 1; i <= ClientMap.size(); i++)
        {
            if (f.matching(ClientMap[i - 1].ClientWanIp,
                           ClientMap[i - 1].ClientLanIp,
                           to_string(ClientMap[i - 1].ClientConnectPort)) &&
                ClientMap[i - 1].state != ClientSocketFlagStruct::Use)
            {
                sectClient++;
                delForId(i);
            }
        }
        if (ServerSEIDMap[seid].isBack)
        {
            *funlog << "Server exit" << lns::endl;
            *funlog << "cmod End" << lns::endl;
            return;
        }
        if (!send_message(s, "\r\nok\r\n") ||
            !send_message(s, to_string(sectClient).c_str()) ||
            !send_message(s, to_string(ClientMap.size()).c_str()))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
        }
    }
    else if (strcmp(cmods[1].c_str(), "cmd") == 0 && cmodsNum >= 4)
    {
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        if (!send_message(s, "\r\ncmd\r\n"))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            *funlog << "cmod End" << lns::endl;
            return;
        }
        int cmdstart = 0;
        // 一条指令
        // cmd [port [!=,==,>,<,>=,<=] [port]] [wanip [!=,==] [ip]] [lanip [!=,==] [ip]] [all] "指令"
        for (int i = 2; i <= cmodsNum && cmods[i][0] != '"'; i += 3, cmdstart = i)
        {
            {
                std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
                if (ServerSEIDMap[seid].isBack)
                {
                    *funlog << "Server is exit" << lns::endl;
                    *funlog << "cmod End" << lns::endl;
                    return;
                }
            }
            if (cmods[i][0] == '"')
            {
                break;
            }
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            }
        }
        if (cmdstart > cmodsNum) // 判断指令格式是否标准
        {
            {
                std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
                if (ServerSEIDMap[seid].isBack)
                {
                    *funlog << "Server is exit" << lns::endl;
                    *funlog << "cmod End" << lns::endl;
                    return;
                }
            }
            if (!send_message(s, "\r\ncmd error\r\n"))
            {
                *funlog << "send to Server error "
                        << "error code:" << WSAGetLastError() << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
            *funlog << "Server cmd error" << lns::endl;
            *funlog << "cmod End" << lns::endl;
            return;
        }
        string sendBuf;
        int sectClient = 0;
        // 合并指令
        /*
        之前的存储模式：按空格分割
        */
        for (int i = cmdstart; i <= cmodsNum; i++)
        {
            sendBuf += cmods[i] + " ";
        }
        sendBuf = sendBuf.substr(1, sendBuf.length() - 2); // 去掉头尾的 ' " '
        sendBuf += "\r\nexit\r\n";
        {
            while (ClientMapLock.exchange(true, std::memory_order_acquire))
                ; // 加锁
            for (int i = 1; i <= ClientMap.size(); i++)
            {
                if (f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)) && ClientMap[i - 1].state != ClientSocketFlagStruct::Use)
                {
                    *funlog << "Server cmd match one" << lns::endl;
                    sectClient++;
                    if (!send_message(ClientSEIDMap[ClientMap[i - 1].SEID].ServerSocket, sendBuf)) // 发送指令至Client
                    {
                        *funlog << "send to Client error "
                                << "error code:" << WSAGetLastError() << lns::endl;
                        if (!send_message(s, "\r\ncmd error\r\n"))
                        {
                            *funlog << "send to Server error "
                                    << "error code:" << WSAGetLastError() << lns::endl;
                            ClientMapLock.exchange(false, std::memory_order_release);
                            return;
                        }
                    }
                }
            }
            ClientMapLock.exchange(false, std::memory_order_release);
        }
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        if (!send_message(s, "\r\nok\r\n") ||
            !send_message(s, to_string(sectClient).c_str()) ||
            !send_message(s, to_string(ClientMap.size()).c_str()))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
        }
    }
    else if (strcmp(cmods[1].c_str(), "show") == 0 && cmodsNum >= 3)
    {
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        if (!send_message(s, "\r\nsee\r\n"))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            *funlog << "cmod End" << lns::endl;
            return;
        }
        for (int i = 2; i <= cmodsNum && (cmodsNum - i) >= 2; i += 3)
        {
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    goto addRule_error;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, NULL, NULL))
                {
                    goto addRule_error;
                }
                break;
            default:
                goto addRule_error;
            }
        }
        int showClient = showForSend(seid, f);
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[seid].OtherValueLock);
            if (ServerSEIDMap[seid].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "cmod End" << lns::endl;
                return;
            }
        }
        if (!send_message(s, "\r\nok\r\n") ||
            !send_message(s, to_string(showClient).c_str()) ||
            !send_message(s, to_string(ClientMap.size()).c_str()))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
        }
    }
    *funlog << "cmod End" << lns::endl;
    return;
}