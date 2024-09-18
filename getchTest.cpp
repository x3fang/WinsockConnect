#include <iostream>
#include <conio.h>
#include <string>
#include <windows.h>
using namespace std;

int main()
{
    string ch;
    while (1)
    {
        cout << "Enter a character: ";
        getline(cin, ch);
        if (ch.c_str()[1] == '\0')
        {
            cout << "12";
        }
        cout << "You entered: " << (int)ch[0] << endl;
    }

    system("pause");
    return 0;
}
