#include "plugin.h"

bool EXPORT registerPlugin(string pluginName,
                           string runlineS,
                           void (*startupfunPtr)(),
                           void (*startfunPtr)(),
                           void (*stopfunPtr)(),
                           bool (*runfunPtr)(allInfoStruct &info),
                           bool isStart)
{
    cout << runLineMap[runlineS] << endl;
    std::bitset<RUN_LINE_NUM> runline(runLineMap[runlineS], 0);
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
    for (int i = 2; i < RUN_LINE_NUM; i++)
    {
        if (runline[i])
        {
            cout << "'" << i;
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
                       bool (*runfunPtr)(allInfoStruct &info),
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
            if (it->plugin->funName == pluginName && !it->plugin->isStart)
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
            if (it->plugin->funName == pluginName && it->plugin->isStart)
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
    cout << runlineS << endl
         << runLineMap[runlineS] << endl
         << ":";
    string pluginLine = runLineMap[runlineS];
    reverse(pluginLine.begin(), pluginLine.end());
    cout << pluginLine << endl;
    std::bitset<RUN_LINE_NUM> runline(pluginLine);
    for (int i = 0; i < RUN_LINE_NUM; i++)
    {
        cout << runline[i];
    }
    cout << endl;
    for (int i = 1; i < RUN_LINE_NUM; i++)
    {
        if (runline[i])
        {
            cout << runline[i] << i << '/';
            for (pluginListStruct *it = pluginList[i]; it != nullptr; it = it->next)
            {
                cout << it->plugin->funName << endl;
                if (it->plugin->isStart)
                {
                    if (it->plugin->runFun(info))
                        return false;
                }
            }
        }
    }
    cout << "l'\n";
    return true;
}