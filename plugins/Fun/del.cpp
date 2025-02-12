#include "pluginFun.h"
extern "C" bool EXPORT run(allInfoStruct *info)
{
      SOCKET &sockC = (*info).NowSocket;
      while (1)
      {
            system("cls");
            int clientNum = 0;
            string clientNumStr;
            int kb_cin = -1;
            clientNum = sendClientList(sockC);
            if (_kbhit())
            {
                  getline(cin, clientNumStr);
                  if (!std::regex_match(clientNumStr, only_number))
                  {
                        Sleep(500);
                        send_message(sockC, "\r\nnext\r\n");
                        continue;
                  }
                  kb_cin = stoi(clientNumStr);
                  if (kb_cin == 0)
                  {
                        send_message(sockC, "\r\nexit\r\n");
                        break;
                  }
                  if (kb_cin > 0 && kb_cin <= clientNum)
                  {
                        string recvBuf;
                        cout << "wait for client until del(Yes/No):" << endl;
                        getline(cin, recvBuf);
                        transform(recvBuf.begin(), recvBuf.end(), recvBuf.begin(), ::tolower);
                        send_message(sockC, to_string(kb_cin));
                        if (recvBuf == "yes" || recvBuf == "y")
                              send_message(sockC, "Y");
                        else
                              send_message(sockC, "N");
                        receive_message(sockC, recvBuf);
                        if (recvBuf == "\r\nwait\r\n")
                        {
                              cout << "wait for server ...." << endl;
                              receive_message(sockC, recvBuf);
                              cout << "secces" << endl;
                        }
                        else if (recvBuf == "\r\nok\r\n")
                        {
                              cout << "secces" << endl;
                        }
                        else if (recvBuf == "\r\nUse\r\n")
                        {
                              cout << "Client is Using" << endl;
                        }
                        else if (recvBuf == "\r\nUnError\r\n")
                        {
                              cout << "Unknow Error" << endl;
                        }
                        system("pause");
                  }
            }
            else
            {
                  Sleep(500);
                  send_message(sockC, "\r\nnext\r\n");
            }
      }
      return true;
}