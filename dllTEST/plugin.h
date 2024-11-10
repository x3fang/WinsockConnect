#ifndef PLUGIN_H
#define PLUGIN_H
#define EXPORT __declspec(dllexport)
#include "definehead.h"
// #include "globaldll.h"
/*
run line:
0: accept socket
1: serverRS start
2: server online
3: server rece data
4: clientRS start
5: client oline
6: client exit
7: healthyCheack send data
8: healthyCheack recv data
9: healthyCheack judge
10: everyFunction start
11: everyFunction end
12: create SEID
*/
map<string, string> runLineMap = {
    {"as", "1000000000000000"},       // accept socket
    {"sRSstart", "0100000000000000"}, // serverRS start
    {"sonline", "0010000000000000"},  // server online
    {"srd", "0001000000000000"},      // server rece data
    {"cRSstart", "0000100000000000"}, // clientRS start
    {"coline", "0000010000000000"},   // client online
    {"cexit", "0000001000000000"},    // client exit
    {"hCsd", "0000000100000000"},     // healthyCheack send data
    {"hCrd", "0000000010000000"},     // healthyCheack recv data
    {"hCjudge", "0000000001000000"},  // healthyCheack judge
    {"eFstart", "0000000000100000"},  // everyFunction start
    {"eFend", "0000000000010000"},    // evertFunction end
    {"cSEID", "0000000000001000"},    // create SEID
    {"fun", "0000000000000010"},      // fun plugin
    {"log", "0000000000000001"}       // log plugin
};
enum RunLine
{
    AcceptSocket = 0,
    ServerRSStart = 1,
    ServerOnline = 2,
    ServerRecvData = 3,
    ClientRSStart = 4,
    ClientOnline = 5,
    ClientExit = 6,
    HealthyCheckSendData = 7,
    HealthyCheckRecvData = 8,
    HealthyCheckJudge = 9,
    EveryFunctionStart = 10,
    EveryFunctionEnd = 11,
    CreateSEID = 12,
    Fun = 13,
    Log = 14

};
struct EXPORT pluginStruct
{
    bool isStart = false;
    string funName;
    void (*startupFun)();
    void (*startFun)();
    void (*stopFun)();
    bool (*runFun)(allInfoStruct &info);
    pluginStruct(string funName,
                 void (*startupfunPtr)(),
                 void (*startfunPtr)(),
                 void (*stopfunPtr)(),
                 bool (*runfunPtr)(allInfoStruct &info),
                 bool isStart = true)
    {
        this->funName = funName;
        this->startupFun = startupfunPtr;
        this->startFun = startfunPtr;
        this->stopFun = stopfunPtr;
        this->runFun = runfunPtr;
        this->isStart = isStart;
    }
    ~pluginStruct()
    {
        this->startupFun = nullptr;
        this->startFun = nullptr;
        this->stopFun = nullptr;
        this->runFun = nullptr;
    }
};
struct EXPORT pluginListStruct
{
    pluginListStruct *next = nullptr;
    std::unique_ptr<pluginStruct> plugin;
};
pluginListStruct *pluginList[RUN_LINE_NUM + 2];
atomic<bool> pluginVectorLock(false);
vector<string> pluginNameList;
vector<string> funPluginNameList;
string fileName;
extern "C"
{
    bool EXPORT registerPlugin(string pluginName,
                               string runlineS,
                               void (*startupfunPtr)(),
                               void (*startfunPtr)(),
                               void (*stopfunPtr)(),
                               bool (*runfunPtr)(allInfoStruct &),
                               bool isStart = true);
    bool EXPORT delPlugin(string pluginName);
    bool EXPORT findPlugin(string pluginName);
    bool EXPORT rsetPlugin(string pluginName,
                           string runlineS,
                           void (*startupfunPtr)(),
                           void (*startfunPtr)(),
                           void (*stopfunPtr)(),
                           bool (*runfunPtr)(allInfoStruct &),
                           bool isStart = true);
    bool EXPORT startPlugin(string pluginName);
    // bool EXPORT LogWrite(string logMSG);
    bool EXPORT stopPlugin(string pluginName);
    bool EXPORT runPlugin(allInfoStruct &info, string runlineS);
}
#endif