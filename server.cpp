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
#include <string>
#include "..\\MD5.h"
#include "hookClose.h"
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

CloseCheak closeCheak;
bool closeP = false;
string serverIp;
int serverPort;
string password;
SOCKET sockC, healthyBeat;
sockaddr_in sockAddr, healthyBeatAddr;
string SEID;
bool ServerState = false;
std::atomic<bool> ServerHealthCheck(false);
void WhenClose();
void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
{
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);             // 获取缓冲区句柄
    SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); // 设置文本及背景色
}
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
    // cout << "data_length:" << data_length << endl;
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
int sendClientList(SOCKET s)
{
    int clientNum = 0;
    cout << "__________Clients List__________" << endl;
    cout << "WanIp\tLanIP\tClient Connect Port\tClient Satte\n";
    do
    {
        string recvBuf;
        receive_message(sockC, recvBuf);
        if (strcmp(string(recvBuf).c_str(), "\r\n\r\nend\r\n\r\n") == 0)
        {

            break;
        }
        clientNum++;
        string srecv = recvBuf;
        istringstream iss(srecv);
        string clientWanIp, clientLanIp, clientPort, clientState;
        cout << clientNum;
        iss >> clientWanIp >> clientLanIp >> clientPort >> clientState;
        switch (stoi(clientState))
        {
        case 0:
            cout << "\t" << clientWanIp << "\t" << clientLanIp << "\t" << clientPort << "\t" << "UnKnown" << endl;
            break;
        case 1:
            cout << "\t" << clientWanIp << "\t" << clientLanIp << "\t" << clientPort << "\t" << "Online" << endl;
            break;
        case 2:
            cout << "\t" << clientWanIp << "\t" << clientLanIp << "\t" << clientPort << "\t" << "Offline" << endl;
            break;
        case 3:
            cout << "\t" << clientWanIp << "\t" << clientLanIp << "\t" << clientPort << "\t" << "Use" << endl;
            break;
        }
    } while (1);
    if (clientNum == 0)
    {
        cout << "No Clients\n";
    }
    cout << "__________Clients List__________" << endl;
    cout << "Cin 0 to exit:";
    return clientNum;
}
void coin(string couts, string &cins)
{
    cout << couts;
    cin >> cins;
    cin.ignore();
    return;
}
void coin(string couts, int &cins)
{
    cout << couts;
    cin >> cins;
    cin.ignore();
    return;
}
int initClient(SOCKET &s, sockaddr_in &sockAddr, string serverIp, int serverPort)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return WSAGetLastError();
    }
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        return WSAGetLastError();
    }
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp.c_str(), &sockAddr.sin_addr) <= 0)
    {
        return WSAGetLastError();
    }
    return -8192;
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
void healthyCheck(SOCKET HealthyBeat)
{
    int timeout = 10000; // 设置超时时间为 10 秒
    setsockopt(HealthyBeat, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof(timeout));
    setsockopt(HealthyBeat, SOL_SOCKET, SO_SNDTIMEO, (char *)timeout, sizeof(timeout));
    while (1)
    {
        if (closeP)
        {
            send_message(HealthyBeat, "\r\nClose\r\n");
            closesocket(HealthyBeat);
            return;
        }
        string buf;
        int state = receive_message(HealthyBeat, buf);
        if (closeP)
        {
            send_message(HealthyBeat, "\r\nClose\r\n");
            closesocket(HealthyBeat);
            return;
        }
        int state1 = send_message(HealthyBeat, buf);
        if (state == SOCKET_ERROR || state1 == SOCKET_ERROR)
        {
            while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
                ;
            ServerState = false;
            ServerHealthCheck.exchange(false, std::memory_order_release);
            return;
        }
    }
}
int login(SOCKET s)
{

    string lastloginYZM;
    string loginYZM;
    lastloginYZM.clear();
    bool loginTRUE = false;
    MD5 m;
    try
    {
        coin("password:", password);
        for (int i = 1; i <= 5 && !loginTRUE; i++)
        {
            m.init();
            string recvBuf;
            string temp = StringTime(time(NULL)) + lastloginYZM + password;
            loginYZM = m.encode(temp);
            send_message(s, loginYZM);
            receive_message(s, recvBuf);
            if (recvBuf == "error")
            {
                coin("password:", password);
                system("cls");
                lastloginYZM = loginYZM;
                continue;
            }
            else if (recvBuf == "true")
            {
                loginTRUE = true;
                break;
            }
        }
        if (!loginTRUE)
            return 0;
        string recvBuf;
        receive_message(s, recvBuf);
        SEID = recvBuf;
        initClient(healthyBeat, healthyBeatAddr, serverIp, serverPort);
        if (connect(healthyBeat, (sockaddr *)&healthyBeatAddr, sizeof(healthyBeatAddr)) == SOCKET_ERROR)
        {
            return 0;
        }
        else
        {
            ServerState = true;
            return 1;
        }
    }
    catch (std::exception &e)
    {
        cout << e.what() << endl;
    }
}
void Connect()
{
    system("cls");
    send_message(sockC, "connect");
    string recvBuf;
    string srecv, cmds;
    int kb_cin = -1;

    while (1)
    {
        system("cls");
        int clientNum = sendClientList(sockC);
        if (_kbhit())
        {
            cin >> kb_cin;
            cin.ignore();
            if (kb_cin == 0)
            {
                send_message(sockC, "\r\nexit\r\n");
                break;
            }
            if (kb_cin > 0 && kb_cin <= clientNum)
            {
                send_message(sockC, to_string(kb_cin));
                receive_message(sockC, recvBuf);
                if (strcmp(recvBuf.c_str(), "\r\n\r\nsec\r\n\r\n") == 0)
                {
                    system("cls");
                    receive_message(sockC, recvBuf);
                    cout << recvBuf;
                    int qt = 1;
                    while (1)
                    {
                        getline(cin, cmds);
                        if (strcmp(cmds.c_str(), "powershell") == 0 || strcmp(cmds.c_str(), "cmd") == 0)
                        {
                            qt++;
                        }
                        else if (strcmp(cmds.c_str(), "exit") == 0)
                        {
                            qt--;
                            if (qt == 0)
                            {
                                send_message(sockC, "\r\nfexit\r\n");
                                break;
                            }
                        }
                        else if (strcmp(cmds.c_str(), "cls") == 0)
                        {
                            system("cls");
                            continue;
                        }
                        send_message(sockC, (cmds + "\n"));
                        receive_message(sockC, recvBuf);
                        cout << recvBuf;
                    }
                }
                else
                {
                    cout << "ERROR" << endl;
                    system("pause");
                }
            }
            else
            {
                send_message(sockC, "\r\nnext\r\n");
            }
        }
        else
        {
            send_message(sockC, "\r\nnext\r\n");
        }
        Sleep(500);
    }
}
void del()
{
    send_message(sockC, "del");
    string recvBuf;
    string srecv, cmds;
    int kb_cin = -1;
    int clientNum = 0;
    while (1)
    {
        system("cls");
        sendClientList(sockC);
        if (_kbhit())
        {
            cin >> kb_cin;
            cin.ignore();
            if (kb_cin == 0)
            {
                send_message(sockC, "\r\nexit\r\n");
                break;
            }
            if (kb_cin > 0 && kb_cin <= clientNum)
            {
                send_message(sockC, to_string(kb_cin));
                receive_message(sockC, recvBuf);
                if (strcmp(recvBuf.c_str(), "\r\n\r\nUse\r\n\r\n") == 0)
                {
                    cout << "It was Use" << endl;
                    system("pause");
                }
                else if (strcmp(recvBuf.c_str(), "\r\n\r\nUnError\r\n\r\n") == 0)
                {
                    cout << "UnError" << endl;
                    system("pause");
                }
            }
        }
        else
        {
            send_message(sockC, "\r\nnext\r\n");
        }
        Sleep(500);
    }
}
void show()
{
    send_message(sockC, "show");
    char recvBuf[8192] = {0};
    string srecv, cmds;
    int kb_cin = -1;
    int clientNum = 0;
    while (1)
    {
        clientNum = 0;
        system("cls");
        sendClientList(sockC);
        if (_kbhit())
        {
            getch();
            send_message(sockC, "\r\nclose\r\n");
            break;
        }
        else
        {
            send_message(sockC, "\r\nnext\r\n");
        }
        Sleep(500);
    }
}
void cmd()
{
    char cinBuf[8192] = {0};
    string recvInfo1;
    string recvInfo2;
    string recvBuf;
    string cmds;
    while (1)
    {
        memset(cinBuf, 0, sizeof(cinBuf));
        cout << "Entry \"exit\" to exit:";
        cin.getline(cinBuf, 8191);
        cmds = cinBuf;
        if (strcmp(cmds.c_str(), "exit") == 0)
        {
            break;
        }
        send_message(sockC, ("cmd " + cmds));
        recvBuf.clear();
        receive_message(sockC, recvBuf);
        if (strcmp(recvBuf.c_str(), "\r\nsee\r\n") == 0)
        {
            sendClientList(sockC);
        }
        recvBuf.clear();
        receive_message(sockC, recvBuf);
        if (strcmp(recvBuf.c_str(), "\r\nok\r\n") == 0)
        {
            receive_message(sockC, recvInfo1);
            receive_message(sockC, recvInfo2);
            SetColor(10, 0); // green
            cout << "Command executed successfully127" << endl;
            cout << "Number of Client: " << recvInfo2 << endl;
            cout << "Number of clients successfully executing instructions: " << recvInfo1 << endl;
            SetColor(15, 0); // white
        }
        else if (strcmp(recvBuf.c_str(), "\r\ncmd error\r\n") == 0)
        {
            SetColor(4, 0); // red
            cout << "Command Error or Server Error!" << endl;
            SetColor(15, 0); // white
        }
    }
}
void pageShow()
{
    while (ServerState && !closeP)
    {
        int choose;
        cout << "1.Connect" << endl
             << "2.del Client" << endl
             << "3.show all Clients" << endl
             << "4.cmd" << endl
             << "5.exit" << endl
             << "Choose:";
        cin >> choose;
        cin.ignore();
        system("cls");
        switch (choose)
        {
        case 1:
            Connect(); // ok
            break;
        case 2:
            del(); // ok
            break;
        case 3:
            show();
            break;
        case 4:
            cmd();
            break;
        case 5:
            WhenClose();
            break;
        }
        system("cls");
    }
    closesocket(sockC);
}
thread healthyCheckThread;
thread pageShowThread;
void WhenClose()
{
    try
    {
        healthyCheckThread.detach();
        pageShowThread.detach();
    }
    catch (std::exception &e)
    {
        MessageBox(NULL, e.what(), "Error", MB_OK);
    }
    closeP = true;
    send_message(healthyBeat, "\r\nClose\r\n");
    send_message(sockC, "\r\nClose\r\n");
    closesocket(healthyBeat);
    closesocket(sockC);
    return;
}
int main()
{
    system("chcp 65001>nul");
    do
    {
        serverIp.clear();
        serverPort = 0;
        coin("ip: ", serverIp);
        coin("port: ", serverPort);
        system("cls");
    } while (initClient(sockC, sockAddr, serverIp, serverPort) != -8192);
    if (connect(sockC, (sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
        return WSAGetLastError();
    }
    string recvBuf;
    send_message(sockC, "Server");
    receive_message(sockC, recvBuf);
    if (!login(sockC))
    {
        return -4096;
    }
    send_message(healthyBeat, SEID.c_str());
    healthyCheckThread = thread(healthyCheck, healthyBeat);
    pageShowThread = thread(pageShow);
    closeCheak.setRunFun((void *)WhenClose, (void *)WhenClose, (void *)WhenClose, (void *)WhenClose, (void *)WhenClose);
    closeCheak.startHook();
    while (1)
    {
        if (closeP)
        {
            return 0;
        }
        while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
            ;
        if (ServerState == false)
        {
            healthyCheckThread.detach();
            pageShowThread.detach();
            return -16384;
        }
        ServerHealthCheck.exchange(false, std::memory_order_release);
    }
}