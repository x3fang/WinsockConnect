
// 设置连接器选项
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
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
#include <string>
#include "plugin.cpp"
#include "..\\MD5.h"
#include "..\\hookClose.h"
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

SOCKET g_sock = INVALID_SOCKET;
sockaddr_in g_serverInfo = {0};
vector<string> g_getFilesPathList, g_getFilesNameList;
string g_SEID;
string serverIP, serverPort;
thread g_healthyThread;

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
int initClientSocket(SOCKET &sock, sockaddr_in &serverInfo, string serverIP, int serverPort)
{
    WSADATA wsaData;
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return WSAGetLastError();
    }
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return WSAGetLastError();
    }
    // Set address and port

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverInfo.sin_port = htons(serverPort);
    return 0;
}
int connectServer(SOCKET &sock, sockaddr_in &serverInfo)
{
    int iResult;
    // Connect to server
    iResult = connect(sock,
                      (struct sockaddr *)&serverInfo,
                      sizeof(serverInfo));
    if (iResult == SOCKET_ERROR)
    {
        printf("Error at connect(): %ld\n", WSAGetLastError());
        closesocket(g_sock);
        WSACleanup();
    }
}
int initPlugin()
{
    getFilesName("plugins", g_getFilesNameList);
    if (g_getFilesNameList.empty())
        return -1;
    for (auto pluginName : g_getFilesNameList)
    {
        string temp;
        try
        {
            temp = pluginName.substr(0, pluginName.find("."));
            temp = temp.substr(temp.find("\\"));
        }
        catch (...)
        {
            temp = pluginName;
        }
        g_getFilesNameList.push_back(temp);
    }

    return 0;
}
int init(string serverIP, int serverPort)
{
    // value define
    int iResult;

    // init plugin
    iResult = initPlugin();
    if (iResult != 0)
        return iResult;

    // init socket
    iResult = initClientSocket(g_sock, g_serverInfo, serverIP, serverPort);
    if (iResult != 0)
        return iResult;

    // connect server
    iResult = connectServer(g_sock, g_serverInfo);
    if (iResult != 0)
        return iResult;

    return 0;
}
int loadPlugin(vector<string> pluginList, string pluginType)
{
    int iResult = 0;
    for (auto pluginName : pluginList)
    {
        if (find(g_getFilesNameList.begin(),
                 g_getFilesNameList.end(),
                 pluginName) != g_getFilesNameList.end())
        {
            auto loadLibrary = LoadLibrary(("plugins\\" + pluginName + ".dll").c_str());
            if (loadLibrary == NULL)
            {
                iResult++;
            }
            else
            {
                auto statusFun = (startupFun_ptr)GetProcAddress(loadLibrary, "Startup");
                auto startFun = (startFun_ptr)GetProcAddress(loadLibrary, "Start");
                auto stopFun = (stopFun_ptr)GetProcAddress(loadLibrary, "Stop");
                auto runFun = (runFun_ptr)GetProcAddress(loadLibrary, "run");
                registerPlugin(pluginName, pluginType, statusFun, startFun, stopFun, runFun);
            }
        }
        else
        {
            iResult++;
        }
    }
    return iResult;
}
int pullOneClassPlugin(SOCKET &sock, vector<string> &pluginList)
{
    string message;
    do
    {
        receive_message(sock, message);
        if (message.find("\r\nend\r\n") != string::npos)
            break;
        pluginList.push_back(message);
    } while (true);
    return 0;
}
// necessity: every bit: 0:unnecessity 1:necessary
int pullPlugin(SOCKET &sock, string necessity, vector<string> pluginType)
{
    int loadFailedNum = 0;
    int iResult = 0;

    for (int i = 0; i < necessity.length(); i++)
    {
        vector<string> pluginListTemp;
        iResult = pullOneClassPlugin(sock, pluginListTemp);
        if (iResult != 0)
        {
            return -iResult;
        }
        if (necessity[i] == '0')
        {
            loadFailedNum += loadPlugin(pluginListTemp, pluginType[i]);
        }
        else
        {
            iResult = loadPlugin(pluginListTemp, pluginType[i]);
            if (iResult != 0)
            {
                return -iResult;
            }
        }
    }
    return loadFailedNum;
}
void healthyCheck(string SEID)
{
    SOCKET sock;
    sockaddr_in serverInfo;
    int iResult = initClientSocket(sock, serverInfo, serverIP, std::stoi(serverPort));
    if (iResult != 0)
        return;
    iResult = connectServer(sock, serverInfo);
    if (iResult != 0)
        return;
    send_message(sock, SEID);
}
int handshake(SOCKET &sock)
{
    // send id
    send_message(sock, "Server");

    // load plugin
    int iResultPlugin = pullPlugin(sock, "0111", {"fun", "srd", "sonline", "sRSstart"});
    if (iResultPlugin < 0)
    {
        return iResultPlugin;
    }
    else
    {
        return iResultPlugin;
    }

    // create healthy check thread
    receive_message(sock, g_SEID);
    g_healthyThread = thread(healthyCheck, g_SEID);
}
int main()
{
    cin >> serverIP >> serverPort;
    int iResult = init(serverIP, std::stoi(serverPort));
    if (iResult != 0)
    {
        cout << "init failed" << endl
             << "error code " << iResult
             << endl;
        system("pause");
        return iResult;
    }
    iResult = connectServer(g_sock, g_serverInfo);
    if (iResult != 0)
    {
        cout << "connect failed" << endl
             << "error code " << iResult
             << endl;
        system("pause");
        return iResult;
    }

    iResult = handshake(g_sock);
    if (iResult < 0)
    {
        cout << "handshake failed" << endl
             << "error code " << iResult
             << endl;
        system("pause");
        return iResult;
    }
}