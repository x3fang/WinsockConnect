#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <bitset>
#include "definehead.h"
using namespace std;
start_ptr start;
RegisterPluginFun_ptr regPlugin;
runFun_ptr connectFun;
int main()
{
      // system("pause");
      HINSTANCE dllHandle = LoadLibrary("serverForTelnetWindowsV_DLL.dll");
      HINSTANCE dllHandle2 = LoadLibrary("simpleSMC.dll");
      if (dllHandle == NULL || dllHandle2 == NULL)
            return 1;
      start = (start_ptr)GetProcAddress(dllHandle, TEXT("start"));
      regPlugin = (RegisterPluginFun_ptr)GetProcAddress(dllHandle, TEXT("registerPlugin"));
      connectFun = (runFun_ptr)GetProcAddress(dllHandle2, TEXT("Connect"));
      if (start == NULL || regPlugin == NULL || connect == NULL)
            return 2;
      try
      {
            regPlugin("connect", "fun", NULL, NULL, NULL, connectFun, true);
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