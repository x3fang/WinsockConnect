#include "global.h"
#include "saveData.h"
#include "filter.h"
logNameSpace::Log prlog;
int showForSend(string, filter, bool, ClientSocketFlagStruct::states);
void sendError(logNameSpace::funLog &funlog, string seid, string nextDo)
{
    funlog << "send error.SEID:"
           << seid
           << " error code:" << WSAGetLastError()
           << " next do: " << nextDo
           << lns::endl;
    return;
}
void recvError(logNameSpace::funLog &funlog, string seid, string nextDo)
{
    funlog << "recv error.SEID:"
           << seid
           << " error code:" << WSAGetLastError()
           << " next do: " << nextDo
           << lns::endl;
    return;
}
map<string, int> StringToInt =
    {
        {"connect", 1},
        {"del", 2},
        {"show", 3},
        {"cmd", 4}

};
bool send_message(SOCKET sock, const std::string &message)
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
bool receive_message(SOCKET sock, std::string &message)
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
int initServer(SOCKET &ListenSocket, sockaddr_in &sockAddr, int port)
{
    auto funlog = prlog.getFunLog("initServer");
    *funlog << "initServer Start" << lns::endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        *funlog << "WSAStartup() failed with error: " << iResult << lns::endl;
        *funlog << "initServer End" << lns::endl;
        return iResult;
    }
