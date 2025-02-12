#ifndef PLUGINFUN_H
#define PLUGINFUN_H
#define UNLOAD_PLUGIN
#define UNLOADSC_HEAD
#include "../../include/definehead.h"
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
#endif