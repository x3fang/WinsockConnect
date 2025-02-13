
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
#define UNLOADSC_HEAD
#include "include/definehead.h"
#include "include/MD5.h"
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
SOCKET g_sock = INVALID_SOCKET, healthySock = INVALID_SOCKET;
sockaddr_in g_serverInfo = {0};
vector<string> g_getFilesNameList, g_loadFunPluginName, g_otherPluginList;
string g_SEID;
string serverIP, serverPort;
thread g_healthyThread;
bool g_serverStopFlag = false;
bool stopHealthyCheck = true;
void getFilesName(string path, vector<string> &files);
vector<string> pageList = {"Fun plugins", "setting"};
vector<void (*)()> pageFunList;

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
int initFunPlugin()
{
      getFilesName("plugins/Fun", g_getFilesNameList);
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

      // init fun plugin
      iResult = initFunPlugin();
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
                          vector<string> &loadPluginName, bool Funplugin = true)
{
      int sucLoadNum = 0;
      int failLoadNum = 0;
      for (auto pluginName : pluginList)
      {
            if (find(g_loadFunPluginName.begin(),
                     g_loadFunPluginName.end(),
                     pluginName) == g_loadFunPluginName.end()) // no find
            {
                  auto loadLibrary = LoadLibrary(("plugins\\Fun\\" + pluginName + ".dll").c_str());
                  if (!Funplugin)
                        loadLibrary = LoadLibrary(("plugins\\other\\" + pluginName + ".dll").c_str());
                  if (loadLibrary == NULL)
                  {
                        failLoadNum++;
                        continue;
                  }
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
                  else
                  {
                        cout << "registerPlugin failed" << endl;
                        system("pause");
                        failLoadNum++;
                  }
            }
            else
            {
                  loadPluginName.push_back(pluginName);
                  sucLoadNum++;
            }
      }
      return std::make_pair(sucLoadNum, failLoadNum);
}
pair<int, int> loadlocalPlugin(vector<string> &loadPluginName)
{
      int loadFailedNum = 0;
      int loadsecNum = 0;
      vector<string> otherPluginNames;
      // get other plugin  names
      getFilesName("plugins/Other", otherPluginNames);
      if (otherPluginNames.empty())
      {
            return std::make_pair(0, 0);
      }
      for (auto pluginName : otherPluginNames)
      {
            if (pluginName.find(".dll") != string::npos)
                  pluginName = pluginName.substr(0, pluginName.find_last_of("."));
            if (pluginName.find("\\") != string::npos)
                  pluginName = pluginName.substr(pluginName.find_last_of("\\") + 1);
            else if (pluginName.find("/") != string::npos)
                  pluginName = pluginName.substr(pluginName.find_last_of("/") + 1);
      }

      // load plugin
      auto temp = loadPlugin(otherPluginNames, "local", loadPluginName, false);
      return temp;
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
                        if (find(g_loadFunPluginName.begin(), g_loadFunPluginName.end(), index) == g_loadFunPluginName.end())
                        {
                              g_loadFunPluginName.push_back(index);
                        }
                  }
            }
            else
            {
                  iResult = loadPlugin(pluginListTemp, pluginType[i], loadPluginName[i]).second;
                  for (auto index : loadPluginName[i])
                  {
                        if (find(g_loadFunPluginName.begin(), g_loadFunPluginName.end(), index) == g_loadFunPluginName.end())
                        {
                              g_loadFunPluginName.push_back(index);
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
      sockaddr_in serverInfo;
      int iResult = initClientSocket(g_wsaData, healthySock, serverInfo, serverIP, std::stoi(serverPort));
      if (iResult != 0)
            return;
      iResult = connectServer(healthySock, serverInfo);
      if (iResult != 0)
            return;
      send_message(healthySock, SEID);
      string msg;

      int timeout = 3000;
      setsockopt(healthySock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
      setsockopt(healthySock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
      while (true)
      {
            if (!stopHealthyCheck)
            {
                  int state1 = receive_message(healthySock, msg);
                  int state2 = send_message(healthySock, msg);
                  if (g_serverStopFlag || !state1 || !state2)
                  {
                        g_serverStopFlag = true;
                        send_message(healthySock, "\r\nClose\r\n");
                        break;
                  }
            }
      }
      closesocket(healthySock);
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
int choosePage()
{
      while (true)
      {
            system("cls");
            cout << "input -1 to exit program" << endl;
            for (int index = 0; index < pageList.size(); index++)
            {
                  cout << "[" << index + 1 << "]" << pageList[index] << "   ";
                  if ((index + 1) % 5 == 0)
                        cout << endl;
            }
            cout << "Choose(1~" << pageList.size() << "): ";
            int choose = 0;
            cin.clear();
            cin.sync();
            scanf("%d", &choose);
            if (choose == -1)
            {
                  return -1;
            }
            if (choose < 1 || choose > pageList.size())
            {
                  system("cls");
            }
            else
            {
                  return choose - 1;
            }
      }
}
void funplugin()
{
      while (true)
      {
            system("cls");
            vector<vector<string>> pluginList;
            pullPlugin(g_sock, "0", {"fun"}, pluginList);
            string pluginName;
            // system("pause");
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
                  cin >> choose;
                  if (choose == -1)
                  {
                        return;
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
                  allInfoStruct info(g_SEID, g_sock, "", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                  if (!runFun(&info, pluginName))
                  {
                        if (!info.msg.empty())
                        {
                              MessageBox(NULL,
                                         ("Error from plugin:" + pluginName + ".dll\n" + "path: plugins/" + pluginName + ".dll\n" + info.msg).c_str(),
                                         "Error!", MB_OK);
                        }
                  }
                  receive_message(g_sock, Sres);
                  cout << Sres << endl;
                  if (Sres != "\r\nrfs\r\n")
                  {
                        runPlugin(info, "rFf");
                  }
                  else
                  {
                        runPlugin(info, "rFs");
                  }
            }
            else
            {
                  // No find fun
            }
      }
}
int Mainloop()
{
      loadlocalPlugin(g_otherPluginList);
      while (true)
      {
            int ch = choosePage();
            if (ch == -1)
            {
                  g_serverStopFlag = true;
                  g_healthyThread.join();
                  return 0;
            }
            if (pageFunList.at(ch) != NULL && pageFunList[ch] != nullptr)
            {
                  pageFunList.at(ch)();
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
      cout << "input server IP and port:";
      // cin >> serverIP >> serverPort;
      serverIP = "127.0.0.1";
      serverPort = "6020";
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
      pageFunList.push_back((&funplugin));
      pageFunList.push_back(nullptr);
      vector<void *> tempV;
      tempV.push_back(&pageFunList);
      tempV.push_back(&pageList);
      allInfoStruct info(g_SEID,
                         g_sock,
                         string("B-loop"),
                         nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr,
                         &tempV);
      runPlugin(info, "local");
      return Mainloop();
}
void getFilesName(string path, vector<string> &files)
{
      // 文件句柄
      intptr_t hFile = 0;
      // 文件信息
      struct _finddata_t fileinfo;
      string p;
      if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
      {
            do
            {
                  // 如果是目录,迭代之
                  // 如果不是,加入列表
                  if ((fileinfo.attrib & _A_SUBDIR))
                  {
                        if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                              getFilesName(p.assign(path).append("\\").append(fileinfo.name), files);
                  }
                  else
                  {
                        files.push_back(path + "\\" + fileinfo.name);
                  }
            } while (_findnext(hFile, &fileinfo) == 0);
            _findclose(hFile);
      }
}