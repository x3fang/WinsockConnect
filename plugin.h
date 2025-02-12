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
    {"as", "100000000000000000"},       // accept socket
    {"sRSstart", "010000000000000000"}, // serverRS start
    {"sonline", "001000000000000000"},  // server online
    {"srd", "000100000000000000"},      // server rece data
    {"cRSstart", "000010000000000000"}, // clientRS start
    {"coline", "000001000000000000"},   // client online
    {"cexit", "000000100000000000"},    // client exit
    {"hCsd", "000000010000000000"},     // healthyCheack send data
    {"hCrd", "000000001000000000"},     // healthyCheack recv data
    {"hCjudge", "000000000100000000"},  // healthyCheack judge
    {"eFstart", "000000000010000000"},  // everyFunction start
    {"eFend", "000000000001000000"},    // evertFunction end
    {"cSEID", "000000000000100000"},    // create SEID
    {"fun", "000000000000001000"},      // fun plugin
    {"rFf", "000000000000000100"},      // run fun failed
    {"rFs", "000000000000000010"},      // run fun success
    {"local", "000000000000000001"}     // local plugin
};
#define RUN_LINE_NUM 19 // runLineMap.size()+1
enum RunLine            // 对应的位数(runLineMap) 以便索引此功能在 pluginList 中的位置
{
      AcceptSocket = 1,
      ServerRSStart = 2,
      ServerOnline = 3,
      ServerRecvData = 4,
      ClientRSStart = 5,
      ClientOnline = 6,
      ClientExit = 7,
      HealthyCheckSendData = 8,
      HealthyCheckRecvData = 9,
      HealthyCheckJudge = 10,
      EveryFunctionStart = 11,
      EveryFunctionEnd = 12,
      CreateSEID = 13,
      Fun = 14,
      Log = 15,
      RunFunFailed = 16,
      RunFunSuccess = 17,
      Local = 18
};
struct EXPORT pluginStruct
{
      bool isStart = false;
      string funName;
      void (*startupFun)();
      void (*startFun)();
      void (*stopFun)();
      bool (*runFun)(allInfoStruct *info);
      pluginStruct(string funName,
                   void (*startupfunPtr)(),
                   void (*startfunPtr)(),
                   void (*stopfunPtr)(),
                   bool (*runfunPtr)(allInfoStruct *info),
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
      std::shared_ptr<pluginListStruct> next;
      std::shared_ptr<pluginStruct> plugin;
};
vector<std::shared_ptr<pluginListStruct>> pluginList(RUN_LINE_NUM + 2);
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
                                 bool (*runfunPtr)(allInfoStruct *),
                                 bool isStart = true);
      bool EXPORT delPlugin(string pluginName);
      bool EXPORT findPlugin(string pluginName);
      bool EXPORT rsetPlugin(string pluginName,
                             string runlineS,
                             void (*startupfunPtr)(),
                             void (*startfunPtr)(),
                             void (*stopfunPtr)(),
                             bool (*runfunPtr)(allInfoStruct *),
                             bool isStart = true);
      bool EXPORT startPlugin(string pluginName);
      // bool EXPORT LogWrite(string logMSG);
      bool EXPORT stopPlugin(string pluginName);
      bool EXPORT runPlugin(allInfoStruct &info, string runlineS);
      bool EXPORT runFun(allInfoStruct *info, string Name);
}
#include "plugin.cpp"
#endif