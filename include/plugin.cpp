#include "plugin.h"
#include <iostream>
using std::cout;
using std::endl;
extern "C" bool EXPORT registerPlugin(string pluginName,
                                      string runlineS,
                                      void (*startupfunPtr)(),
                                      void (*startfunPtr)(),
                                      void (*stopfunPtr)(),
                                      bool (*runfunPtr)(allInfoStruct *info),
                                      bool isStart,
                                      map<string, string> (*cmdInfoGet)())
{
      string runLineB = runLineMap[runlineS];
      std::reverse(runLineB.begin(), runLineB.end());
      std::bitset<RUN_LINE_NUM> runline(runLineB, 0);
      if (findPlugin(pluginName))
            return false;
      std::shared_ptr<pluginListStruct> newPlugin = std::make_shared<pluginListStruct>();
      newPlugin->next = nullptr;
      newPlugin->plugin = std::make_shared<pluginStruct>(pluginName,
                                                         startupfunPtr,
                                                         startfunPtr,
                                                         stopfunPtr,
                                                         runfunPtr,
                                                         isStart,
                                                         cmdInfoGet);
      pluginNameList.push_back(pluginName);
      if (newPlugin->plugin->startupFun != nullptr)
      {
            newPlugin->plugin->startupFun();
      }
      int index = 0;
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (runline[i])
            {
                  if (pluginList[i] != nullptr)
                        newPlugin->next = pluginList[i];
                  pluginList[i] = newPlugin;
                  index = i;
            }
      }
      if (runline[Fun])
      {
            funPluginNameList.push_back(pluginName);
      }
      else if (runline[FunTerminal])
      {
            funPluginComdVerNameList.push_back({pluginName, pluginList[index]});
      }
      return true;
}
extern "C" bool EXPORT delPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
      {
            // 没找到
            return false;
      }

      bool status = false;
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (pluginList[i].get() != nullptr)
            {
                  for (std::shared_ptr<pluginListStruct> last = pluginList[i], it = pluginList[i]->next;
                       it != nullptr;)
                  {
                        if (it->plugin->funName == pluginName)
                        {
                              status = true;
                              last->next = it->next;
                              if (it->next != nullptr)
                                    it = it->next;
                              else
                                    break;
                              continue;
                        }
                        last = it;
                        it = it->next;
                  }
                  if (pluginList[i]->plugin->funName == pluginName)
                  {
                        auto temp = pluginList[i]->next;
                        pluginList[i] = temp;
                        status = true;
                  }
            }
      }
      if (find(funPluginNameList.begin(),
               funPluginNameList.end(),
               pluginName) != funPluginNameList.end())
      {
            funPluginNameList.erase(
                find(funPluginNameList.begin(),
                     funPluginNameList.end(),
                     pluginName));
      }
      if (find(funPluginComdVerNameList.begin(),
               funPluginComdVerNameList.end(),
               pluginName) != funPluginComdVerNameList.end())
      {
            funPluginComdVerNameList.erase(
                find(funPluginComdVerNameList.begin(),
                     funPluginComdVerNameList.end(),
                     pluginName));
      }
      pluginNameList.erase(find(pluginNameList.begin(), pluginNameList.end(), pluginName));
      return status;
}
extern "C" bool EXPORT findPlugin(string pluginName)
{
      return (find(pluginNameList.begin(), pluginNameList.end(), pluginName) != pluginNameList.end());
}
extern "C" bool EXPORT rsetPlugin(string pluginName,
                                  string runlineS,
                                  void (*startupfunPtr)(),
                                  void (*startfunPtr)(),
                                  void (*stopfunPtr)(),
                                  bool (*runfunPtr)(allInfoStruct *info),
                                  bool isStart,
                                  map<string, string> (*cmdInfoGet)())
{
      if (!findPlugin(pluginName))
            return false;
      if (delPlugin(pluginName))
            return registerPlugin(pluginName,
                                  runlineS,
                                  startupfunPtr,
                                  startfunPtr,
                                  stopfunPtr,
                                  runfunPtr,
                                  isStart,
                                  cmdInfoGet);
      return false;
}
extern "C" bool EXPORT startPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
            return false;
      for (int i = 2; i < RUN_LINE_NUM; i++)
      {
            for (std::shared_ptr<pluginListStruct> it = pluginList[i]; it != nullptr; it = it->next)
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
extern "C" bool EXPORT stopPlugin(string pluginName)
{
      if (!findPlugin(pluginName))
            return false;
      for (int i = 2; i < RUN_LINE_NUM; i++)
      {
            for (std::shared_ptr<pluginListStruct> it = pluginList[i]; it != nullptr; it = it->next)
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
extern "C" bool EXPORT runPlugin(allInfoStruct &info, string runlineS)
{
      string pluginLine = runLineMap[runlineS];
      std::reverse(pluginLine.begin(), pluginLine.end());
      std::bitset<RUN_LINE_NUM> runline(pluginLine, 0);
      for (int i = 0; i < RUN_LINE_NUM; i++)
      {
            if (runline[i])
            {
                  for (std::shared_ptr<pluginListStruct> it = pluginList[i]; it != nullptr; it = it->next)
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
extern "C" bool EXPORT runFun(allInfoStruct *info, string Name)
{
      try
      {
            for (std::shared_ptr<pluginListStruct> it = pluginList[RunLine::Fun]; it != nullptr; it = it->next)
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

            return false;
      }
      catch (const std::exception &e)
      {
            (*info).msg = e.what();
            return false;
      }
}