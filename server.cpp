
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
#include "MD5.h"
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

WSADATA g_wsaData;
SOCKET g_sock = INVALID_SOCKET;
sockaddr_in g_serverInfo = {0};
vector<string> g_getFilesNameList, g_loadPluginName;
string g_SEID;
string serverIP, serverPort;
thread g_healthyThread;
bool g_serverStopFlag = false;

int initClientSocket(WSADATA &g_wsaData, SOCKET &sock, sockaddr_in &serverInfo, string serverIP, int serverPort)
{
      int iResult;
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
            return WSAGetLastError();
      }
      return 0;
}
int initPlugin()
{
      getFilesName("plugins", g_getFilesNameList);
      if (g_getFilesNameList.empty())
            return 0;
      string pluginName = g_getFilesNameList.front();
      for (int i = 0;
           i <= g_getFilesNameList.size();
           i++, pluginName = g_getFilesNameList.front())

      {
            if (pluginName.find(".dll") == string::npos)
            {
                  g_getFilesNameList.erase(g_getFilesNameList.begin());
                  continue;
            }
            string temp;
            temp.clear();
            try
            {
                  string h = (pluginName).substr(0, (pluginName).find("."));
                  temp = (pluginName).substr(0, (pluginName).find("."));
                  temp = temp.substr(temp.find_last_of("\\") + 1);
            }
            catch (...)
            {
                  temp = (pluginName);
            }
            g_getFilesNameList.erase(g_getFilesNameList.begin());
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
      iResult = initClientSocket(g_wsaData, g_sock, g_serverInfo, serverIP, serverPort);
      if (iResult != 0)
            return iResult;

      return 0;
}
pair<int, int> loadPlugin(vector<string> pluginList,
                          string pluginType,
                          vector<string> &loadPluginName)
{
      int sucLoadNum = 0;
      int failLoadNum = 0;
      for (auto pluginName : pluginList)
      {
            if (find(g_loadPluginName.begin(),
                     g_loadPluginName.end(),
                     pluginName) == g_loadPluginName.end()) // no find
            {
                  auto loadLibrary = LoadLibrary(("plugins\\" + pluginName + ".dll").c_str());
                  if (loadLibrary == NULL)
                  {
                        failLoadNum++;
                  }
                  else
                  {
                        auto statusFun = (startupFun_ptr)GetProcAddress(loadLibrary, "Startup");
                        auto startFun = (startFun_ptr)GetProcAddress(loadLibrary, "Start");
                        auto stopFun = (stopFun_ptr)GetProcAddress(loadLibrary, "Stop");
                        auto runFun = (runFun_ptr)GetProcAddress(loadLibrary, "run");
                        if (registerPlugin(pluginName,
                                           pluginType,
                                           statusFun,
                                           startFun,
                                           stopFun,
                                           runFun))
                        {
                              loadPluginName.push_back(pluginName);
                              sucLoadNum++;
                        }
                  }
            }
            else
            {
                  loadPluginName.push_back(pluginName);
                  failLoadNum++;
            }
      }
      return std::make_pair(sucLoadNum, failLoadNum);
}
int pullOneClassPlugin(SOCKET &sock, vector<string> &pluginList)
{
      int iResult = 0;
      string message;
      do
      {
            receive_message(sock, message);
            if (message.find("\r\nend\r\n") != string::npos)
                  break;
            pluginList.push_back(message);
            iResult++;
      } while (true);
      return iResult;
}
// necessity: every bit: 0:unnecessity 1:necessary
int pullPlugin(SOCKET &sock, string necessity,
               vector<string> pluginType,
               vector<vector<string>> &loadPluginName)
{
      int loadFailedNum = 0;
      int iResult = 0;
      loadPluginName.resize(64);
      for (int i = 0; i < necessity.length(); i++)
      {
            vector<string> pluginListTemp;
            iResult = pullOneClassPlugin(sock, pluginListTemp);
            if (iResult == 0)
            {
                  continue;
            }
            if (necessity[i] == '0')
            {
                  auto temp = loadPlugin(pluginListTemp, pluginType[i], loadPluginName[i]);
                  loadFailedNum += temp.first;
                  for (auto index : loadPluginName[i])
                  {
                        if (find(g_loadPluginName.begin(), g_loadPluginName.end(), index) == g_loadPluginName.end())
                        {
                              g_loadPluginName.push_back(index);
                        }
                  }
            }
            else
            {
                  iResult = loadPlugin(pluginListTemp, pluginType[i], loadPluginName[i]).second;
                  for (auto index : loadPluginName[i])
                  {
                        if (find(g_loadPluginName.begin(), g_loadPluginName.end(), index) == g_loadPluginName.end())
                        {
                              g_loadPluginName.push_back(index);
                        }
                  }
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
      int iResult = initClientSocket(g_wsaData, sock, serverInfo, serverIP, std::stoi(serverPort));
      if (iResult != 0)
            return;
      iResult = connectServer(sock, serverInfo);
      if (iResult != 0)
            return;
      send_message(sock, SEID);
      string msg;

      int timeout = 3000;
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
      setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

      while (true)
      {
            int state1 = receive_message(sock, msg);
            int state2 = send_message(sock, msg);
            if (!STOP_HEALTH_CHECK && (g_serverStopFlag || !state1 || !state2))
            {
                  g_serverStopFlag = true;
                  send_message(sock, "\r\nClose\r\n");
                  break;
            }
      }
      closesocket(sock);
      return;
}
int handshake(SOCKET &sock)
{
      // send id
      send_message(sock, "Server");

      // load plugin
      vector<vector<string>> temp;
      int iResultPlugin = pullPlugin(sock, "1", {"sRSstart"}, temp);
      if (iResultPlugin < 0)
      {
            return iResultPlugin;
      }

      // create healthy check thread
      receive_message(sock, g_SEID);
      g_healthyThread = thread(healthyCheck, g_SEID);
      return 0;
}
int Mainloop()
{
      while (true)
      {
            vector<vector<string>> pluginList;
            pullPlugin(g_sock, "0", {"fun"}, pluginList);
            string pluginName;
            system("pause");
            while (!pluginList[0].empty())
            {
                  cout << "input -1 to exit program" << endl;
                  int Tabflag = 1, index = 0;
                  for (auto plugin : pluginList[0])
                  {
                        cout << "plugin: " << plugin << " [" << index << "]";
                        if (Tabflag == 2)
                              cout << endl;
                        else
                              cout << "\t";
                        Tabflag = (Tabflag + 1) % 3;
                        index++;
                  }

                  cout << "Choose(0~" << index - 1 << "): \n";
                  int choose = 0;
                  cin.clear();
                  cin.sync();
                  scanf("%d", &choose);
                  if (choose == -1)
                  {
                        send_message(g_sock, "\r\nClose\r\n");
                        g_serverStopFlag = true;
                        g_healthyThread.join();
                        return 0;
                  }
                  else if (choose < 0 || choose >= index)
                  {
                        system("cls");
                        continue;
                  }
                  else
                  {
                        pluginName = pluginList[0][choose];
                        break;
                  }
            }

            string Sres;
            send_message(g_sock, pluginName);
            receive_message(g_sock, Sres);
            if (Sres == "\r\nsec\r\n")
            {
                  allInfoStruct info(g_SEID, g_sock, "", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                  if (runFun(&info, pluginName))
                  {
                  }
                  receive_message(g_sock, Sres);
                  if (Sres != "\r\nrfs\r\n")
                  {
                        // run fun failed
                  }
                  else
                  {
                        // success run fun
                  }
            }
            else
            {
                  // No find fun
            }
      }
}
int main()
{
      cin.clear();
      cin.sync();
      char path[MAX_PATH] = {0};
      string temp, fileName;
      GetModuleFileName(NULL, path, MAX_PATH);
      temp = path;
      temp = temp.substr(temp.find_last_of("\\") + 1);
      temp = temp.substr(0, temp.find(".exe"));

      fileName = temp;

      prlog.setName(fileName);

      int iResult_ = WSAStartup(MAKEWORD(2, 2), &g_wsaData);
      if (iResult_ != 0)
      {
            printf("WSAStartup failed: %d\n", iResult_);
            return WSAGetLastError();
      }
      cout << "input server IP and port" << endl;
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

      ;
      return Mainloop();
}