#endif
    // 创建套接字
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET)
    {
        int errorCode = WSAGetLastError();
        *funlog << "socket() failed with error: " << errorCode << lns::endl;
        *funlog << "initServer End" << lns::endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        WSACleanup();
#endif
        return errorCode;
    }
    // 绑定套接字

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);
    if (bind(ListenSocket, (SOCKADDR *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        WSACleanup();
#endif
        int errorCode = WSAGetLastError();
        *funlog << "bind() failed with error: " << errorCode << lns::endl;
        *funlog << "initServer End" << lns::endl;
        return errorCode;
    }
    // 监听套接字
    if (listen(ListenSocket, 5) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        WSACleanup();
#endif
        int errorCode = WSAGetLastError();
        *funlog << "listen() failed with error: " << errorCode << lns::endl;
        *funlog << "initServer End" << lns::endl;
        return errorCode;
    }
    *funlog << "initServer End" << lns::endl;
    return 0;
}
string StringTime(time_t t1)
{
    auto funlog = prlog.getFunLog("StringTime");
    time_t t = t1;
    char tmp[64];
    struct tm *timinfo;
    timinfo = localtime(&t);
    strftime(tmp, sizeof(tmp), "%Y%m%d%H%M", timinfo);
    *funlog << "String Time End" << lns::endl;
    return tmp;
}
void setServerOrClientExit(SEIDForSocketStruct &s)
{
    auto funlog = prlog.getFunLog("setServerOrClientExit");
    // 设置退出标志
    {
        std::lock_guard<std::mutex> lock(s.OtherValueLock);
        s.isBack = true;
        s.isSocketExit = false;
        *funlog << "set status isBack" << lns::endl;
    }
    // 关闭测试套接字
    {
        std::lock_guard<std::mutex> lock(s.ServerHealthySocketLock);
        closesocket(s.socketH);
        *funlog << "close healthy cheacksocket" << lns::endl;
    }
    // 关闭连接套接字
    {
        std::lock_guard<std::mutex> lock(s.ServerSocketLock);
        closesocket(s.ServerSocket);
        *funlog << "close ServerSocket" << lns::endl;
    }
    *funlog << "setServerOrClientExit End" << lns::endl;
}
void healthyLock()
{
    while (HealthyLock.exchange(true, std::memory_order_acquire))
        ; // 加锁)
}
void healthyUnlock()
{
    HealthyLock.exchange(false, std::memory_order_release); // 解锁
}
void HealthyCheack()
{
    auto funlog = prlog.getFunLog("HealthyCheack");
    *funlog << "HealthyCheack Start" << lns::endl;
    while (1)
    {
        if (!HealthyQueue.empty())
        {
            healthyLock();
            for (int i = 0; i < HealthyQueue.size(); i++)
            {
                SOCKET s;
                SEIDForSocketStruct *temp;
                string SEID = HealthyQueue.front().SEID;

                if (HealthyQueue.front().isServer)
                {
                    *funlog << "start Server HealthyCheck,SEID:" << SEID << lns::endl;
                    temp = &ServerSEIDMap[SEID];
                }
                else
                {
                    *funlog << "start Client HealthyCheck,SEID:" << SEID << lns::endl;
                    temp = &ClientSEIDMap[SEID];
                }
                std::lock_guard<std::mutex> lock(temp->ServerHealthySocketLock);

                srand(time(NULL) + rand());
                string sendMsg = to_string(rand() % 1000000000);
                string buf;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                int timeout = 3000;
                setsockopt(temp->socketH, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
                setsockopt(temp->socketH, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#elif __linux__
                socklen_t optlen = sizeof(struct timeval);
                struct timeval tv;
                tv.tv_sec = 3;
                tv.tv_usec = 0;
                setsockopt(temp->socketH, SOL_SOCKET, SO_SNDTIMEO, &tv, optlen);
                setsockopt(temp->socketH, SOL_SOCKET, SO_RCVTIMEO, &tv, optlen);
#endif
                int state = send_message(temp->socketH, sendMsg);
                int state1 = receive_message(temp->socketH, buf);
                if (strcmp(buf.c_str(), "\r\nClose\r\n") == 0)
                {
                    *funlog << (HealthyQueue.front().isServer ? "Server" : "Client")
                            << " HealthyCheck Status:Close SEID:" << SEID << lns::endl;
                    HealthyQueue.pop();
                    setServerOrClientExit(*temp);
                    continue;
                }
                if (buf != sendMsg || temp->isBack || !state || !state1)
                {
                    *funlog << (HealthyQueue.front().isServer ? "Server" : "Client")
                            << " HealthyCheck Status:Error,error code:"
                            << WSAGetLastError()
                            << " SEID:" << SEID << lns::endl;
                    HealthyQueue.pop();
                    setServerOrClientExit(*temp);
                    continue;
                }
                HealthyQueue.push({temp->SEID, HealthyQueue.front().isServer});
                HealthyQueue.pop();
                temp = nullptr;
                Sleep(300);
            }
            healthyUnlock();
        }
    }
    *funlog << "HealthyCheack End" << lns::endl;
}
int showForSend(string seid, filter f, bool startIf = false, ClientSocketFlagStruct::states state = ClientSocketFlagStruct::NULLs)
{
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    int canShowClient = 0;
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    for (int i = 1; i <= ClientMap.size(); i++)
    {
        if (ServerSEIDMap[seid].isBack)
        {
            ClientMapLock.exchange(false, std::memory_order_release);
            return -1;
        }
        if (!startIf || ClientMap[i - 1].state == state || f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)))
        {
            canShowClient++;
            string sendBuf = ClientMap[i - 1].ClientWanIp + " " + ClientMap[i - 1].ClientLanIp + " " + to_string(ClientMap[i - 1].ClientConnectPort) + " " + to_string(ClientMap[i - 1].state);
            if (!send_message(s, sendBuf))
            {
                ClientMapLock.exchange(false, std::memory_order_release);
                return -2;
            }
        }
    }
    if (!send_message(s, "\r\n\r\nend\r\n\r\n"))
    {
        ClientMapLock.exchange(false, std::memory_order_release);
        return -3;
    }
    ClientMapLock.exchange(false, std::memory_order_release);
    return canShowClient;
}
void Connect(string seid, vector<string> cmods, int cmodsNum)
{
    auto funlog = prlog.getFunLog("Connect");
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
            break;
        }
        bool state = false;
        {
            state = receive_message(s, recvBuf);
        }
        if (!state)
        {
            *funlog << "Server recv error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            break;
        }
        else if (state > 0)
        {
            if (strcmp(recvBuf.c_str(), "\r\nexit\r\n") == 0)
            {
                *funlog << "Server exit Connect "
                        << "error code:" << WSAGetLastError() << lns::endl;
                break;
            }
            else if (strcmp(recvBuf.c_str(), "\r\nnext\r\n") == 0)
            {
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
}
int delForId(int ClientId)
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
void del(string seid, vector<string> cmods, int cmodsNum)
{
    auto funlog = prlog.getFunLog("del");
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
void show(string seid, vector<string> cmods, int cmodsNum)
{
    auto funlog = prlog.getFunLog("show");
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
void cmod(string seid, vector<string> cmods, int cmodsNum)
{
    //[del,cmd,show][all,]
    auto funlog = prlog.getFunLog("cmod");
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
string createSEID(SOCKET sock, string something = NULL)
{
    MD5 m;
    m.init();
    return m.encode(StringTime(time(NULL)) + to_string(sock));
}
void joinClient(string ClientWanIp, string ClientLanIp, string ClientPort, unsigned long long int OnlineTime, unsigned long long int OfflineTime, string SEID)
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
// void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
// {
//     HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);             // 获取缓冲区句柄
//     SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); // 设置文本及背景色
// }
void ServerRS(SOCKET s)
{
    string lastloginYZM;
    string loginYZM;
    lastloginYZM.clear();
    bool loginTRUE = false;
    MD5 m;
    for (int i = 1; i <= 5 && !loginTRUE; i++)
    {
        m.init();
        string recvBuf;
        if (!receive_message(s, recvBuf))
        {
            return;
        }
        loginYZM = m.encode(StringTime(time(NULL)) + lastloginYZM + password);
        if (recvBuf == loginYZM)
        {
            loginTRUE = true;
            if (!send_message(s, "true"))
            {
                return;
            }
            break;
        }
        if (!send_message(s, "error"))
        {
            return;
        }
        lastloginYZM = recvBuf;
    }
    if (!loginTRUE)
    {
        auto funlog = prlog.getFunLog("ServerRS");
        *funlog << "login error" << lns::endl;
        return;
    }
    string SEID = createSEID(s, lastloginYZM + loginYZM);
    auto funlog = prlog.getFunLog("ServerRS");
    {
        std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
        if (!send_message(s, SEID.c_str()))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            return;
        }
    }
    {
        std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].OtherValueLock);
        ServerSEIDMap[SEID].isSEIDExit = true;
        ServerSEIDMap[SEID].SEID = SEID;
        ServerSEIDMap[SEID].ServerSocket = s;
    }
    std::unique_lock<std::mutex> lock(ServerSEIDMap[SEID].isSocketExitLock);
    ServerSEIDMap[SEID].cv.wait(lock, [SEID]
                                { return ServerSEIDMap[SEID].isSocketExit; });
    *funlog << "Server online" << lns::endl;
    try
    {
        healthyLock();
        HealthyQueue.push({SEID, true});
        healthyUnlock();
    }
    catch (std::exception &e)
    {
        *funlog << "Server Healthy Queue push error" << lns::endl;
    }

    while (1)
    {
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].OtherValueLock);
            if (ServerSEIDMap[SEID].isBack)
            {
                *funlog << "Server is exit" << lns::endl;
                *funlog << "ServerRS End" << lns::endl;
                break;
            }
        }
        string recvBuf;
        bool state = false;
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
            state = receive_message(s, recvBuf);
        }
        if (!state || recvBuf.find("\r\nClose\r\n") != string::npos)
        {
            *funlog << "recv From Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
            std::lock_guard<std::mutex> lockO(ServerSEIDMap[SEID].OtherValueLock);
            ServerSEIDMap[(string)SEID].isBack = true;
            closesocket(s);
            *funlog << "Server exit" << lns::endl;
            *funlog << "ServerRS End" << lns::endl;
            return;
        }
        else
        {
            *funlog << "Recv:\n---------------------------------------------" << lns::endl
                    << string(recvBuf).substr(0, (recvBuf.length() > 60 ? recvBuf.length() / 2 : recvBuf.length() - 1)) << lns::endl
                    << "....\n---------------------------------------------" << lns::endl;
            vector<string> cmods;
            string token;
            int tokenNum = 0;
            istringstream iss((string)recvBuf);
            while (getline(iss, token, ' '))
            {
                tokenNum++;
                cmods.push_back(token);
            }
            switch (StringToInt[cmods[0]])
            {
            case 1:
                *funlog << "Server command:Connect" << lns::endl;
                Connect(SEID, cmods, tokenNum); // ok
                break;
            case 2:
                *funlog << "Server command:del Client" << lns::endl;
                del(SEID, cmods, tokenNum); // ok
                break;
            case 3:
                *funlog << "Server command:show Client" << lns::endl;
                show(SEID, cmods, tokenNum); // ok
                break;
            case 4:
                *funlog << "Server command:run command" << lns::endl;
                cmod(SEID, cmods, tokenNum); // ok
                break;
            default:
                break;
            }
        }
    }
}
void ClientRS(SOCKET s)
{
    string recvBuf;
    if (!receive_message(s, recvBuf))
    {
        return;
    }
    istringstream iss((string)recvBuf);
    string ClientWanIp, ClientLanIp, ClientPort, ClientState;
    iss >> ClientWanIp >> ClientLanIp >> ClientPort;
    string SEID = createSEID(s, ClientLanIp + ClientWanIp);
    {
        std::lock_guard<std::mutex> lock(ClientSEIDMap[SEID].OtherValueLock);
        ClientSEIDMap[SEID].ServerSocket = s;
        ClientSEIDMap[SEID].isSEIDExit = true;
    }

    auto funlog = prlog.getFunLog("ClientRS");
    if (!send_message(s, SEID))
    {
        *funlog << "send to Client error "
                << "error code:" << WSAGetLastError() << lns::endl;
        return;
    }
    joinClient(ClientWanIp, ClientLanIp, ClientPort, time(NULL), 0, SEID);
    std::unique_lock<std::mutex> lock(ClientSEIDMap[SEID].isSocketExitLock);
    ClientSEIDMap[SEID].cv.wait(lock, [SEID]
                                { return ClientSEIDMap[SEID].isSocketExit; });
    *funlog << "Client online" << lns::endl;
    healthyLock();
    HealthyQueue.push({SEID, true});
    healthyUnlock();
    while (1)
    {
        if (ClientSEIDMap[(string)SEID].isBack)
        {
            *funlog << "Client exit" << lns::endl;
            closesocket(s);
            return;
        }
        Sleep(1000);
    }
}
bool nozero(int &num)
{
    return num > 0;
}
void ServerConnect()
{

    while (true)
    {
        std::unique_lock<std::mutex> lock(ServerQueueLock);
        Queuecv.wait(lock, [&]
                     { return ServerSocketQueue.size() > 0; });
        for (; ServerSocketQueue.size() > 0;)
        {
            SOCKET ServerSocket = ServerSocketQueue.front();
            ServerSocketQueue.pop();
            thread ServerRSThread = thread(ServerRS, ServerSocket);
            ServerRSThread.detach();
        }
    }
}
void ClientConnect()
{

    while (1)
    {
        std::unique_lock<std::mutex> lock(ClientQueueLock);
        Queuecv.wait(lock, [&]
                     { return ClientSocketQueue.size() > 0; });
        for (; ClientSocketQueue.size() > 0;)
        {
            SOCKET ClientSocket = ClientSocketQueue.front();
            ClientSocketQueue.pop();
            ClientRSThreadArry.push_back(thread(ClientRS, ClientSocket));
        }
    }
}
int main(int argc, char **argv)
{
    string temp = argv[0];
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    temp = temp.substr(temp.find_last_of("\\") + 1);
    temp = temp.substr(0, temp.find(".exe"));
#endif
#if __linux__
    temp = temp.substr(temp.find_last_of("/") + 1);
    temp = temp.substr(0, temp.find(".out"));
#endif
    prlog = logNameSpace::Log(temp);
    prlog << "program start" << lns::endl;
    prlog << "log init ok" << lns::endl;
    prlog << "program name:" << temp << lns::endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    system("chcp 65001");
#endif
    loadData();
    prlog << "loadData OK" << lns::endl;
    int ir = initServer(ListenSocket, service, ServerPort);
    if (ir != 0)
    {
        prlog << "ERROR:initServer failed" << lns::endl;
        prlog << "Error code:" << ir << lns::endl;
        return ir;
    }
    thread HealthyCheackThread = thread(HealthyCheack);
    thread ClientConnectThread = thread(ClientConnect);
    thread ServerConnectThread = thread(ServerConnect);
    thread dataSaveThread = thread(dataSave);
    thread PassDataThread = thread(passData);
    prlog << "server init ok" << lns::endl;
    prlog << ("port:" + to_string(ServerPort)) << lns::endl;
    prlog << "Listen!" << lns::endl;
    cout << "star Server" << endl;
    while (true)
    {
        string buf;
        SOCKET aptSocket;
        sockaddr_in aptsocketAddr = {0};
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        int len = sizeof(aptsocketAddr);
#endif
#if __linux__
        socklen_t len = sizeof(aptsocketAddr);
#endif
        aptSocket = accept(ListenSocket, (SOCKADDR *)&aptsocketAddr, &len);
        if (aptSocket != INVALID_SOCKET)
        {
            if (!receive_message(aptSocket, buf))
            {
                closesocket(aptSocket);
                prlog << "recv From Socket error "
                      << "error code:" << WSAGetLastError() << lns::endl;
                continue;
            }
            if (strcmp(buf.c_str(), "Client") == 0)
            {
                if (!send_message(aptSocket, "OK"))
                {
                    closesocket(aptSocket);
                    prlog << "send to Client error "
                          << "error code:" << WSAGetLastError() << lns::endl;
                    continue;
                }
                prlog << "Client Connect" << lns::endl;
                ClientSocketQueue.push(aptSocket);
                Queuecv.notify_all();
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(aptsocketAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
                prlog << "Client IP:" << clientIP << lns::endl;
            }
            else if (strcmp(buf.c_str(), "Server") == 0)
            {
                if (!send_message(aptSocket, "OK"))
                {
                    closesocket(aptSocket);
                    prlog << "send to Server error "
                          << "error code:" << WSAGetLastError() << lns::endl;
                    continue;
                }
                prlog << "Server Connect" << lns::endl;
                ServerSocketQueue.push(aptSocket);
                Queuecv.notify_all();
                char ServerIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(aptsocketAddr.sin_addr), ServerIP, INET_ADDRSTRLEN);
                prlog << "Server IP:" << ServerIP << lns::endl;
            }
            else if (ServerSEIDMap.find(buf) != ServerSEIDMap.end())
            {
                std::lock_guard<std::mutex> lockH(ServerSEIDMap[buf].ServerHealthySocketLock);
                std::lock_guard<std::mutex> lockO(ServerSEIDMap[buf].OtherValueLock);
                ServerSEIDMap[buf].socketH = aptSocket;
                ServerSEIDMap[buf].isSocketExit = true;
                ServerSEIDMap[buf].cv.notify_all();
                prlog << "Server Healthy Connect" << lns::endl;
                char ServerIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(aptsocketAddr.sin_addr), ServerIP, INET_ADDRSTRLEN);
                prlog << "Server IP:" << ServerIP << lns::endl;
            }
            else if (ClientSEIDMap.find(buf) != ClientSEIDMap.end())
            {
                std::lock_guard<std::mutex> lockH(ClientSEIDMap[buf].ServerHealthySocketLock);
                std::lock_guard<std::mutex> lockO(ClientSEIDMap[buf].OtherValueLock);
                ClientSEIDMap[buf].socketH = aptSocket;
                ClientSEIDMap[buf].isSocketExit = true;
                ClientSEIDMap[buf].cv.notify_all();
                prlog << "Client Healthy Connect" << lns::endl;
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(aptsocketAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
                prlog << "Client IP:" << clientIP << lns::endl;
            }
            else
            {
                prlog << "Connect error" << lns::endl;
                closesocket(aptSocket);
            }
        }
    }
    return 0;
}