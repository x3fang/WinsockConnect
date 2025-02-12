#ifndef SAVEDATA_H_
#define SAVEDATA_H_
#include "definehead.h"

bool ischange = false;
int PassDataInit = 0;
bool closeServer = false;
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
                  << " " << DataSaveArry[i - 1].SEID
                  << endl;
      }
      outss.close();
}
void dataSave()
{
      while (!closeServer)
      {
            if (dataIsChange)
            {
                  saveData();
            }
            Sleep(5000);
      }
}
void passData()
{
      allInfoStruct info("", 0, "ClientConnect", runPluginValue);
      while (!closeServer)
      {
            if (!PassDataInit)
            {
                  for (int i = 1; i <= ClientMap.size(); i++)
                  {
                        if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
                        {
                              std::shared_ptr<thread> waitthread;
                              std::pair<bool, std::shared_ptr<thread>> delsecWait = {false, waitthread};
                              int res = delForId(&info, i, true, &delsecWait);
                              while (!delsecWait.first)
                                    ;
                        }
                        else
                              DataSaveArry.push_back(ClientMap[i - 1]);
                  }
                  Sleep(1000);
                  PassDataInit = true;
                  continue;
            }
            for (int i = 1; i <= ClientMap.size(); i++)
            {
                  if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
                  {
                        std::shared_ptr<thread> waitthread;
                        std::pair<bool, std::shared_ptr<thread>> delsecWait = {false, waitthread};
                        int res = delForId(&info, i, true, &delsecWait);
                        while (!delsecWait.first)
                              ;
                  }
                  else
                  {
                        dataIsChange = true;
                        vector<ClientSocketFlagStruct>::iterator itr = find(DataSaveArry.begin(), DataSaveArry.end(), ClientMap[i - 1]);
                        if (itr != DataSaveArry.end())
                        {
                              DataSaveArry[distance(DataSaveArry.begin(), itr)] = ClientMap[i - 1];
                        }
                  }
            }
            Sleep(10000);
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
            password = "123456";
      }
      else
      {
            inPassword >> password;
#ifdef _DEBUG
            password = "1";
            cout << "Load Password:" << password << endl;
#endif
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
      string wanip, lanip, port, OnlineTime, OfflineTime, SEID;
      while (inss >> wanip >> lanip >> port >> OnlineTime >> OfflineTime >> SEID)
      {
            joinClient(wanip, lanip, port, stoi(OnlineTime), stoi(OfflineTime), SEID);
      }
      inss.close();
      inss.open("serverConfig.config", ios::in);
      if (!inss)
      {
            ofstream out;
            out.open("serverConfig.config", ios::out);
            out.close();
            inss.close();
            ServerPort = 6020;
            return;
      }
      string portstr;
      inss >> portstr;
      ServerPort = stoi(portstr);
      inss.close();
}
#endif