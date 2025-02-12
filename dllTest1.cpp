#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <bitset>
#define UNLOADSC_HEAD
#include "include/definehead.h"
using namespace std;
start_ptr start;
RegisterPluginFun_ptr regPlugin;
set_isServerHealthyCheckClose_ptr setServerHealthyCheck;
runFun_ptr connectFun, delFun, showFun;
DelPluginFun_ptr delPluginFun;
int main()
{
      // system("pause");
      HINSTANCE dllHandle = LoadLibrary("../A_dllB/serverForTelnetWindowsV_DLL.dll");
      HINSTANCE dllHandle2 = LoadLibrary("../A_dllB/simpleSMC.dll");

      if (dllHandle == NULL || dllHandle2 == NULL)
            return 1;
      setServerHealthyCheck = (set_isServerHealthyCheckClose_ptr)GetProcAddress(dllHandle, TEXT("set_isServerHealthyCheckClose"));
      start = (start_ptr)GetProcAddress(dllHandle, TEXT("start"));
      regPlugin = (RegisterPluginFun_ptr)GetProcAddress(dllHandle, TEXT("registerPlugin"));
      connectFun = (runFun_ptr)GetProcAddress(dllHandle2, TEXT("Connect"));
      delFun = (runFun_ptr)GetProcAddress(dllHandle2, TEXT("del"));
      showFun = (runFun_ptr)GetProcAddress(dllHandle2, TEXT("show"));
      delPluginFun = (DelPluginFun_ptr)GetProcAddress(dllHandle, TEXT("delPlugin"));
      if (start == NULL || regPlugin == NULL || connect == NULL)
            return 2;
      try
      {
            // setServerHealthyCheck(true);
            try
            {
                  cout << regPlugin("connect", "fun", NULL, NULL, NULL, connectFun, true);
            }
            catch (std::exception &e)
            {
                  cout << e.what();
            }
            cout << regPlugin("del", "fun", NULL, NULL, NULL, delFun, true);
            cout << regPlugin("show", "fun", NULL, NULL, NULL, showFun, true);
            // cout << delPluginFun("show");
            thread t = thread(start);
            t.join();
      }
      catch (std::exception &e)
      {
            std::cout << "Error occurred while calling start function of dll" << std::endl
                      << e.what();
      }
      FreeLibrary(dllHandle);
}