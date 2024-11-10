#include "globaldll.h"
#include "saveDatadll.h"
#include "plugin.cpp"

int initServer(SOCKET &, sockaddr_in &, int);
string StringTime(time_t);
// void Connect(string, vector<string>, int);
// void del(string, vector<string>, int);
// void show(string, vector<string>, int);
// void cmod(string, vector<string>, int);
string createSEID(SOCKET, string);
void ServerRS(SOCKET);
void ClientRS(SOCKET);
void ServerConnect();
void ClientConnect();
void saveData();
void dataSave();
void passData();
void loadData();
map<string, int> StringToInt =
    {
        {"connect", 1},
        {"del", 2},
        {"show", 3},
        {"cmd", 4}

};
int initServer(SOCKET &ListenSocket, sockaddr_in &sockAddr, int port)
{
    allInfoStruct info("", NULL, "initServer");
    runPlugin(info, "eFstart");

    auto funlog = prlog.getFunLog("initServer");
    *funlog << "initServer Start" << lns::endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        *funlog << "WSAStartup() failed with error: " << iResult << lns::endl;
        *funlog << "initServer End" << lns::endl;
        runPlugin(info, "eFend");
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
        runPlugin(info, "eFend");
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
        runPlugin(info, "eFend");
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
        runPlugin(info, "eFend");
        return errorCode;
    }
    *funlog << "initServer End" << lns::endl;
    runPlugin(info, "eFend");
    return 0;
}
string StringTime(time_t t1)
{
    allInfoStruct info("", NULL, "StringTime");
    runPlugin(info, "eFstart");

    auto funlog = prlog.getFunLog("StringTime");
    time_t t = t1;
    char tmp[64];
    struct tm *timinfo;
    timinfo = localtime(&t);
    strftime(tmp, sizeof(tmp), "%Y%m%d%H%M", timinfo);
    *funlog << "String Time End" << lns::endl;
    runPlugin(info, "eFend");
    return tmp;
}
void setServerOrClientExit(SEIDForSocketStruct &s)
{
    allInfoStruct info("", NULL, "setServerOrClientExit");
    runPlugin(info, "eFstart");

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
    runPlugin(info, "eFend");
    return;
}
void healthyLock()
{
    allInfoStruct info("", NULL, "healthyLock");
    runPlugin(info, "eFstart");

    while (HealthyLock.exchange(true, std::memory_order_acquire))
        ; // 加锁)
    runPlugin(info, "eFend");
    return;
}
void healthyUnlock()
{
    allInfoStruct info("", NULL, "healthyUnlock");
    runPlugin(info, "eFstart");

    HealthyLock.exchange(false, std::memory_order_release); // 解锁
    runPlugin(info, "eFend");
    return;
}
void HealthyCheack()
{
    allInfoStruct info("", NULL, "HealthyCheack");
    runPlugin(info, "eFstart");

    auto funlog = prlog.getFunLog("HealthyCheack");
    *funlog << "HealthyCheack Start" << lns::endl;
    while (!closeServer)
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
                allInfoStruct info(temp->SEID, temp->socketH, "");
                info.msg = sendMsg;
                if (runPlugin(info, "hCsd"))
                    sendMsg = info.msg;
                int state = send_message(temp->socketH, sendMsg);
                int state1 = receive_message(temp->socketH, buf);
                info.msg = buf;
                if (runPlugin(info, "hCrd"))
                    buf = info.msg;
                if (runPlugin(info, "hCjudge") || buf != sendMsg || temp->isBack || !state || !state1)
                {
                    *funlog << (HealthyQueue.front().isServer ? "Server" : "Client")
                            << " HealthyCheck Status:Error,error code:"
                            << WSAGetLastError()
                            << " SEID:" << SEID << lns::endl;
                    HealthyQueue.pop();
                    setServerOrClientExit(*temp);
                    continue;
                }
                if (strcmp(buf.c_str(), "\r\nClose\r\n") == 0)
                {
                    *funlog << (HealthyQueue.front().isServer ? "Server" : "Client")
                            << " HealthyCheck Status:Close SEID:" << SEID << lns::endl;
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
    if (!HealthyQueue.empty())
    {
        healthyLock();
        for (int i = 0; i < HealthyQueue.size(); i++)
        {
            SOCKET s;
            SEIDForSocketStruct *temp;
            string SEID = HealthyQueue.front().SEID;

            if (HealthyQueue.front().isServer)
                temp = &ServerSEIDMap[SEID];
            else
                temp = &ClientSEIDMap[SEID];
            std::lock_guard<std::mutex> lock(temp->ServerHealthySocketLock);
            int state = send_message(temp->socketH, "\r\n\r\nClose\r\n\r\n");
        }
        healthyUnlock();
    }
    *funlog << "HealthyCheack End" << lns::endl;
    runPlugin(info, "eFend");
}
string createSEID(SOCKET sock, string something = NULL)
{
    allInfoStruct info("", NULL, "createSEID");
    runPlugin(info, "eFstart");

    MD5 m;
    m.init();
    string SEID = m.encode(StringTime(time(NULL)) + to_string(sock));
    info.msg = SEID;
    runPlugin(info, "cSEID");
    SEID = info.msg;
    runPlugin(info, "eFend");
    return SEID;
}
// void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
// {
//     HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);             // 获取缓冲区句柄
//     SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); // 设置文本及背景色
// }
void ServerRS(SOCKET s)
{
    allInfoStruct info("", s, "ServerRS");
    runPlugin(info, "eFstart");
    if (!runPlugin(info, "sRSstart"))
    {
        auto funlog = prlog.getFunLog("ServerRS");
        *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
    }
    // string lastloginYZM;
    // string loginYZM;
    // lastloginYZM.clear();
    // bool loginTRUE = false;
    // MD5 m;
    // for (int i = 1; i <= 5 && !loginTRUE; i++)
    // {
    //     m.init();
    //     string recvBuf;
    //     if (!receive_message(s, recvBuf))
    //     {
    //         return;
    //     }
    //     loginYZM = m.encode(StringTime(time(NULL)) + lastloginYZM + password);
    //     if (recvBuf == loginYZM)
    //     {
    //         loginTRUE = true;
    //         if (!send_message(s, "true"))
    //         {
    //             return;
    //         }
    //         break;
    //     }
    //     if (!send_message(s, "error"))
    //     {
    //         return;
    //     }
    //     lastloginYZM = recvBuf;
    // }
    // if (!loginTRUE)
    // {
    //     auto funlog = prlog.getFunLog("ServerRS");
    //     *funlog << "login error" << lns::endl;
    //     return;
    // }
    string SEID = createSEID(s, StringTime(time(NULL)));
    auto funlog = prlog.getFunLog("ServerRS");
    {
        std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
        if (!send_message(s, SEID.c_str()))
        {
            *funlog << "send to Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            runPlugin(info, "eFend");
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
    if (!runPlugin(info, "sonline"))
    {
        *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
    }

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

    while (!closeServer)
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
        for (auto *it = pluginList[13];
             it != nullptr && it->next != nullptr;
             it = it->next)
        {
            send_message(s, it->plugin->funName);
        }
        send_message(s, "\r\nend\r\n");
        string recvBuf;
        bool state = false;
        {
            std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
            state = receive_message(s, recvBuf);
        }
        *funlog << state << closeServer << recvBuf << lns::endl;
        if (closeServer)
            break;
        if (closeServer || !state || recvBuf.find("\r\nClose\r\n") != string::npos)
        {
            *funlog << "recv From Server error "
                    << "error code:" << WSAGetLastError() << lns::endl;
            std::lock_guard<std::mutex> lock(ServerSEIDMap[SEID].ServerSocketLock);
            std::lock_guard<std::mutex> lockO(ServerSEIDMap[SEID].OtherValueLock);
            ServerSEIDMap[(string)SEID].isBack = true;
            closesocket(s);
            *funlog << "Server exit" << lns::endl;
            *funlog << "ServerRS End" << lns::endl;
            runPlugin(info, "eFend");
            return;
        }
        else if (closeServer)
        {
            break;
        }
        else
        {
            info.msg = recvBuf;
            if (!runPlugin(info, "srd"))
            {
                *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
            }
            *funlog << "Recv:\n---------------------------------------------" << lns::endl
                    << string(recvBuf).substr(0, (recvBuf.length() > 60 ? recvBuf.length() / 2 : recvBuf.length() - 1))
                    << lns::endl
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
            bool findFun = false;
            cout << "cmods:" << cmods[0] << endl;
            for (auto *it = pluginList[13];
                 it != nullptr && it->next != nullptr;
                 it = it->next)
            {
                if (it->plugin->funName == cmods[0])
                {
                    findFun = true;
                    allInfoStruct info(SEID, s, cmods);
                    if (!it->plugin->runFun(info))
                    {
                        *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
                    }
                    break;
                }
            }
            if (!findFun)
            {
                if (!send_message(s, "\r\nnFind\r\n"))
                {
                    *funlog << "send to Server error "
                            << "error code:" << WSAGetLastError() << lns::endl;
                    runPlugin(info, "eFend");
                }
            }
            else
            {
                if (!send_message(s, "\r\nok\r\n"))
                {
                    *funlog << "send to Server error "
                            << "error code:" << WSAGetLastError() << lns::endl;
                    runPlugin(info, "eFend");
                }
            }
        }
    }
    closesocket(s);
    runPlugin(info, "eFend");
}
void ClientRS(SOCKET s)
{
    allInfoStruct info("", s, "ClientRS");
    runPlugin(info, "eFstart");

    auto funlog = prlog.getFunLog("ClientRS");
    if (!runPlugin(info, "cRSstart"))
    {
        *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
    }

    string recvBuf;
    if (!receive_message(s, recvBuf))
    {
        info.msg = "recv From Client error";
        if (!runPlugin(info, "cexit"))
        {
            *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
        }
        runPlugin(info, "eFend");
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

    if (!send_message(s, SEID))
    {
        *funlog << "send to Client error "
                << "error code:" << WSAGetLastError() << lns::endl;
        runPlugin(info, "eFend");
        return;
    }
    joinClient(ClientWanIp, ClientLanIp, ClientPort, time(NULL), 0, SEID);
    std::unique_lock<std::mutex> lock(ClientSEIDMap[SEID].isSocketExitLock);
    ClientSEIDMap[SEID].cv.wait(lock, [SEID]
                                { return ClientSEIDMap[SEID].isSocketExit; });
    *funlog << "Client online" << lns::endl;
    if (!runPlugin(info, "conline"))
    {
        *funlog << "runFun error for Server line:" << __LINE__ << lns::endl;
    }
    healthyLock();
    HealthyQueue.push({SEID, true});
    healthyUnlock();
    while (!closeServer)
    {
        if (ClientSEIDMap[(string)SEID].isBack || closeServer)
        {
            *funlog << "Client exit" << lns::endl;
            closesocket(s);
            runPlugin(info, "eFend");
            return;
        }
        Sleep(1000);
    }
    closesocket(s);
    runPlugin(info, "eFend");
}
void ServerConnect()
{
    allInfoStruct info("", NULL, "ServerConnect");
    runPlugin(info, "eFstart");

    while (!closeServer)
    {
        std::unique_lock<std::mutex> lock(ServerQueueLock);
        Queuecv.wait(lock, [&]
                     { return ServerSocketQueue.size() > 0; });
        for (; ServerSocketQueue.size() > 0 && !closeServer;)
        {
            SOCKET ServerSocket = ServerSocketQueue.front();
            ServerSocketQueue.pop();
            thread ServerRSThread = thread(ServerRS, ServerSocket);
            ServerRSThread.detach();
        }
    }
    runPlugin(info, "eFend");
}
void ClientConnect()
{
    allInfoStruct info("", NULL, "ClientConnect");
    runPlugin(info, "eFstart");

    while (!closeServer)
    {
        std::unique_lock<std::mutex> lock(ClientQueueLock);
        Queuecv.wait(lock, [&]
                     { return ClientSocketQueue.size() > 0; });
        for (; ClientSocketQueue.size() > 0 && !closeServer;)
        {
            SOCKET ClientSocket = ClientSocketQueue.front();
            ClientSocketQueue.pop();
            ClientRSThreadArry.push_back(thread(ClientRS, ClientSocket));
        }
    }
    runPlugin(info, "eFend");
}
extern "C" int EXPORT start()
{

    allInfoStruct info("", NULL, "start");
    runPlugin(info, "eFstart");

    char path[MAX_PATH] = {0};
    string temp;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    GetModuleFileName(NULL, path, MAX_PATH);
    temp = path;
    temp = temp.substr(temp.find_last_of("\\") + 1);
    temp = temp.substr(0, temp.find(".exe"));
#endif
#if __linux__
    temp = "none";
    temp = temp.substr(temp.find_last_of("/") + 1);
    temp = temp.substr(0, temp.find(".out"));
#endif
    fileName = temp;
    prlog.setName(fileName);
    prlog << "program start";
    prlog << lns::endl;
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
    while (!closeServer)
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
        if (closeServer)
            break;
        if (aptSocket != INVALID_SOCKET)
        {
            allInfoStruct info("", aptSocket);
            runPlugin(info, "as");
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
                cout << "ServerSEIDMap find" << endl;
                prlog << "Server Healthy Connect" << lns::endl;
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
    HealthyCheackThread.join();
    ClientConnectThread.join();
    ServerConnectThread.join();
    dataSaveThread.join();
    for (auto it = ClientRSThreadArry.begin(); it != ClientRSThreadArry.end(); it++)
        (*it).join();
    for (auto it = ServerRSThreadArry.begin(); it != ServerRSThreadArry.end(); it++)
        (*it).join();
    PassDataThread.join();
    prlog << "program end" << lns::endl;
    runPlugin(info, "eFend");
    return 0;
}
void stop()
{
    closeServer = true;
    closesocket(ListenSocket);
    prlog << "program stop" << lns::endl;
    return;
}