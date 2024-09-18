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
            send(HealthyBeat, "\r\nClose\r\n", strlen("\r\nClose\r\n"), 0);
            closesocket(HealthyBeat);
            return;
        }
        char buf[8192] = {0};
        int state = recv(HealthyBeat, buf, sizeof(buf), 0);
        if (closeP)
        {
            send(HealthyBeat, "\r\nClose\r\n", strlen("\r\nClose\r\n"), 0);
            closesocket(HealthyBeat);
            return;
        }
        int state1 = send(HealthyBeat, buf, strlen(buf), 0);
        if (state == SOCKET_ERROR || state1 == SOCKET_ERROR)
        {
            while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
                ;
            ServerState = false;
            ServerHealthCheck.exchange(false, std::memory_order_release);
            return;
        }
        SetColor(4, 0);
        cout << "healthyCheck" << endl;
        SetColor(15, 0);
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
        cout << "9-";
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
    send(sockC, "connect", strlen("connect"), 0);
    char recvBuf[8192] = {0};
    string srecv, cmds;
    int kb_cin = -1;
    int clientNum = 0;
    while (1)
    {
        clientNum = 0;
        system("cls");
        cout << "__________Clients List__________" << endl;
        do
        {
            recv(sockC, recvBuf, 2048, 0);
            if (strcmp(recvBuf, "\r\n\r\nend\r\n\r\n") == 0)
            {
                break;
            }
            clientNum++;
            srecv = recvBuf;
            srecv = srecv.substr(srecv.length() - 3);
            cout << clientNum << " " << srecv << endl;
        } while (1);
        if (clientNum == 0)
        {
            cout << "No Clients\n";
        }
        cout << "__________Clients List__________" << endl;
        cout << "Cin 0 to exit:";
        if (_kbhit())
        {
            cin >> kb_cin;
            cin.ignore();
            if (kb_cin == 0)
            {
                send(sockC, "\r\nexit\r\n", strlen("\r\nexit\r\n"), 0);
                break;
            }
            if (kb_cin > 0 && kb_cin <= clientNum)
            {
                send(sockC, to_string(kb_cin).c_str(), strlen(to_string(kb_cin).c_str()), 0);
                recv(sockC, recvBuf, sizeof(recvBuf), 0);
                if (strcmp(recvBuf, "\r\n\r\nsec\r\n\r\n") == 0)
                {
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
                                send(sockC, "\r\nfexit\r\n", strlen("\r\nfexit\r\n"), 0);
                                break;
                            }
                        }
                        send(sockC, cmds.c_str(), strlen(cmds.c_str()), 0);
                    }
                }
                else
                {
                    cout << "ERROR" << endl;
                    system("pause");
                }
            }
        }
        else
        {
            send(sockC, "\r\nnext\r\n", strlen("\r\nnext\r\n"), 0);
        }
        Sleep(500);
    }
}
void del()
{
    send(sockC, "del", strlen("del"), 0);
    char recvBuf[8192] = {0};
    string srecv, cmds;
    int kb_cin = -1;
    int clientNum = 0;
    while (1)
    {
        clientNum = 0;
        system("cls");
        cout << "__________Clients List__________" << endl;
        do
        {
            recv(sockC, recvBuf, 2048, 0);
            if (strcmp(recvBuf, "\r\n\r\nend\r\n\r\n") == 0)
            {
                break;
            }
            clientNum++;
            srecv = recvBuf;
            srecv = srecv.substr(srecv.length() - 3);
            cout << clientNum << " " << srecv << endl;
        } while (1);
        if (clientNum == 0)
        {
            cout << "No Clients\n";
        }
        cout << "__________Clients List__________" << endl;
        cout << "Cin 0 to exit:";
        if (_kbhit())
        {
            cin >> kb_cin;
            cin.ignore();
            if (kb_cin == 0)
            {
                send(sockC, "\r\nexit\r\n", strlen("\r\nexit\r\n"), 0);
                break;
            }
            if (kb_cin > 0 && kb_cin <= clientNum)
            {
                send(sockC, to_string(kb_cin).c_str(), strlen(to_string(kb_cin).c_str()), 0);
                recv(sockC, recvBuf, sizeof(recvBuf), 0);
                if (strcmp(recvBuf, "\r\n\r\nUse\r\n\r\n") == 0)
                {
                    cout << "It was Use" << endl;
                    system("pause");
                }
                else if (strcmp(recvBuf, "\r\n\r\nUnError\r\n\r\n") == 0)
                {
                    cout << "UnError" << endl;
                    system("pause");
                }
            }
        }
        else
        {
            send(sockC, "\r\nnext\r\n", strlen("\r\nnext\r\n"), 0);
        }
        Sleep(500);
    }
}
void show()
{
    send(sockC, "show", strlen("show"), 0);
    char recvBuf[8192] = {0};
    string srecv, cmds;
    int kb_cin = -1;
    int clientNum = 0;
    while (1)
    {
        clientNum = 0;
        system("cls");
        cout << "__________Clients List__________" << endl;
        do
        {
            recv(sockC, recvBuf, 2048, 0);
            if (strcmp(recvBuf, "\r\n\r\nend\r\n\r\n") == 0)
            {
                break;
            }
            clientNum++;
            srecv = recvBuf;
            srecv = srecv.substr(srecv.length() - 3);
            cout << clientNum << " " << srecv << endl;
        } while (1);
        if (clientNum == 0)
        {
            cout << "No Clients\n";
        }
        cout << "__________Clients List__________" << endl;
        cout << "Cin 0 to exit:";
        if (_kbhit())
        {
            getch();
            send(sockC, "\r\nclose\r\n", strlen("\r\nclose\r\n"), 0);
            break;
        }
        else
        {
            send(sockC, "\r\nnext\r\n", strlen("\r\nnext\r\n"), 0);
        }
        Sleep(500);
    }
}
void cmd()
{
    char cinBuf[8192] = {0};
    char recvBuf[8192] = {0};
    char recvInfo1[8192] = {0};
    char recvInfo2[8192] = {0};
    string cmds;
    system("cls");
    while (1)
    {
        memset(cinBuf, 0, sizeof(cinBuf));
        memset(recvBuf, 0, sizeof(recvBuf));
        memset(recvInfo1, 0, sizeof(recvInfo1));
        memset(recvInfo2, 0, sizeof(recvInfo2));
        cout << "Entry \"exit\" to exit:";
        cin.getline(cinBuf, 8191);
        cmds = cinBuf;
        if (strcmp(cmds.c_str(), "exit") == 0)
        {
            break;
        }
        send(sockC, ("cmd " + cmds).c_str(), strlen(("cmd " + cmds).c_str()), 0);
        recv(sockC, recvBuf, sizeof(recvBuf), 0);
        if (strcmp(recvBuf, "\r\nshow\r\n") == 0)
        {
            int clientNum = 0;
            system("cls");
            cout << "__________Clients List__________" << endl;
            do
            {
                recv(sockC, recvBuf, 2048, 0);
                if (strcmp(recvBuf, "\r\n\r\nend\r\n\r\n") == 0)
                {
                    break;
                }
                clientNum++;
                string srecv = recvBuf;
                srecv = srecv.substr(srecv.length() - 3);
                cout << clientNum << " " << srecv << endl;
            } while (1);
            if (clientNum == 0)
            {
                cout << "No Clients\n";
            }
            cout << "__________Clients List__________" << endl;
        }
        recv(sockC, recvBuf, sizeof(recvBuf), 0);
        if (strcmp(recvBuf, "\r\nok\r\n") == 0)
        {
            recv(sockC, recvInfo1, sizeof(recvInfo1), 0);
            recv(sockC, recvInfo2, sizeof(recvInfo2), 0);
            SetColor(10, 0); // green
            cout << "Command executed successfully" << endl;
            cout << "Number of Client: " << recvInfo2 << endl;
            cout << "Number of clients successfully executing instructions: " << recvInfo1 << endl;
            SetColor(15, 0); // white
        }
        else if (strcmp(recvBuf, "\r\ncmd error\r\n") == 0)
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
    send(healthyBeat, "\r\nClose\r\n", strlen("\r\nClose\r\n"), 0);
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
    char recvBuf[1240] = {0};
    send(sockC, "Server", strlen("Server"), 0);
    recv(sockC, recvBuf, sizeof(recvBuf), 0);
    if (!login(sockC))
    {
        return -4096;
    }
    send(healthyBeat, SEID.c_str(), strlen(SEID.c_str()), 0);
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