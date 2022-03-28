#include "SendPoint.h"

#include <WinBase.h>
#include <iostream>
#include <WS2tcpip.h>
#include <atlstr.h>
#include <fstream>

NetWork::sendPoint::
sendPoint(void)
{
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &_wsaData);
	if (wsaResult != 0)
	{
		printf("WSAStartup failed: %d\n", wsaResult);
		return;
	}

	_sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (_sock == INVALID_SOCKET)
		return;
	_path = getMyDirectory();
	_mData = (PMessageData)malloc(sizeof(messageData));
	memset(_mData->sendIPv4, 0, 64);
	memset(_mData->sendIPv6, 0, 64);
	memset(_mData->message, 0, 1024);
}

NetWork::sendPoint::
~sendPoint(void)
{
	free(_mData->sendIPv4);
	free(_mData->sendIPv6);
	free(_mData->message);
	free(_mData);
	_mData = nullptr;

	CloseHandle(m_hDirectory);
	CloseHandle(hFile);
	::closesocket(_sock);
	::WSACleanup();
}

void 
NetWork::sendPoint::
startConnect(void)
{
	int net_buf;
	std::string serverAddr;
	std::cout << "Input Receiver Address: ";
	std::cin >> serverAddr;
	if (::InetPtonA(AF_INET, serverAddr.c_str(), &net_buf) != 1)
		return;

	_addr.sin_addr.S_un.S_addr = net_buf;
	_addr.sin_family = AF_INET;
	_addr.sin_port = ::htons(9008);
	if (::connect(_sock, reinterpret_cast<const sockaddr*>(&_addr),
		sizeof _addr) == SOCKET_ERROR)
	{
		auto x = ::GetLastError();
		printf("Error: \n", x);
	}

	_mData->sendPort = 9008;
	getIP();
	initOperateFile();

	while (true)
	{
		listenFile();
		// 开始发送消息
		int len = ::send(_sock, reinterpret_cast<char*>(_mData), sizeof(_mData), 0);
		if (len < 0)
		{
			printf("Send Error: %d", len);
		}
		int count = 0;
		while (_mData->message[count])
		{
			free(_mData->message[count]);
			_mData->message[count] = nullptr;
			++count;
		}
	}
	
}

std::wstring 
NetWork::sendPoint::
getMyDirectory(void)
{
	std::wstring wstr;
	unsigned long size = GetCurrentDirectoryW(0, NULL);
	wchar_t* path = new wchar_t[size];
	if (GetCurrentDirectory(size, path) != 0)
	{
		wstr = path;
	}
	delete[] path;
	return wstr;
}

void
NetWork::sendPoint::
initOperateFile(void)
{
	memset(notify, 0, sizeof(notify));
	pNotification = (FILE_NOTIFY_INFORMATION*)notify;
	DWORD BytesReturned = 0;

	hFile = CreateFile(L"tar.cpp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CloseHandle(hFile);


	m_hDirectory = CreateFile(_path.c_str(),
		GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS /*| FILE_FLAG_OVERLAPPEDFILE_ATTRIBUTE_NORMAL*/,
		NULL);
	if (m_hDirectory == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		std::cout << dwErr << std::endl;
		return;
	}
}

void 
NetWork::sendPoint::
listenFile(void)
{
	while (true)
	{
		DWORD BytesReturned = 0;

		ZeroMemory(pNotification, sizeof(notify));
		// 监控文件及目录
		bool watch_state = ReadDirectoryChangesW(m_hDirectory,
			&notify,
			sizeof(notify),
			false/*TRUE*/,   //监控子目录
			/*FILE_NOTIFY_CHANGE_FILE_NAME | */FILE_NOTIFY_CHANGE_LAST_WRITE, //FILE_NOTIFY_CHANGE_DIR_NAME FILE_NOTIFY_CHANGE_CREATION FILE_NOTIFY_CHANGE_SIZE
			(LPDWORD)&BytesReturned,
			NULL,
			NULL);
		if (GetLastError() == ERROR_INVALID_FUNCTION)
		{
			std::cout << "文件监控，系统不支持!" << std::endl;
			break;
		}
		else if (watch_state == FALSE)
		{
			DWORD dwErr = GetLastError();
			std::cout << "文件监控，监控失败!" << std::endl;
			break;
		}
		else if (GetLastError() == ERROR_NOTIFY_ENUM_DIR)
		{
			std::cout << "文件监控，内存溢出!" << std::endl;
			continue;
		}
		else if (pNotification->Action == FILE_ACTION_MODIFIED)
		{
			CString szFileName(pNotification->FileName, pNotification->FileNameLength / sizeof(wchar_t));
			if (szFileName.Compare(L"tar.cpp") == 0)
			{
				// 发送文件内容
				
				std::fstream fs;
				fs.open(TAR, std::ios::binary | std::ios::in);
				if (!fs)
				{
					std::cout << "Open Error" << std::endl;
					continue;
				}
				int count = 0;
				while (!fs.eof())
				{
					char* temp = (char*)malloc(sizeof(char)*1024);
					memset(temp, 0, 1024);
					fs.read(temp, 1024);
					_mData->message[count] = temp;
					++count;
				}
				fs.close();
				return;
			}
		}
	}
}

void 
NetWork::sendPoint::
getIP(void)
{
	INT iRetval;
	DWORD dwRetval;
	int i = 1;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	struct sockaddr_in* sockaddr_ipv4;
	//    struct sockaddr_in6 *sockaddr_ipv6;
	LPSOCKADDR sockaddr_ip;

	wchar_t ipstringbuffer[46];
	DWORD ipbufferlength = 46;


	//  存放主机名的缓冲区
	char szHost[256];
	//  取得本地主机名称
	::gethostname(szHost, 256);

	//-------------------------------
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	dwRetval = getaddrinfo(szHost, nullptr, &hints, &result);
	if (dwRetval != 0) {
		printf("getaddrinfo failed with error: %d\n", dwRetval);
		WSACleanup();
		return;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family) {
		case AF_UNSPEC:
			printf("Unspecified\n");
			break;
		case AF_INET:
			printf("AF_INET (IPv4)\n");
			sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			strncpy_s(_mData->sendIPv4, 64, inet_ntoa(sockaddr_ipv4->sin_addr), 
				strlen(inet_ntoa(sockaddr_ipv4->sin_addr)) + 1);

			printf("IPv4 address %s\n", _mData->sendIPv4);
			break;
		case AF_INET6:
			printf("AF_INET6 (IPv6)\n");
			sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
			ipbufferlength = 46;
			iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL,
				ipstringbuffer, &ipbufferlength);
			if (iRetval)
				printf("WSAAddressToString failed with %u\n", WSAGetLastError());
			else
			{
				std::wcout << L"IPv6 address  " << ipstringbuffer << std::endl;
				wcsncpy_s(_mData->sendIPv6, 64, ipstringbuffer, wcslen(ipstringbuffer) + 1);
			}
			break;
		case AF_NETBIOS:
			printf("AF_NETBIOS (NetBIOS)\n");
			break;
		default:
			printf("Other %ld\n", ptr->ai_family);
			break;
		}
	}
	freeaddrinfo(result);
}
