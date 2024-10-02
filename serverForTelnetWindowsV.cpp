#include "global.h"
#include "saveData.h"
#include "filter.h"
int showForSend(string, filter, bool, ClientSocketFlagStruct::states);
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
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        // case CTRL_CLOSE_EVENT: // 关闭
        //     End();
        //     break;
        // case CTRL_LOGOFF_EVENT: // 用户退出
        //     End();
        //     break;
        // case CTRL_SHUTDOWN_EVENT: // 系统被关闭时.
        //     End();
        //     break;
    }
    return TRUE;
}
int initServer(SOCKET &ListenSocket, sockaddr_in &sockAddr, int port)
{
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
        return iResult;
    // 创建套接字
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET)
    {
        WSACleanup();
        return WSAGetLastError();
    }
    // 绑定套接字

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);
    if (bind(ListenSocket, (SOCKADDR *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
        WSACleanup();
        return WSAGetLastError();
    }
    // 监听套接字
    if (listen(ListenSocket, 5) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
        WSACleanup();
        return WSAGetLastError();
    }
    return 0;
}
string StringTime(time_t t1)
{
    time_t t = t1;
    char tmp[64];
    struct tm *timinfo;
    timinfo = localtime(&t);

    strftime(tmp, sizeof(tmp), "%Y%m%d%H%M", timinfo);
    return tmp;
}
void HealthyCheckByServer(const string SEID)
{
    int timeout = 3000; // 设置超时时间为 3 秒
    // timeval timeout = {1, 0};
    ServerSEIDMap[SEID].getServerHealthySocketLock();
    try
    {
        if (SOCKET_ERROR == setsockopt(ServerSEIDMap[SEID].socketH, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)))
            cout << GetLastError() << endl;
        if (SOCKET_ERROR == setsockopt(ServerSEIDMap[SEID].socketH, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)))
            cout << GetLastError() << endl;
    }
    catch (std::runtime_error &e)
    {
        cout << e.what() << endl;
    }
    catch (std::exception &e)
    {
        cout << e.what() << endl;
    }
    catch (...)
    {
        cout << "unknown error" << endl;
    }
    ServerSEIDMap[SEID].releaseServerHealthySocketLock();
    while (1)
    {
        srand(time(NULL) + rand());
        string sendMsg = to_string(rand() % 1000000000);
        string buf;
        ServerSEIDMap[SEID].getServerHealthySocketLock();
        int state = send_message(ServerSEIDMap[SEID].socketH, sendMsg);
        int state1 = receive_message(ServerSEIDMap[SEID].socketH, buf);
        if (strcmp(buf.c_str(), "\r\nClose\r\n") == 0)
        {
            ServerSEIDMap[SEID].getOtherValueLock();
            ServerSEIDMap[SEID].isBack = true;
            ServerSEIDMap[SEID].isSocketExit = false;
            ServerSEIDMap[SEID].releaseOtherValueLock();
            closesocket(ServerSEIDMap[SEID].socketH);
            ServerSEIDMap[SEID].releaseServerHealthySocketLock();
            return;
        }
        if (strcmp(buf.c_str(), sendMsg.c_str()) != 0 || ServerSEIDMap[SEID].isBack || state <= 0 || state1 <= 0)
        {
            ServerSEIDMap[SEID].getOtherValueLock();
            ServerSEIDMap[SEID].isBack = true;
            ServerSEIDMap[SEID].isSocketExit = false;
            ServerSEIDMap[SEID].releaseOtherValueLock();
            closesocket(ServerSEIDMap[SEID].socketH);
            ServerSEIDMap[SEID].releaseServerHealthySocketLock();
            return;
        }
        ServerSEIDMap[SEID].releaseServerHealthySocketLock();
        Sleep(1000);
    }
}
void HealthyCheckByClient(string SEID)
{
    int timeout = 3000; // 设置超时时间为 3 秒
    ClientSEIDMap[SEID].getServerHealthySocketLock();
    setsockopt(ClientSEIDMap[SEID].socketH, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(ClientSEIDMap[SEID].socketH, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    ClientSEIDMap[SEID].getServerHealthySocketLock();
    while (1)
    {
        srand(time(NULL));
        string sendMsg = to_string(rand() % 1000000000);
        string buf;
        ClientSEIDMap[SEID].getServerHealthySocketLock();
        int state1 = send_message(ClientSEIDMap[SEID].socketH, sendMsg);
        int state2 = receive_message(ClientSEIDMap[SEID].socketH, buf);
        if (strcmp(buf.c_str(), sendMsg.c_str()) != 0 || ClientSEIDMap[SEID].isBack || state2 <= 0 || state1 <= 0)
        {
            ClientSEIDMap[SEID].getOtherValueLock();
            ClientSEIDMap[SEID].isBack = true;
            ClientSEIDMap[SEID].isSocketExit = false;
            ClientSEIDMap[SEID].releaseOtherValueLock();
            closesocket(ClientSEIDMap[SEID].socketH);
            cout << "unhelathy";
            ServerSEIDMap[SEID].releaseServerHealthySocketLock();
            return;
        }
        ClientSEIDMap[SEID].getServerHealthySocketLock();
    }
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
            return -1;
        if (!startIf || ClientMap[i - 1].state == state || f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)))
        {
            canShowClient++;
            string sendBuf = ClientMap[i - 1].ClientWanIp + " " + ClientMap[i - 1].ClientLanIp + " " + to_string(ClientMap[i - 1].ClientConnectPort) + " " + to_string(ClientMap[i - 1].state);
            ServerSEIDMap[seid].getServerSocketLock();
            send_message(s, sendBuf);
            ServerSEIDMap[seid].releaseServerSocketLock();
        }
    }
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁
    ServerSEIDMap[seid].getServerSocketLock();
    send_message(s, "\r\n\r\nend\r\n\r\n");
    ServerSEIDMap[seid].releaseServerSocketLock();
    return canShowClient;
}
void Connect(string seid, vector<string> cmods, int cmodsNum)
{
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    while (1)
    {
        recvBuf.clear();
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            break;
        ServerSEIDMap[seid].releaseOtherValueLock();
        filter temp;
        showForSend(seid, temp, true, ClientSocketFlagStruct::Online);
        ServerSEIDMap[seid].getServerSocketLock();
        int state = receive_message(s, recvBuf);
        ServerSEIDMap[seid].releaseServerSocketLock();
        if (state == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            break;
        }
        else if (state > 0)
        {
            if (strcmp(recvBuf.c_str(), "\r\nexit\r\n") == 0)
            {
                break;
            }
            else if (strcmp(recvBuf.c_str(), "\r\nnext\r\n") == 0)
            {
                continue;
            }
            int setClientId = atoi(recvBuf.c_str());
            cout << "setClientId:" << setClientId << endl;
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
                ServerSEIDMap[seid].getServerSocketLock();
                ClientSEIDMap[ClientMap[ClientIndex].SEID].getServerSocketLock();
                send_message(s, "\r\n\r\nsec\r\n\r\n");
                send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "\r\nstart\r\n");
                receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2);
                send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "\r\n");
                receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, temp);
                buf2 += temp;
                send_message(s, buf2);
                // receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, temp);
                ServerSEIDMap[seid].releaseServerSocketLock();
                ClientSEIDMap[ClientMap[ClientIndex].SEID].releaseServerSocketLock();
                while (1)
                {
                    buf.clear();
                    buf2.clear();

                    ServerSEIDMap[seid].getServerSocketLock();
                    int state = receive_message(s, buf);
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    if (state == SOCKET_ERROR)
                    {
                        break;
                    }
                    else if (state > 0)
                    {
#ifdef _DEBUG
                        cout << "recv:" << buf << endl;
#endif
                        ClientSEIDMap[ClientMap[ClientIndex].SEID].getServerSocketLock();
                        if (strcmp(buf.c_str(), "\r\nfexit\r\n") == 0)
                        {
                            send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, "exit\r\n");
                            receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2);
                            ClientSEIDMap[ClientMap[ClientIndex].SEID].releaseServerSocketLock();
                            ClientMap[ClientIndex].state = ClientSocketFlagStruct::Online;
                            break;
                        }
                        else
                        {
                            send_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf.c_str());
                            receive_message(ClientSEIDMap[ClientMap[ClientIndex].SEID].ServerSocket, buf2);
                            send_message(s, buf2);
                        }
                        ClientSEIDMap[ClientMap[ClientIndex].SEID].releaseServerSocketLock();
                    }
                }
                // break;
            }
            else
            {
                ServerSEIDMap[seid].getServerSocketLock();
                send_message(s, "\r\n\r\nfail\r\n\r\n");
                ServerSEIDMap[seid].releaseServerSocketLock();
            }
        }
    }
}
int delForId(int ClientId)
{
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    SOCKET &ClientSocket = ClientSEIDMap[ClientMap[ClientId - 1].SEID].ServerSocket;

    if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state != ClientSocketFlagStruct::Use)
    {
        if (send_message(ClientSocket, "del") != SOCKET_ERROR)
        {
            ClientMapLock.exchange(false, std::memory_order_release); // 解锁
            return 0;
        }
        else
        {
            ClientMapLock.exchange(false, std::memory_order_release); // 解锁
            return 1;
        }
    }
    else if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state == ClientSocketFlagStruct::Use)
    {
        ClientMapLock.exchange(false, std::memory_order_release); // 解锁
        return 2;
    }
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁
    return -1;
}
void del(string seid, vector<string> cmods, int cmodsNum)
{
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    while (1)
    {
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            break;
        ServerSEIDMap[seid].releaseOtherValueLock();
        filter temp;
        showForSend(seid, temp);
        ServerSEIDMap[seid].getServerSocketLock();
        int state = receive_message(s, recvBuf);
        ServerSEIDMap[seid].releaseServerSocketLock();
        if (state == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            break;
        }
        else if (state > 0)
        {
            if (strcmp(recvBuf.c_str(), "\r\nexit\r\n") == 0)
            {
                break;
            }
            else if (strcmp(recvBuf.c_str(), "\r\nnext\r\n") == 0)
            {
                continue;
            }
            int setClientId = atoi(recvBuf.c_str());
            int res = delForId(setClientId);
            ServerSEIDMap[seid].getServerSocketLock();
            if (res == 0)
            {
                send_message(s, "\r\n\r\nok\r\n\r\n");
            }
            else if (res == 2)
            {
                send_message(s, "\r\n\r\nUse\r\n\r\n");
            }
            else
            {
                send_message(s, "\r\n\r\nUnError\r\n\r\n");
            }
            ServerSEIDMap[seid].releaseServerSocketLock();
        }
    }
}
void show(string seid, vector<string> cmods, int cmodsNum)
{
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    string recvBuf;
    while (1)
    {
        filter temp;
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            break;
        ServerSEIDMap[seid].releaseOtherValueLock();
        showForSend(seid, temp);
        ServerSEIDMap[seid].getServerSocketLock();
        int state = receive_message(s, recvBuf);
        ServerSEIDMap[seid].releaseServerSocketLock();
        if (state == SOCKET_ERROR)
        {
            return;
        }
        else if (strcmp(recvBuf.c_str(), "\r\nclose\r\n") == 0)
        {
            return;
        }
    }
}
void cmod(string seid, vector<string> cmods, int cmodsNum)
{
    //[del,cmd,show][all,]
    SOCKET &s = ServerSEIDMap[seid].ServerSocket;
    map<string, int> StringToIntInComd = {
        {"run", 1},
        {"show", 2},
        {"del", 3},
        {"cmod", 4},

        {"port", 5},
        {"wanip", 6},
        {"lanip", 7},
        {"all", 8}};
    filter f;
    if (strcmp(cmods[1].c_str(), "del") == 0 && cmodsNum >= 3)
    {
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            return;
        ServerSEIDMap[seid].releaseOtherValueLock();
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\ndel\r\n");
        ServerSEIDMap[seid].releaseServerSocketLock();
        for (int i = 2; i <= cmodsNum; i += 3)
        {
            ServerSEIDMap[seid].getOtherValueLock();
            if (ServerSEIDMap[seid].isBack)
                return;
            ServerSEIDMap[seid].releaseOtherValueLock();
            ServerSEIDMap[seid].getServerSocketLock();
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            }
            ServerSEIDMap[seid].releaseServerSocketLock();
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
            return;
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\nok\r\n");
        send_message(s, to_string(sectClient).c_str());
        send_message(s, to_string(ClientMap.size()).c_str());
        ServerSEIDMap[seid].releaseServerSocketLock();
    }
    else if (strcmp(cmods[1].c_str(), "cmd") == 0 && cmodsNum >= 4)
    {
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            return;
        ServerSEIDMap[seid].releaseOtherValueLock();
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\ncmd\r\n");
        ServerSEIDMap[seid].releaseServerSocketLock();
        int cmdstart = 0;
        // 一条指令
        // cmd [port [!=,==,>,<,>=,<=] [port]] [wanip [!=,==] [ip]] [lanip [!=,==] [ip]] [all] "指令"
        for (int i = 2; i <= cmodsNum && cmods[i][0] != '"'; i += 3, cmdstart = i)
        {
            ServerSEIDMap[seid].getOtherValueLock();
            if (ServerSEIDMap[seid].isBack)
                return;
            ServerSEIDMap[seid].releaseOtherValueLock();
            if (cmods[i][0] == '"')
            {
                break;
            }
            ServerSEIDMap[seid].getServerSocketLock();
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            }
            ServerSEIDMap[seid].releaseServerSocketLock();
        }
        if (cmdstart > cmodsNum) // 判断指令格式是否标准
        {
            ServerSEIDMap[seid].getOtherValueLock();
            if (ServerSEIDMap[seid].isBack)
                return;
            ServerSEIDMap[seid].releaseOtherValueLock();
            ServerSEIDMap[seid].getServerSocketLock();
            send_message(s, "\r\ncmd error\r\n");
            ServerSEIDMap[seid].releaseServerSocketLock();
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
        ClientMapLock.exchange(true, std::memory_order_acquire); // 加锁
        for (int i = 1; i <= ClientMap.size(); i++)
        {
            if (f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)) && ClientMap[i - 1].state != ClientSocketFlagStruct::Use)
            {
                sectClient++;
                send_message(ClientSEIDMap[ClientMap[i - 1].SEID].ServerSocket, sendBuf); // 发送指令至Client
            }
        }
        ClientMapLock.exchange(false, std::memory_order_release); // 解锁
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            return;
        ServerSEIDMap[seid].releaseOtherValueLock();
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\nok\r\n");
        send_message(s, to_string(sectClient).c_str());
        send_message(s, to_string(ClientMap.size()).c_str());
        ServerSEIDMap[seid].releaseServerSocketLock();
    }
    else if (strcmp(cmods[1].c_str(), "show") == 0 && cmodsNum >= 3)
    {
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            return;
        ServerSEIDMap[seid].releaseOtherValueLock();
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\nsee\r\n");
        ServerSEIDMap[seid].releaseServerSocketLock();
        for (int i = 2; i <= cmodsNum && (cmodsNum - i) >= 2; i += 3)
        {
            ServerSEIDMap[seid].getServerSocketLock();
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, NULL, NULL))
                {
                    send_message(s, "\r\ncmd error\r\n");
                    ServerSEIDMap[seid].releaseServerSocketLock();
                    return;
                }
                break;
            default:
                send_message(s, "\r\ncmd error\r\n");
                return;
            }
            ServerSEIDMap[seid].releaseServerSocketLock();
        }
        int showClient = showForSend(seid, f);
        ServerSEIDMap[seid].getOtherValueLock();
        if (ServerSEIDMap[seid].isBack)
            return;
        ServerSEIDMap[seid].releaseOtherValueLock();
        ServerSEIDMap[seid].getServerSocketLock();
        send_message(s, "\r\nok\r\n");
        send_message(s, to_string(showClient).c_str());
        send_message(s, to_string(ClientMap.size()).c_str());
        ServerSEIDMap[seid].releaseServerSocketLock();
    }
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
        while (ClientMap[distance(ClientMap.begin(), itr)].state == ClientSocketFlagStruct::states::Use)
            ;
        closesocket(ClientSEIDMap[ClientMap[distance(ClientMap.begin(), itr)].SEID].ServerSocket);
        ClientMap[distance(ClientMap.begin(), itr)] = temp;
    }
    else
    {
        ClientMap.push_back(temp);
    }
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁
    return;
}
void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
{
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);             // 获取缓冲区句柄
    SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); // 设置文本及背景色
}
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
        receive_message(s, recvBuf);
        loginYZM = m.encode(StringTime(time(NULL)) + lastloginYZM + password);
        if (recvBuf == loginYZM)
        {
            loginTRUE = true;
            send_message(s, "true");
            break;
        }
        send_message(s, "error");
        lastloginYZM = loginYZM;
    }
    if (!loginTRUE)
        return;
    string SEID = createSEID(s, lastloginYZM + loginYZM);
    ServerSEIDMap[SEID].getServerSocketLock();
    send_message(s, SEID.c_str());
    ServerSEIDMap[SEID].releaseServerSocketLock();
    ServerSEIDMap[SEID].getOtherValueLock();
    ServerSEIDMap[SEID].isSEIDExit = true;
    ServerSEIDMap[SEID].SEID = SEID;
    ServerSEIDMap[SEID].ServerSocket = s;
    ServerSEIDMap[SEID].releaseOtherValueLock();
    while (true)
    {
        ServerSEIDMap[SEID].getOtherValueLock();
        if (ServerSEIDMap[SEID].isSocketExit)
        {
            ServerSEIDMap[SEID].releaseOtherValueLock();
            break;
        }
        ServerSEIDMap[SEID].releaseOtherValueLock();
    }
    try
    {
        thread HealthyCheckThread = thread(HealthyCheckByServer, SEID);
        HealthyCheckThread.detach();
    }
    catch (std::exception &e)
    {
        SetColor(4, 0);
        cout << e.what() << endl;
        SetColor(15, 0);
    }

    while (1)
    {
        ServerSEIDMap[SEID].getOtherValueLock();
        if (ServerSEIDMap[(string)SEID].isBack)
        {
            return;
        }
        ServerSEIDMap[SEID].releaseOtherValueLock();
        string recvBuf;
        ServerSEIDMap[SEID].getServerSocketLock();
        int state = receive_message(s, recvBuf);

        ServerSEIDMap[SEID].releaseServerSocketLock();
        if (state == SOCKET_ERROR || recvBuf.find("\r\nClose\r\n") != string::npos)
        {
            ServerSEIDMap[SEID].getServerSocketLock();
            ServerSEIDMap[SEID].getOtherValueLock();
            ServerSEIDMap[(string)SEID].isBack = true;
            closesocket(s);
            ServerSEIDMap[SEID].releaseOtherValueLock();
            ServerSEIDMap[SEID].releaseServerSocketLock();
            return;
        }
        else
        {
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
                Connect(SEID, cmods, tokenNum); // ok
                break;
            case 2:
                del(SEID, cmods, tokenNum); // ok
                break;
            case 3:
                show(SEID, cmods, tokenNum); // ok
                break;
            case 4:
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
    receive_message(s, recvBuf);
    istringstream iss((string)recvBuf);
    string ClientWanIp, ClientLanIp, ClientPort, ClientState;
    iss >> ClientWanIp >> ClientLanIp >> ClientPort;
    string SEID = createSEID(s, ClientLanIp + ClientWanIp);
    cout << "Client SEID:" << SEID << endl;
    ClientSEIDMap[SEID].getOtherValueLock();
    ClientSEIDMap[SEID].ServerSocket = s;
    ClientSEIDMap[SEID].isSEIDExit = true;
    ClientSEIDMap[SEID].releaseOtherValueLock();
    send_message(s, SEID);
    joinClient(ClientWanIp, ClientLanIp, ClientPort, time(NULL), 0, SEID);
    while (true)
    {
        ClientSEIDMap[SEID].getOtherValueLock();
        if (ClientSEIDMap[SEID].isSocketExit)
        {
            ClientSEIDMap[SEID].releaseOtherValueLock();
            break;
        }
        ClientSEIDMap[SEID].releaseOtherValueLock();
    }
    thread HealthyCheckThread = thread(HealthyCheckByClient, SEID);
    while (1)
    {

        if (ServerSEIDMap[(string)SEID].isBack)
        {
            HealthyCheckThread.detach();
            closesocket(s);
            return;
        }
    }
}
void ServerConnect()
{
    while (true)
    {
        while (ServerQueueLock.exchange(true, std::memory_order_acquire))
            ;
        if (ServerSocketQueue.size() > 0)
        {
            SOCKET ServerSocket = ServerSocketQueue.front();
            ServerSocketQueue.pop();
            thread ServerRSThread = thread(ServerRS, ServerSocket);
            ServerRSThread.detach();
        }
        ServerQueueLock.exchange(false, std::memory_order_release);
    }
}
void ClientConnect()
{
    while (1)
    {
        while (ClientQueueLock.exchange(true, std::memory_order_acquire))
            ;
        if (ClientSocketQueue.size() > 0)
        {
            SOCKET ClientSocket = ClientSocketQueue.front();
            ClientSocketQueue.pop();
            ClientRSThreadArry.push_back(thread(ClientRS, ClientSocket));
        }
        ClientQueueLock.exchange(false, std::memory_order_release);
    }
}
int main(int argc, char **argv)
{
    system("chcp 65001>nul");
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    loadData();
    cout << "loadData OK\n";
    cout << "password:" << password << endl;
    initServer(ListenSocket, service, 6020);

    thread ClientConnectThread = thread(ClientConnect);
    thread ServerConnectThread = thread(ServerConnect);
    thread dataSaveThread = thread(dataSave);
    thread PassDataThread = thread(passData);
    cout << "server start";
    while (true)
    {
        string buf;
        SOCKET aptSocket;
        sockaddr_in aptsocketAddr = {0};
        int len = sizeof(aptsocketAddr);
        aptSocket = accept(ListenSocket, (SOCKADDR *)&aptsocketAddr, &len);
        if (aptSocket != INVALID_SOCKET)
        {
            receive_message(aptSocket, buf);
            if (strcmp(buf.c_str(), "Client") == 0)
            {

                send_message(aptSocket, "Recv");
                while (ClientQueueLock.exchange(true, std::memory_order_acquire))
                    ;
                ClientSocketQueue.push(aptSocket);
                ClientQueueLock.exchange(false, std::memory_order_release);
                cout << "Client Connect\n"; // aptSocket = INVALID_SOCKET;
            }
            else if (strcmp(buf.c_str(), "Server") == 0)
            {
                send_message(aptSocket, "Recv");
                while (ServerQueueLock.exchange(true, std::memory_order_acquire))
                    ;
                ServerSocketQueue.push(aptSocket);
                ServerQueueLock.exchange(false, std::memory_order_release);
                // aptSocket = INVALID_SOCKET;
            }
            else if (ServerSEIDMap[buf].isSEIDExit)
            {
                ServerSEIDMap[buf].getServerHealthySocketLock();
                ServerSEIDMap[buf].getOtherValueLock();
                ServerSEIDMap[buf].socketH = aptSocket;
                ServerSEIDMap[buf].isSocketExit = true;
                ServerSEIDMap[buf].releaseOtherValueLock();
                ServerSEIDMap[buf].releaseServerHealthySocketLock();
            }
            else if (ClientSEIDMap[buf].isSEIDExit)
            {
                ClientSEIDMap[buf].getServerHealthySocketLock();
                ClientSEIDMap[buf].getOtherValueLock();
                ClientSEIDMap[buf].socketH = aptSocket;
                ClientSEIDMap[buf].isSocketExit = true;
                ClientSEIDMap[buf].releaseOtherValueLock();
                ClientSEIDMap[buf].releaseServerHealthySocketLock();
            }
            cout << buf << endl;
        }
    }
    return 0;
}