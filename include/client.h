#ifndef client_h
#define client_h
#include "definehead.h"
mutex ClientQueueLock;
atomic<bool> ClientMapLock(false);
vector<thread> ClientRSThreadArry;
vector<ClientSocketFlagStruct> ClientMap;
map<string, SEIDForSocketStruct> ClientSEIDMap;
queue<SOCKET> ClientSocketQueue;
void delForId_wait(SEIDForSocketStruct *ClientInfo_CSM, int ClientId, bool *issec);
void EXPORT joinClient(string ClientWanIp, string ClientLanIp, string ClientPort, unsigned long long int OnlineTime, unsigned long long int OfflineTime, string SEID);
extern "C" int EXPORT delForId(allInfoStruct *info, int ClientId, bool iswait, std::pair<bool, std::shared_ptr<thread>> *wait);
void setClientExit(SEIDForSocketStruct &s);
int showForSend(allInfoStruct *info, string seid, filter f, bool startIf = false, ClientSocketFlagStruct::states state = ClientSocketFlagStruct::NULLs);
void setClientState(ClientSocketFlagStruct *ClientSocketFlagStruct, ClientSocketFlagStruct::states state)
{
      ClientSocketFlagStruct->state = state;
      return;
}
#include "client.cpp"
#endif