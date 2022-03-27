#include "ReceivePoint.h"
#include "AllData.h"

#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

#define _max_size 1024

NetWork::receivePoint*
NetWork::receivePoint::_receive_point = nullptr;

std::mutex
NetWork::receivePoint::_mx;

NetWork::receivePoint* 
NetWork::receivePoint::
getInstance(void)
{// 线程安全的单例模式
	if (_receive_point == nullptr)
	{
		std::unique_lock lck(_mx);
		if (_receive_point == nullptr)
		{
			_receive_point = new receivePoint();
		}
	}
	return _receive_point;
}

void 
NetWork::receivePoint::
deleteInstance(void)
{
	std::unique_lock<std::mutex> lck(_mx);
	if (_receive_point != nullptr)
	{
		delete _receive_point;
		_receive_point = nullptr;
	}
}

void 
NetWork::receivePoint::
startServer(void)
{
	runEvent();

	//异步io，非异步选择
	while (true)
	{
		sockaddr_in addr_clnt{ 0 };
		int clnt_len = sizeof(addr_clnt);

		SOCKET sock_work = ::accept(_sock,
			reinterpret_cast<sockaddr*>(&addr_clnt),
			&clnt_len);

		remove(TAR);

		/* INVALID_SOCKET是判断套接字是否成功，SOCKET_ERROR是判断调用函数是否成功 */
		if (sock_work == INVALID_SOCKET)
			return;

		/* print address */
		char addr_str[46] = "";
		InetNtopA(AF_INET, (void*)&addr_clnt.sin_addr, addr_str, 46);
		printf("client address: %s\t port: %i\n", addr_str, ::ntohs(addr_clnt.sin_port));

		/* send & receive */
		int send_len = ::send(sock_work, "Connect to Server!", strlen("Connect to Server!") + 1, 0);
		if (send_len == SOCKET_ERROR)
			return;

		char* buf = new char[_max_size] { 0 };
		//int recv_len = ::recv(sock_work, buf, 100, 0);
		//---overlappedio自动接收---
		WSABUF wsa_buf;
		wsa_buf.len = _max_size;
		wsa_buf.buf = buf;
		DWORD received_count = 0l;
		OVERLAPPED overlapped;
		overlapped.hEvent = ::WSACreateEvent();   //异步选择，统一等待
		overlapped.Pointer = buf;  //带入参数，同时以相同方式带出参数
		DWORD flags = 0;
		int ret = ::WSARecv(sock_work, &wsa_buf, 1, &received_count,
			&flags, &overlapped, nullptr);

		if (ret != 0)
		{// WSARecv接收成功返回0
			int i = ::WSAGetLastError();
			/*return i;*/
		}
		::CreateIoCompletionPort(reinterpret_cast<HANDLE>(sock_work),
			_hIocp, static_cast<ULONG_PTR>(sock_work), 0);
	}
}

void 
NetWork::receivePoint::
runEvent(void)
{
	_thdPool.addTask([this](void) -> void
		{
			DWORD cReceived = 0;
			SOCKET sock_complete = INVALID_SOCKET;
			LPOVERLAPPED poverlapped_data;
			std::fstream fs;
			char buf[_max_size]{ 0 };
			//int recv_len = ::recv(sock_work, buf, 100, 0);
			//---overlappedio自动接收---
			WSABUF wsa_buf;
			wsa_buf.len = _max_size;
			wsa_buf.buf = buf;
			DWORD received_count = 0l;
			OVERLAPPED overlapped;
			overlapped.hEvent = ::WSACreateEvent();   //异步选择，统一等待
			overlapped.Pointer = buf;  //带入参数，同时以相同方式带出参数
			DWORD flags = 0;

			while (::GetQueuedCompletionStatus(_hIocp, &cReceived,
				reinterpret_cast<PULONG_PTR>(&sock_complete),
				&poverlapped_data, INFINITE))
			{
				//  成功接收-----处理事件
				if (cReceived > 0)
				{
					// 接受的消息
					NetWork::PMessageData rcv = reinterpret_cast<NetWork::PMessageData>(poverlapped_data->Pointer);

					fs.open(TAR, std::fstream::in | std::fstream::out | std::fstream::app
						| std::fstream::binary);
					if (!fs)
					{
						std::cout << "Open Error" << std::endl;
						continue;
					}
					if (fs.is_open())
					{
						fs.write(rcv->message.c_str(), rcv->message.size());
						fs.close();
					}
					// 转换编码并拷贝到剪贴板
					transCode();
					//int send_len = ::send(sock_complete, "Exit!", strlen("Exit!") + 1, 0);
					delete[] poverlapped_data->Pointer;
					// 删除文件夹（包含文件）
					/*delete _sock_addr[sock_complete];
					_sock_addr[sock_complete] = nullptr;*/

					// 重新启动接收
					int ret = ::WSARecv(sock_complete, &wsa_buf, 1, &received_count,
							&flags, &overlapped, nullptr);
					if (ret != 0)
					{// WSARecv接收成功返回0
						int i = ::WSAGetLastError();
						printf("Error: %d", i);
					}
				}
			}
			::closesocket(sock_complete);
		});
	
}

