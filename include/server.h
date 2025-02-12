#ifndef SERVER_H
#define SERVER_H
#include "definehead.h"
atomic<bool> HealthyLock(false);
queue<HealthyDataStruct> HealthyQueue;
int ServerPort;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
WSADATA wsaData;
#endif
SOCKET ListenSocket;
sockaddr_in service;
queue<SOCKET> ServerSocketQueue;
map<string, SEIDForSocketStruct> ServerSEIDMap;
vector<ClientSocketFlagStruct> DataSaveArry;
string password;
bool dataIsChange = false;
vector<thread> ServerRSThreadArry;
mutex ServerQueueLock;
condition_variable Queuecv;
#endif