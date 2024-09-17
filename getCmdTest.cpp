#include "getCmd.h"
#include <iostream>
#include <string>
using namespace std;

int main()
{
    string cmd;
    cout << "Enter a command: ";
    getline(cin, cmd);
    cout << "The command entered is: " << filePath(cmd, 0, 1) << endl;
    system("pause");
    return 0;
}