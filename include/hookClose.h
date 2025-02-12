#include <windows.h>
class CloseCheak
{
public:
    enum CloseType
    {
        EwhenCtrl_c,
        EwhenCtrl_Break,
        EwhenClose,
        EwhenUserLogout,
        EwhenSystemClose
    };
    void setRunFun(_In_ void *_whenCtrl_c,
                   _In_ void *_whenCtrl_Break,
                   _In_ void *_whenClose,
                   _In_ void *_whenUserLogout,
                   _In_ void *_whenSystemClose)
    {
        this->instance->whenCtrl_c = _whenCtrl_c;
        this->instance->whenCtrl_Break = _whenCtrl_Break;
        this->instance->whenClose = _whenClose;
        this->instance->whenUserLogout = _whenUserLogout;
        this->instance->whenSystemClose = _whenSystemClose;
        return;
    }
    void startHook()
    {
        SetConsoleCtrlHandler(HandlerRoutine, TRUE);
    }
    void stopHook()
    {
        SetConsoleCtrlHandler(HandlerRoutine, FALSE);
    }
    CloseCheak operator=(const CloseCheak &other)
    {
        this->whenCtrl_c = other.whenCtrl_c;
        this->whenCtrl_Break = other.whenCtrl_Break;
        this->whenClose = other.whenClose;
        this->whenUserLogout = other.whenUserLogout;
        this->whenSystemClose = other.whenSystemClose;
        return *this;
    }
    CloseCheak()
    {
        // this->instance = nullptr;
        this->whenCtrl_c = nullptr;
        this->whenCtrl_Break = nullptr;
        this->whenClose = nullptr;
        this->whenUserLogout = nullptr;
        this->whenSystemClose = nullptr;
    }
    ~CloseCheak()
    {
        stopHook();
    }

private:
    void *whenCtrl_c;
    void *whenCtrl_Break;
    void *whenClose;
    void *whenUserLogout;
    void *whenSystemClose;
    static CloseCheak *instance;
    void *getRunFun(CloseType type)
    {
        switch (type)
        {
        case EwhenCtrl_c:
            return this->whenCtrl_c;
        case EwhenCtrl_Break:
            return this->whenCtrl_Break;
        case EwhenClose:
            return this->whenClose;
        case EwhenUserLogout:
            return this->whenUserLogout;
        case EwhenSystemClose:
            return this->whenSystemClose;
        }
        return NULL;
    }
    static BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
    {

        void *fun = nullptr; // 在 switch 语句外部定义 fun

        switch (dwCtrlType)
        {
        case CTRL_C_EVENT: // CTRL + C
            fun = instance->getRunFun(CloseCheak::EwhenCtrl_c);
            if (fun != NULL)
            {
                ((void (*)())fun)();
            }
            break;
        case CTRL_BREAK_EVENT: // CTRL + BREAK
            fun = instance->getRunFun(CloseCheak::EwhenCtrl_Break);
            if (fun != NULL)
            {
                ((void (*)())fun)();
            }
            break;
        case CTRL_CLOSE_EVENT: // 关闭
            fun = instance->getRunFun(CloseCheak::EwhenClose);
            if (fun != NULL)
            {
                ((void (*)())fun)();
                while (1)
                    ;
            }
            break;
        }

        return 0;
    }
};
CloseCheak *CloseCheak::instance = new CloseCheak;