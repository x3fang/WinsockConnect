#include "pluginFun.h"
extern "C" bool EXPORT run(allInfoStruct *info)
{
      SOCKET &sockC = (*info).NowSocket;
      while (true)
      {
            system("cls");
            sendClientList(sockC);
            if (_kbhit())
            {
                  cin.clear();
                  cin.sync();
                  send_message(sockC, "\r\nexit\r\n");
                  break;
            }
            Sleep(500);
            send_message(sockC, "\r\nnext\r\n");
      }
      return true;
}