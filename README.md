# WinsockConnect
一个基于winsock2的远程控制软件     
编译命令：      
```
g++.exe -fdiagnostics-color=always -g -fexec-charset=UTF-8 -finput-charset=UTF-8 -std=c++17 PathAngName.cpp -o PathAngName.exe -lwsock32 -lws2_32 -static-libgcc -static-libstdc++ -lnetapi32
```
记得改一下 PathAngName      
你应该要编译:      
```
connet.cpp
serverForTelnetWindowsV.exe
server.cpp
```
编译后，将 `serverForTelnetWindowsV.exe connet.exe` 启动        
`server.exe` 中输入 `serverForTelnetWindowsV.exe` 的IP         
与 `serverConfig.config` 中的端口         
`passwoed.data` 有password        
