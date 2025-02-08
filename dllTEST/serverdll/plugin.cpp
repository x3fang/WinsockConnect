#include "plugin.h"
#include <iostream>
using std::cout;
using std::endl;
bool EXPORT registerPlugin(string pluginName,
                           string runlineS,
                           void (*startupfunPtr)(),
                           void (*startfunPtr)(),
                           void (*stopfunPtr)(),
                           bool (*runfunPtr)(allInfoStruct *info),
                           bool isStart)
{
      string runLineB = runLineMap[runlineS];
      std::reverse(runLineB.begin(), runLineB.end());
      std::bitset<RUN_LINE_NUM> runline(runLineB, 0);
      if (!findPlugin(pluginName))
            return false;
      while (pluginVectorLock.exchange(true, std::memory_order_acquire))
            ;
      pluginListStruct *newPlugin = new pluginListStruct;
      newPlugin->next = nullptr;
      newPlugin->plugin = std::make_unique<pluginStruct>(pluginName,
                                                         startupfunPtr,
                                                         startfunPtr,
                                                         stopfunPtr,
                                                         runfunPtr,
                                                         isStart);
      pluginNameList.push_back(pluginName);
      if (newPlugin->plugin->startupFun != nullptr)
      {
            newPlugin->plugin->startupFun();
      }
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (runline[i])
            {
                  newPlugin->next = &(*pluginList[i]);
                  pluginList[i] = newPlugin;
            }
      }
      if (runline[Fun])
      {
            funPluginNameList.push_back(pluginName);
      }
      return true;
}
bool EXPORT delPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
            return false;
      bool status = false;
      for (int i = 2; i < RUN_LINE_NUM; i++)
      {
            pluginListStruct *last;
            for (pluginListStruct *it = pluginList[i]; it != nullptr && it->next != nullptr; last = &(*it), it = it->next)
            {
                  if (it->plugin->funName == pluginName)
                  {
                        last->next = it->next;
                        delete it;
                        status = true;
                  }
            }
      }
      if (find(funPluginNameList.begin(), funPluginNameList.end(), pluginName) != funPluginNameList.end())
      {
            funPluginNameList.erase(find(funPluginNameList.begin(), funPluginNameList.end(), pluginName));
      }
      pluginNameList.erase(find(pluginNameList.begin(), pluginNameList.end(), pluginName));
      return status;
}
bool EXPORT findPlugin(string pluginName)
{
      return (find(pluginNameList.begin(), pluginNameList.end(), pluginName) == pluginNameList.end());
}
bool EXPORT rsetPlugin(string pluginName,
                       string runlineS,
                       void (*startupfunPtr)(),
                       void (*startfunPtr)(),
                       void (*stopfunPtr)(),
                       bool (*runfunPtr)(allInfoStruct *info),
                       bool isStart)
{
      if (!findPlugin(pluginName))
            return false;
      if (delPlugin(pluginName))
            return registerPlugin(pluginName, runlineS, startupfunPtr, startfunPtr, stopfunPtr, runfunPtr, isStart);
      return false;
}
bool EXPORT startPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
            return false;
      for (int i = 2; i < RUN_LINE_NUM; i++)
      {
            for (pluginListStruct *it = pluginList[i]; it != nullptr; it = it->next)
            {
                  if (it->plugin->startFun != NULL &&
                      it->plugin->funName == pluginName &&
                      !it->plugin->isStart)
                  {
                        it->plugin->isStart = true;
                        it->plugin->startFun();
                        return true;
                  }
            }
      }
      return false;
}
bool EXPORT stopPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
            return false;
      for (int i = 2; i < RUN_LINE_NUM; i++)
      {
            for (pluginListStruct *it = pluginList[i]; it != nullptr; it = it->next)
            {
                  if (it->plugin->stopFun != NULL &&
                      it->plugin->funName == pluginName &&
                      it->plugin->isStart)
                  {
                        it->plugin->isStart = false;
                        it->plugin->stopFun();
                        return true;
                  }
            }
      }
      return false;
}
bool EXPORT runPlugin(allInfoStruct &info, string runlineS)
{
      string pluginLine = runLineMap[runlineS];
      std::reverse(pluginLine.begin(), pluginLine.end());
      std::bitset<RUN_LINE_NUM> runline(pluginLine, 0);
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (runline[i])
            {
                  for (pluginListStruct *it = pluginList[i]; it != nullptr; it = it->next)
                  {
                        if (it->plugin->isStart &&
                            it->plugin->runFun != NULL &&
                            it->plugin->runFun(&info))
                        {
                              return false;
                        }
                  }
            }
      }
      return true;
}
bool EXPORT runFun(allInfoStruct *info, string Name)
{
      cout << info->SEID << endl;
      cout << info->ServerSEIDMap << endl;
      cout << info->ClientSEIDMap << endl;
      cout << info->NowSocket << endl;

      string pluginLine = runLineMap["fun"];
      reverse(pluginLine.begin(), pluginLine.end());
      std::bitset<RUN_LINE_NUM> runline(pluginLine, 0);
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (runline[i])
            {
                  for (pluginListStruct *it = pluginList[i]; it != nullptr; it = it->next)
                  {
                        if (it->plugin->isStart &&
                            it->plugin->funName == Name &&
                            it->plugin->runFun != NULL)
                        {
                              if (it->plugin->runFun(info))
                              {
                                    return true;
                              }
                              return false;
                        }
                  }
            }
      }
      return false;
}