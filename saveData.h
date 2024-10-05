#include "global.h"
#ifndef SAVEDATA_H_
#define SAVEDATA_H_
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
    while (1)
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

    while (1)
    {
        if (!PassDataInit)
        {
            while (ClientMapLock.exchange(true, std::memory_order_acquire))
                ; // 加锁
            for (int i = 1; i <= ClientMap.size(); i++)
            {
                if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
                {
                    delForId(i);
                }
                else
                    DataSaveArry.push_back(ClientMap[i - 1]);
            }
            Sleep(1000);
            PassDataInit = true;
            ClientMapLock.exchange(false, std::memory_order_release);
            continue;
        }
        while (ClientMapLock.exchange(true, std::memory_order_acquire))
            ; // 加锁
        for (int i = 1; i <= ClientMap.size(); i++)
        {
            if (ClientMap[i - 1].state != ClientSocketFlagStruct::states::Use && ClientMap[i - 1].Offline != 0 && (ClientMap[i - 1].Offline - ClientMap[i - 1].Online) >= 3600)
            {
                delForId(i);
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
        ClientMapLock.exchange(false, std::memory_order_release);
        Sleep(2500);
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
        port = 6020;
        return;
    }
    string portstr;
    inss >> portstr;
    ServerPort = stoi(portstr);
    inss.close();
}
#endif