void 
NetWork::receivePoint::
getIp(std::string& str, int iResult)
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
			str = inet_ntoa(sockaddr_ipv4->sin_addr);
			printf("IPv4 address %s\n", str.c_str());
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
				std::wcout << L"IPv6 address  " << ipstringbuffer << std::endl;
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

void 
NetWork::receivePoint::
transCode(void)
{
	// 拷贝到剪贴板
	std::fstream fs1;
	fs1.open(TAR, std::ios::in);
	if (!fs1)
	{
		std::cout << "Open Error" << std::endl;
		return;
	}
	std::wstring str;
	while (!fs1.eof())
	{
		std::string ss;
		std::getline(fs1, ss);
		int i = MultiByteToWideChar(CP_UTF8, 0, ss.c_str(), -1, NULL, 0);
		wchar_t* strSrc = new wchar_t[i + 1];
		MultiByteToWideChar(CP_UTF8, 0, ss.c_str(), -1, strSrc, i);
		str += strSrc;
		str += L"\r\n";
		delete[] strSrc;
	}
	fs1.close();
	sendToClip(str.c_str());
	std::cout << "Success Copy." << std::endl;
	printf("\n");
	return;
}

bool 
NetWork::receivePoint::
sendToClip(const wchar_t* data_str)
{
	int str_len = wcslen(data_str);
	if (OpenClipboard(NULL))
	{
		HANDLE hClip;
		EmptyClipboard();
		hClip = GlobalAlloc(GMEM_DDESHARE, (str_len + 1) * sizeof(wchar_t));
		if (NULL == hClip)
		{
			return false;
		}
		wchar_t* pBuf = (wchar_t*)GlobalLock(hClip);
		if (NULL == pBuf)
		{
			return false;
		}
		memcpy(pBuf, data_str, (str_len + 1) * sizeof(wchar_t));
		GlobalUnlock(hClip);
		HANDLE hData = SetClipboardData(CF_UNICODETEXT, pBuf);
		if (NULL == hData)
		{
			return false;
		}
		CloseClipboard();
		return true;
	}
	return false;
}

NetWork::receivePoint::
receivePoint(): _thdPool(10)
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

	int net_buf;
	std::string str_addr;

	getIp(str_addr, wsaResult);

	if (::InetPtonA(AF_INET, str_addr.c_str(), &net_buf) != 1)
		return;
	_addr.sin_addr.S_un.S_addr = net_buf;
	_addr.sin_family = AF_INET;
	_addr.sin_port = ::htons(9008);

	if (::bind(_sock,
		reinterpret_cast<const sockaddr*>(&_addr),
		sizeof _addr) == SOCKET_ERROR)
		return;

	if (::listen(_sock, SOMAXCONN) == SOCKET_ERROR)
	{
		auto i = WSAGetLastError();
		return;
	}

	_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	//std::cout << "Start the Server" << std::endl;
}

NetWork::receivePoint::
~receivePoint()
{
	::closesocket(_sock);
	::WSACleanup();
}
