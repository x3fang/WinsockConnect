#ifndef SAVEDATA_H_
#define SAVEDATA_H_
#include "definehead.h"

bool closeServer = false;
void saveData()
{
      ofstream outss;
      while (!closeServer)
      {
            outss.open("clientData.txt", ios::out);
            for (auto i : ClientMap)
            {
                  outss << i.ClientWanIp
                        << " " << i.ClientLanIp
                        << " " << i.ClientConnectPort
                        << " " << to_string(i.OnlineTime)
                        << " " << to_string(i.OfflineTime)
                        << " " << i.SEID
                        << " " << i.state
                        << endl;
            }
            outss.close();
            Sleep(1800000); // 30min
      }
}
void loadData()
{
      ifstream inss;
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
      allInfoStruct info("", 0, to_string(ServerPort), runPluginValue);
      runPlugin(info, "loadData");
      return;
}
#endif