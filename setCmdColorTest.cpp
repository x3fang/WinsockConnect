#include <windows.h>
#include <iostream>
using namespace std;
void SetColor(unsigned short forecolor = 4, unsigned short backgroudcolor = 0)
{
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);             // 获取缓冲区句柄
    SetConsoleTextAttribute(hCon, forecolor | backgroudcolor); // 设置文本及背景色
}
int main()
{
    for (int i = 0; i < 16; i++)
    {
        cout << i << " ";
        SetColor(i, 0);
        cout << "Hello, world!" << endl;
    }

    SetColor(0, 4);
    cout << "This is a test." << endl;
    SetColor(4, 0);
    system("pause");
    return 0;
}