#pragma comment(lib, "ws2_32.lib")
// 设置连接器选项
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <thread>
#include <time.h>
#include <atomic>
#include <algorithm>
#include <conio.h>
#include <map>
#include <queue>
#include "..\\MD5.h"
#include "getCmd.h"
using std::atomic;
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::ifstream;
using std::ios;
using std::istringstream;
using std::map;
using std::ofstream;
using std::queue;
using std::string;
using std::thread;
using std::to_string;
using std::vector;
typedef struct SEIDForSocketStruct
{
    SOCKET socketH;
    bool isSocketExit;
    bool isSEIDExit;
    bool isBack;
    bool isUse;

    SEIDForSocketStruct()
    {
        socketH = INVALID_SOCKET;
        isSEIDExit = false;
        isSocketExit = false;
        isBack = false;
        isUse = false;
    }
};
typedef struct ClientSocketFlagStruct
{
    SOCKET ClientSocket;
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
class filter
{
private:
    bool notMatching;
    struct rule
    {

        string ruleData;
        int ruleOperatorType; //!= == > < >= <=
                              // 1  2 3 4 5  6
        rule(string _ruleData, int _ruleOperatorType)
        {
            ruleData = _ruleData;
            ruleOperatorType = _ruleOperatorType;
        }
    };
    vector<rule> wanIpRule, lanIpRule, portRule;
    map<string, int> StringToIntForruleOperator =
        {
            {"!=", 1},
            {"==", 2},
            {">", 3},
            {"<", 4},
            {">=", 5},
            {"<=", 6}};
    bool GetnotMatchingState()
    {
        return this->notMatching;
    }

public:
    enum ruleDataType
    {
        wanip = 1,
        lanip = 2,
        port = 3,
        all = 4
    };
    const bool matching(string wanIp, string lanIp, string port)
    {
        if (notMatching)
        {
            return true;
        }
        bool wanIpMatch = false;
        bool lanIpMatch = false;
        bool portMatch = false;

        if ((wanIpRule.size() == 0 && lanIpRule.size() == 0 && portRule.size() == 0)

            || (wanIp.empty() && lanIp.empty() && port.empty()))
        {
            return false;
        }
        // wanIp matching
        if (wanIpRule.size() > 0 && !wanIp.empty())
            for (int i = 1; i <= wanIpRule.size(); i++)
            {
                if (wanIp.find(wanIpRule[i].ruleData) != string::npos)
                {
                    if (wanIpRule[i].ruleOperatorType == 2)
                    {
                        wanIpMatch = true;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        else if (wanIpRule.size() > 0 && wanIp.empty())
            return false;
        else
            wanIpMatch = true;

        // lanIp matching
        if (lanIpRule.size() > 0 && !lanIp.empty())
            for (int i = 1; i <= lanIpRule.size(); i++)
            {
                if (lanIp.find(lanIpRule[i - 1].ruleData) != string::npos)
                {
                    if (lanIpRule[i].ruleOperatorType == 2)
                    {
                        lanIpMatch = true;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        else if (lanIpRule.size() > 0 && lanIp.empty())
            return false;
        else
            lanIpMatch = true;

        // port matching
        if (portRule.size() > 0 && !portRule.empty())
            for (int i = 1; i <= portRule.size(); i++)
            {
                int iPort = stoi(port);
                int iPortRule = stoi(portRule[i - 1].ruleData);
                switch (portRule[i - 1].ruleOperatorType)
                {
                case 1: //!=
                    if (port == portRule[i - 1].ruleData)
                        return false;
                    portMatch = true;
                    break;
                case 2: //==
                    if (port == portRule[i - 1].ruleData)
                        portMatch = true;
                    else
                        return false;
                    break;
                case 3: //>
                    if (iPort > iPortRule)
                        portMatch = true;
                    else
                        return false;
                    break;
                case 4: //<
                    if (iPort < iPortRule)
                        portMatch = true;
                    else
                        return false;
                case 5: //>=
                    if (iPort >= iPortRule)
                        portMatch = true;
                    else
                        return false;
                case 6: //<=
                    if (iPort <= iPortRule)
                        portMatch = true;
                    else
                        return false;
                }
            }
        else if (portRule.size() > 0 && portRule.empty())
            return false;
        else
            portMatch = true;
        return wanIpMatch && lanIpMatch && portMatch;
    }
    bool addRule(ruleDataType ruleDataType, string ruleOperatorType, string ruleData)
    {
        vector<rule> *ruleVector;
        switch (ruleDataType)
        {
        case wanip:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==")
            {
                ruleVector = &wanIpRule;
                break;
            }
            else
                return false;
        case lanip:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==")
            {
                ruleVector = &lanIpRule;
                break;
            }
            else
                return false;
        case port:
            if (ruleOperatorType == "!=" || ruleOperatorType == "==" || ruleOperatorType == ">" || ruleOperatorType == "<" || ruleOperatorType == ">=" || ruleOperatorType == "<=")
            {
                ruleVector = &portRule;
                break;
            }
            else
                return false;
        case all:
            notMatching = true;
            return true;
        default:
            return false;
        }
        ruleVector->push_back(rule{ruleData, StringToIntForruleOperator[ruleOperatorType]});
        ruleVector = NULL;
        return true;
    }
};
map<string, int> StringToInt =
    {
        {"connect", 1},
        {"del", 2},
        {"show", 3},
        {"comd", 4}

};
WSADATA wsaData;
SOCKET ListenSocket;
sockaddr_in service;
queue<SOCKET> ClientSocketQueue, ServerSocketQueue;
map<string, SEIDForSocketStruct> ServerSEIDMap, ClientSEIDMap;
vector<ClientSocketFlagStruct> DataSaveArry;
vector<ClientSocketFlagStruct> ClientMap;
string password;
bool dataIsChange = false;
vector<thread> ServerRSThreadArry, ClientRSThreadArry;
atomic<bool> ClientMapLock(false);
atomic<bool> ServerQueueLock(false), ClientQueueLock(false);
bool ischange = false;
int PassDataInit = 0;
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int initServer(SOCKET &, sockaddr_in &, int);
string StringTime(time_t);
void HealthyCheckByServer(string);
void HealthyCheckByClient(string);
int showForSend(SOCKET, filter, bool, ClientSocketFlagStruct::states);
void Connect(SOCKET, vector<string>, int);
int delForId(int);
void del(SOCKET, vector<string>, int);
void show(SOCKET, vector<string>, int);
void cmod(SOCKET, vector<string>, int);
string createSEID(SOCKET, string);
void joinClient(string, string, string, SOCKET, unsigned long long int, unsigned long long int);
void ServerRS(SOCKET);
void ClientRS(SOCKET);
void ServerConnect();
void ClientConnect();
void saveData();
void dataSave();
void passData();
void loadData();
int main(int, char **);

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
void HealthyCheckByServer(string SEID)
{
    int timeout = 10000; // 设置超时时间为 10 秒
    setsockopt(ServerSEIDMap[SEID].socketH, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof(timeout));
    setsockopt(ServerSEIDMap[SEID].socketH, SOL_SOCKET, SO_SNDTIMEO, (char *)timeout, sizeof(timeout));
    while (1)
    {
        srand(time(NULL));
        string sendMsg = to_string(rand() % 1000000000);
        char buf[128] = {0};
        int state = send(ServerSEIDMap[SEID].socketH, sendMsg.c_str(), sendMsg.length(), 0);
        int state1 = recv(ServerSEIDMap[SEID].socketH, buf, sizeof(buf), 0);
        if (strcmp(buf, sendMsg.c_str()) != 0 || ServerSEIDMap[SEID].isBack || state <= 0 || state1 <= 0)
        {
            ServerSEIDMap[SEID].isBack = true;
            ServerSEIDMap[SEID].isSocketExit = false;
            closesocket(ServerSEIDMap[SEID].socketH);
            return;
        }
        Sleep(1000);
    }
}
void HealthyCheckByClient(string SEID)
{
    int timeout = 10000; // 设置超时时间为 10 秒
    setsockopt(ClientSEIDMap[SEID].socketH, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof(timeout));
    setsockopt(ClientSEIDMap[SEID].socketH, SOL_SOCKET, SO_SNDTIMEO, (char *)timeout, sizeof(timeout));
    while (1)
    {
        srand(time(NULL));
        string sendMsg = to_string(rand() % 1000000000);
        char buf[128] = {0};
        int state1 = send(ClientSEIDMap[SEID].socketH, sendMsg.c_str(), sendMsg.length(), 0);
        int state2 = recv(ClientSEIDMap[SEID].socketH, buf, sizeof(buf), 0);
        if (strcmp(buf, sendMsg.c_str()) != 0 || ClientSEIDMap[SEID].isBack || state2 <= 0 || state1 <= 0)
        {
            ClientSEIDMap[SEID].isBack = true;
            ClientSEIDMap[SEID].isSocketExit = false;
            closesocket(ClientSEIDMap[SEID].socketH);
            cout << "unhelathy";
            return;
        }
    }
}
int showForSend(SOCKET s, filter f, bool startIf = false, ClientSocketFlagStruct::states state = ClientSocketFlagStruct::NULLs)
{
    int canShowClient = 0;
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    for (int i = 1; i <= ClientMap.size(); i++)
    {
        if (!startIf || ClientMap[i - 1].state == state || f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)))
        {
            canShowClient++;
            string sendBuf = ClientMap[i - 1].ClientWanIp + " " + ClientMap[i - 1].ClientLanIp + " " + to_string(ClientMap[i - 1].ClientConnectPort) + " " + to_string(ClientMap[i - 1].state);
            send(s, sendBuf.c_str(), sendBuf.length(), 0);
        }
    }
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁

    send(s, "\r\n\r\nend\r\n\r\n", strlen("\r\n\r\nend\r\n\r\n"), 0);
    return canShowClient;
}
void Connect(SOCKET s, vector<string> cmods, int cmodsNum)
{
    char recvBuf[1024] = {0};
    while (1)
    {
        filter temp;
        showForSend(s, temp, true, ClientSocketFlagStruct::Online);
        int state = recv(s, recvBuf, sizeof(recvBuf), 0);
        if (state == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            return;
        }
        else if (state > 0)
        {
            if (strcmp(recvBuf, "\r\nexit\r\n") == 0)
            {
                return;
            }
            else if (strcmp(recvBuf, "\r\nnext\r\n"))
            {
                continue;
            }
            int setClientId = atoi(recvBuf);
            SOCKET ClientSocket = INVALID_SOCKET;
            for (int i = 0; i < ClientMap.size(); i++)
            {
                if (ClientMap[i].state == ClientSocketFlagStruct::Online)
                {
                    setClientId--;
                    if (setClientId == 0)
                    {
                        ClientMap[i].state = ClientSocketFlagStruct::Use;
                        ClientSocket = ClientMap[i].ClientSocket;
                        break;
                    }
                }
            }

            if (ClientSocket != INVALID_SOCKET)
            {
                send(s, "\r\n\r\nsec\r\n\r\n", strlen("\r\n\r\nsec\r\n\r\n"), 0);
                while (1)
                {
                    char buf[1024] = {0};
                    int state = recv(s, buf, sizeof(buf), 0);
                    if (state == SOCKET_ERROR)
                    {
                        break;
                    }
                    else if (state > 0)
                    {
                        if (strcmp(buf, "\r\nfexit\r\n") == 0)
                        {
                            send(ClientSocket, "exit", strlen("exit"), 0);
                            break;
                        }
                        else
                        {
                            send(ClientSocket, buf, strlen(buf), 0);
                        }
                    }
                }
                break;
            }
            else
            {
                send(s, "\r\n\r\nfail\r\n\r\n", strlen("\r\n\r\nfail\r\n\r\n"), 0);
            }
        }
    }
}
int delForId(int ClientId)
{
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    SOCKET ClientSocket = ClientMap[ClientId - 1].ClientSocket;
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁
    if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state != ClientSocketFlagStruct::Use)
    {
        if (send(ClientSocket, "del", strlen("del"), 0) != SOCKET_ERROR)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else if (ClientSocket != INVALID_SOCKET && ClientMap[ClientId - 1].state == ClientSocketFlagStruct::Use)
    {
        return 2;
    }
    return -1;
}
void del(SOCKET s, vector<string> cmods, int cmodsNum)
{
    char recvBuf[1024] = {0};
    while (1)
    {
        filter temp;
        showForSend(s, temp);
        int state = recv(s, recvBuf, sizeof(recvBuf), 0);
        if (state == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            return;
        }
        else if (state > 0)
        {
            if (strcmp(recvBuf, "\r\nexit\r\n") == 0)
            {
                return;
            }
            else if (strcmp(recvBuf, "\r\nnext\r\n") == 0)
            {
                continue;
            }
            int setClientId = atoi(recvBuf);
            int res = delForId(setClientId);
            if (res == 0)
            {
                send(s, "\r\n\r\nok\r\n\r\n", strlen("\r\n\r\nok\r\n\r\n"), 0);
            }
            else if (res == 2)
            {
                send(s, "\r\n\r\nUse\r\n\r\n", strlen("\r\n\r\nUse\r\n\r\n"), 0);
            }
            else
            {
                send(s, "\r\n\r\nUnError\r\n\r\n", strlen("\r\n\r\nUnError\r\n\r\n"), 0);
            }
        }
    }
}
void show(SOCKET s, vector<string> cmods, int cmodsNum)
{
    cout << "SHOW";
    char recvBuf[1024] = {0};
    while (1)
    {
        filter temp;
        showForSend(s, temp);
        int state = recv(s, recvBuf, sizeof(recvBuf), 0);
        if (state == SOCKET_ERROR)
        {
            return;
        }
        else if (strcmp(recvBuf, "\r\nclose\r\n") == 0)
        {
            return;
        }
    }
}
void cmod(SOCKET s, vector<string> cmods, int cmodsNum)
{
    //[del,cmd,show][all,]

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
        send(s, "\r\ndel\r\n", strlen("\r\ndel\r\n"), 0);
        for (int i = 2; i <= cmodsNum; i += 3)
        {
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
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
            send(s, "\r\nok\r\n", strlen("\r\nok\r\n"), 0);
            send(s, to_string(sectClient).c_str(), to_string(sectClient).length(), 0);
            send(s, to_string(ClientMap.size()).c_str(), to_string(ClientMap.size()).length(), 0);
        }
    }
    else if (strcmp(cmods[1].c_str(), "cmd") == 0 && cmodsNum >= 4)
    {
        send(s, "\r\ncmd\r\n", strlen("\r\ncmd\r\n"), 0);
        int cmdstart = 0;
        // 一条指令
        // cmd [port [!=,==,>,<,>=,<=] [port]] [wanip [!=,==] [ip]] [lanip [!=,==] [ip]] [all] "指令"
        for (int i = 2; i <= cmodsNum && cmods[i][0] != '"'; i += 3, cmdstart = i)
        {
            if (cmods[i][0] == '"')
            {
                break;
            }
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            }
        }
        if (cmdstart > cmodsNum) // 判断指令格式是否标准
        {
            send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
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
        for (int i = 1; i <= ClientMap.size(); i++)
        {
            if (f.matching(ClientMap[i - 1].ClientWanIp, ClientMap[i - 1].ClientLanIp, to_string(ClientMap[i - 1].ClientConnectPort)) && ClientMap[i - 1].state != ClientSocketFlagStruct::Use)
            {
                sectClient++;
                send(ClientMap[i - 1].ClientSocket, sendBuf.c_str(), sendBuf.length(), 0); // 发送指令至Client
            }
        }
        send(s, "\r\nok\r\n", strlen("\r\nok\r\n"), 0);
        send(s, to_string(sectClient).c_str(), to_string(sectClient).length(), 0);
        send(s, to_string(ClientMap.size()).c_str(), to_string(ClientMap.size()).length(), 0);
    }
    else if (strcmp(cmods[1].c_str(), "show") == 0 && cmodsNum >= 3)
    {
        send(s, "\r\nshow\r\n", strlen("\r\nshow\r\n"), 0);
        for (int i = 2; i <= cmodsNum; i += 3)
        {
            switch (StringToIntInComd[cmods[i]])
            {
            case 5:
                if (!f.addRule(filter::ruleDataType::port, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 6:
                if (!f.addRule(filter::ruleDataType::wanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 7:
                if (!f.addRule(filter::ruleDataType::lanip, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            case 8:
                if (!f.addRule(filter::ruleDataType::all, cmods[i + 1], cmods[i + 2]))
                {
                    send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                    return;
                }
                break;
            default:
                send(s, "\r\ncmd error\r\n", strlen("\r\ncmd error\r\n"), 0);
                return;
            }
        }
        int showClient = showForSend(s, f);
        send(s, "\r\nok\r\n", strlen("\r\nok\r\n"), 0);
        send(s, to_string(showClient).c_str(), to_string(showClient).length(), 0);
        send(s, to_string(ClientMap.size()).c_str(), to_string(ClientMap.size()).length(), 0);
    }
}
string createSEID(SOCKET sock, string something = NULL)
{
    MD5 m;
    m.init();
    return m.encode(StringTime(time(NULL)) + to_string(sock));
}
void joinClient(string ClientWanIp, string ClientLanIp, string ClientPort, SOCKET ClientSocket, unsigned long long int OnlineTime, unsigned long long int OfflineTime)
{
    while (ClientMapLock.exchange(true, std::memory_order_acquire))
        ; // 加锁
    ClientSocketFlagStruct temp;
    temp.ClientSocket = ClientSocket;
    temp.ClientWanIp = ClientWanIp;
    temp.ClientLanIp = ClientLanIp;
    temp.ClientConnectPort = atoi(ClientPort.c_str());
    temp.OnlineTime = OnlineTime;
    temp.OfflineTime = OfflineTime;
    temp.state = ClientSocketFlagStruct::states::Online;
    vector<ClientSocketFlagStruct>::iterator itr = find(ClientMap.begin(), ClientMap.end(), temp);
    if (itr != ClientMap.end())
    {
        while (ClientMap[distance(ClientMap.begin(), itr)].state == ClientSocketFlagStruct::states::Use)
            ;
        closesocket(ClientMap[distance(ClientMap.begin(), itr)].ClientSocket);
        ClientMap[distance(ClientMap.begin(), itr)] = temp;
    }
    else
    {
        ClientMap.push_back(temp);
    }
    ClientMapLock.exchange(false, std::memory_order_release); // 解锁
    return;
}
bool send_message(SOCKET sock, const std::string &message)
{
    std::ostringstream oss;
    oss << message.size() << "\r\n"
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
    char buffer[1024];
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
        cout << password << endl;
        receive_message(s, recvBuf);
        loginYZM = m.encode(StringTime(time(NULL)) + lastloginYZM + password);
        cout << loginYZM << endl
             << recvBuf << endl;
        if (recvBuf == loginYZM)
        {
            loginTRUE = true;
            send_message(s, "true");
            break;
        }
        send_message(s, "fail");
        lastloginYZM = loginYZM;
    }
    if (!loginTRUE)
        return;
    string SEID = createSEID(s, lastloginYZM + loginYZM);
    send_message(s, SEID.c_str());
    ServerSEIDMap[SEID].isSEIDExit = true;
    while (!ServerSEIDMap[(string)SEID].isSocketExit)
        ;
    thread HealthyCheckThread = thread(HealthyCheckByServer, SEID);
    while (1)
    {
        if (ServerSEIDMap[(string)SEID].isBack)
        {
            return;
        }
        char recvBuf[8192] = {0};
        int state = recv(s, recvBuf, sizeof(recvBuf), 0);
        if (state == SOCKET_ERROR)
        {
            ServerSEIDMap[(string)SEID].isBack = true;
            closesocket(s);
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
            cout << tokenNum << endl;
            cout << cmods[0] << endl;
            cout << StringToInt[cmods[0]];
            switch (StringToInt[cmods[0]])
            {
            case 1:
                Connect(s, cmods, tokenNum); // ok
                break;
            case 2:
                del(s, cmods, tokenNum); // ok
                break;
            case 3:
                show(s, cmods, tokenNum); // ok
                break;
            case 4:
                cmod(s, cmods, tokenNum); // ok
                break;
            default:
                break;
            }
        }
    }
}
void ClientRS(SOCKET s)
{
    cout << "ok\n";
    char recvBuf[8192] = {0};
    recv(s, recvBuf, sizeof(recvBuf), 0);
    istringstream iss((string)recvBuf);
    string ClientWanIp, ClientLanIp, ClientPort, ClientState;
    iss >> ClientWanIp >> ClientLanIp >> ClientPort;
    joinClient(ClientWanIp, ClientLanIp, ClientPort, s, time(NULL), 0);
    string SEID = createSEID(s, ClientLanIp + ClientWanIp);
    ClientSEIDMap[SEID].isSEIDExit = true;
    send(s, SEID.c_str(), SEID.length(), 0);
    cout << "ok\n";
    while (!ClientSEIDMap[(string)SEID].isSocketExit)
        ;
    cout << "ok\n";
    thread HealthyCheckThread = thread(HealthyCheckByClient, SEID);
    while (1)
    {
        if (ServerSEIDMap[(string)SEID].isBack)
        {
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
            cout << "32423543";
            SOCKET ServerSocket = ServerSocketQueue.front();
            ServerSocketQueue.pop();
            ServerRSThreadArry.push_back(thread(ServerRS, ServerSocket));
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
            cout << "oj\n";
            SOCKET ClientSocket = ClientSocketQueue.front();
            ClientSocketQueue.pop();
            ClientRSThreadArry.push_back(thread(ClientRS, ClientSocket));
        }
        ClientQueueLock.exchange(false, std::memory_order_release);
    }
}
void saveData()
{
    ofstream outss;
    outss.open("clientData.txt", ios::out);
    for (int i = 1; i <= DataSaveArry.size(); i++)
    {
        outss << DataSaveArry[i - 1].ClientWanIp
              << " " << DataSaveArry[i - 1].ClientLanIp
              << " " << DataSaveArry[i - 1].ClientConnectPort
              << " " << to_string(DataSaveArry[i - 1].OnlineTime)
              << " " << to_string(DataSaveArry[i - 1].OfflineTime)
              << endl;
    }
    outss.close();
}
void dataSave()
{
    while (1)
    {
        if (dataIsChange)
        {
            saveData();
        }
        Sleep(1000);
    }
}
void passData()
{

    while (1)
    {
        if (!PassDataInit)
        {
            while (ClientMapLock.exchange(true, std::memory_order_acquire))
                ;
            for (int i = 1; i <= ClientMap.size(); i++)
            {
                if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
                {
                    delForId(i);
                }
                else
                    DataSaveArry.push_back(ClientMap[i - 1]);
            }
            ClientMapLock.exchange(false, std::memory_order_release);
            Sleep(1000);
            PassDataInit = true;
            continue;
        }
        while (ClientMapLock.exchange(true, std::memory_order_acquire))
            ;
        for (int i = 1; i <= ClientMap.size(); i++)
        {
            if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
            {
                delForId(i);
            }
            else
            {
                vector<ClientSocketFlagStruct>::iterator itr = find(DataSaveArry.begin(), DataSaveArry.end(), ClientMap[i - 1]);
                if (itr != DataSaveArry.end())
                {
                    DataSaveArry[distance(DataSaveArry.begin(), itr)] = ClientMap[i - 1];
                }
            }
        }
        ClientMapLock.exchange(false, std::memory_order_release);
    }
}
void loadData()
{
    ifstream inss, inPassword;
    inPassword.open("password.data", ios::in);
    if (!inPassword)
    {
        ofstream out;
        out.open("password.data", ios::out);
        out.close();
        inPassword.close();
        exit(0);
    }
    else
    {
        inPassword >> password;
    }
    inPassword.close();
    inss.open("clientData.data", ios::in);
    if (!inss)
    {
        ofstream out;
        out.open("clientData.data", ios::out);
        out.close();
        inss.close();
        return;
    }
    string wanip, lanip, port, OnlineTime, OfflineTime;
    while (inss >> wanip >> lanip >> port >> OnlineTime >> OfflineTime)
    {
        joinClient(wanip, lanip, port, NULL, stoi(OnlineTime), stoi(OfflineTime));
    }
    inss.close();
}
int main(int argc, char **argv)
{
    system("chcp 65001>nul");
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    loadData();
    cout << "loadData OK";
    initServer(ListenSocket, service, 2060);

    thread ClientConnectThread = thread(ClientConnect);
    thread ServerConnectThread = thread(ServerConnect);
    thread dataSaveThread = thread(dataSave);
    thread PassDataThread = thread(passData);
    while (true)
    {
        char cbuf[1024] = {0};
        SOCKET aptSocket;
        sockaddr_in aptsocketAddr = {0};
        int len = sizeof(aptsocketAddr);
        aptSocket = accept(ListenSocket, (SOCKADDR *)&aptsocketAddr, &len);
        if (aptSocket != INVALID_SOCKET)
        {
            recv(aptSocket, cbuf, 512, 0);
            if (strcmp(cbuf, "Client") == 0)
            {
                cout << "OKcc";
                send(aptSocket, "Recv", strlen("Recv"), 0);
                while (ClientQueueLock.exchange(true, std::memory_order_acquire))
                    ;
                ClientSocketQueue.push(aptSocket);
                ClientQueueLock.exchange(false, std::memory_order_release);
                // aptSocket = INVALID_SOCKET;
            }
            else if (strcmp(cbuf, "Server") == 0)
            {
                cout << "OKss";
                send(aptSocket, "Recv", strlen("Recv"), 0);
                while (ServerQueueLock.exchange(true, std::memory_order_acquire))
                    ;
                ServerSocketQueue.push(aptSocket);
                ServerQueueLock.exchange(false, std::memory_order_release);
                // aptSocket = INVALID_SOCKET;
            }
            else if (ServerSEIDMap[(string)cbuf].isSEIDExit)
            {
                ServerSEIDMap[(string)cbuf].socketH = aptSocket;
                ServerSEIDMap[(string)cbuf].isSocketExit = true;
            }
            else if (ClientSEIDMap[(string)cbuf].isSEIDExit)
            {
                ClientSEIDMap[(string)cbuf].socketH = aptSocket;
                ClientSEIDMap[(string)cbuf].isSocketExit = true;
            }
        }
    }
    return 0;
}