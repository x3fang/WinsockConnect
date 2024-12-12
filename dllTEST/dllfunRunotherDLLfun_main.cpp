#include <windows.h>
#include <stdio.h>
typedef void (*FunctionType)();
typedef void (*FunctionType2)(FunctionType);
int main()
{
    HMODULE hDll1 = LoadLibrary("..//A_dllB/dllfunRunotherDLLfun_1.dll");
    HMODULE hDll2 = LoadLibrary("..//A_dllB/dllfunRunotherDLLfun_2.dll");

    if (hDll1 && hDll2)
    {
        // 获取 dll1 中函数的地址
        FunctionType func = (FunctionType)GetProcAddress(hDll2, "MyFunction");
        FunctionType2 func2 = (FunctionType2)GetProcAddress(hDll1, "CallFunction");

        // 在 dll2 中调用这个函数
        func2(func);

        // 释放 DLL
        FreeLibrary(hDll1);
        FreeLibrary(hDll2);
    }
    else
    {
        printf("Failed to load DLLs!\n");
    }

    return 0;
}
