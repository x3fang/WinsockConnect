#include "hookClose.h"
inline void hookClose()
{
    MessageBoxW(NULL, L"关闭事件", L"提示", MB_OK);
}
int main()
{
    system("CHCP 65001");
    hookClose();
    CloseCheak n;
    n.setRunFun((void *)hookClose, (void *)hookClose, (void *)hookClose, (void *)hookClose, (void *)hookClose);
    n.startHook();
    while (1)
        ;
}