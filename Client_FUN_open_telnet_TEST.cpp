
// 设置连接器选项
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include <winsock2.h>
// #include <ws2tcpip.h>
// #include <windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <random>
#include <time.h>
#include <string>
#include <string.h>
#include <winternl.h>
#include <intrin.h>
#include <fstream>
#include <atomic>
#include <sstream>
using namespace std;

int main(int argc, char *argv[])
{
      PROCESS_INFORMATION Processinfo;
      STARTUPINFO Startupinfo;
      HANDLE outputRead, outputWrite;
      HANDLE inputRead, inputWrite;
      SECURITY_ATTRIBUTES sa = {0};
      char szCMDPath[255];
      DWORD dwBytesRead = 0;
      DWORD dwBytesWrite = 0;

      sa.nLength = sizeof(sa);
      sa.bInheritHandle = TRUE;
      sa.lpSecurityDescriptor = NULL;

      CreatePipe(&outputRead, &outputWrite, &sa, 0); // 创建标准输出管道
      CreatePipe(&inputRead, &inputWrite, &sa, 0);   // 创建标准输入管道

      // 配内存资源，初始化数据
      ZeroMemory(&Processinfo, sizeof(PROCESS_INFORMATION));
      ZeroMemory(&Startupinfo, sizeof(STARTUPINFO));
      GetEnvironmentVariable("COMSPEC", szCMDPath, sizeof(szCMDPath));

      // 设置启动信息
      Startupinfo.cb = sizeof(STARTUPINFO);
      Startupinfo.wShowWindow = SW_HIDE;
      Startupinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      Startupinfo.hStdInput = (HANDLE)inputRead;
      Startupinfo.hStdOutput = (HANDLE)outputWrite;
      Startupinfo.hStdError = (HANDLE)outputWrite;

      // 创建cmd进程
      if (CreateProcess(NULL, szCMDPath, NULL, NULL, TRUE, 0, NULL, NULL, &Startupinfo, &Processinfo))
      {
            CloseHandle(outputWrite);
            CloseHandle(inputRead);

            DWORD mode = PIPE_NOWAIT;
            SetNamedPipeHandleState(outputRead, &mode, NULL, NULL);
            char cbuf[16384] = {0}, cbuf1[16384] = {0};
            int flag = 0; // 判断是否是第3次遇到 ERROR_NO_DATA (重试次数)
            string buf;
            // Sleep(10); // 等待子程序加载
            do
            {
                  ReadFile(outputRead, cbuf, sizeof(cbuf), &dwBytesRead, NULL);
                  DWORD error = GetLastError();
                  SetLastError(ERROR_SUCCESS);
                  if (error == ERROR_NO_DATA)
                  {
                        if (flag == 3)
                              break;
                        flag += 1;
                        Sleep(10);
                        continue;
                  }
                  buf += cbuf;
                  memset(cbuf, 0, sizeof(cbuf));
                  if (dwBytesRead == 0)
                        break;
            } while (true);
      }

      // 关闭进程句柄
      CloseHandle(Processinfo.hProcess);
      CloseHandle(Processinfo.hThread);
      cout << "close";
}
