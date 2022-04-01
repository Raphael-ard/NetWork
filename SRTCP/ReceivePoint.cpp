#include "ReceivePoint.h"

#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

#define READ 0
#define WRITE 1
#define ACCEPT 2
#define _max_size 1024

NetWork::receivePoint*
NetWork::receivePoint::_rcvPoint = nullptr;

std::mutex
NetWork::receivePoint::_mx;

NetWork::receivePoint*
NetWork::receivePoint::
getInstace(void)
{
	if (_rcvPoint == nullptr)
	{
		std::unique_lock lck(_mx);
		if (_rcvPoint == nullptr)
		{
			_rcvPoint = new NetWork::receivePoint();
		}
	}
	return _rcvPoint;
}

void
NetWork::receivePoint::
destoryIns(void)
{
	std::unique_lock lck(_mx);
	if (_rcvPoint != nullptr)
		delete _rcvPoint;
	_rcvPoint = nullptr;
}

void
NetWork::receivePoint::
startReceiver(void)
{
	//准备调用 AcceptEx 函数，该函数使用重叠结构并于完成端口连接
	NetWork::PIOData perIoData = (NetWork::PIOData)GlobalAlloc(GPTR, sizeof(NetWork::IOData));
	memset(&(perIoData->overlapped), 0, sizeof(OVERLAPPED));
	perIoData->buf.buf = perIoData->dataBuffer;
	perIoData->buf.len = perIoData->dataLength = _max_size;
	perIoData->operatorType = ACCEPT;
	//在使用AcceptEx前需要事先重建一个套接字用于其第二个参数。这样目的是节省时间
	//通常可以创建一个套接字库
	perIoData->csock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

	DWORD flags = 0;

	//调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
	//注意这里使用了重叠模型，该函数的完成将在与完成端口关联的工作线程中处理
	std::cout << "Process AcceptEx function wait for send connect..." << std::endl;
	int rc = lpfnAcceptEx(_sock, perIoData->csock,
		perIoData->dataBuffer, 0
		/*perIoData->dataLength - ((sizeof(SOCKADDR_IN) + 16) * 2)*/,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &flags,
		&(perIoData->overlapped));
	if (rc == FALSE)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
			std::cout << "lpfnAcceptEx failed.." << std::endl;
	}
}

WSADATA 
NetWork::receivePoint::getWSAData(void)
{
	return _wsaData;
}

void
NetWork::receivePoint::
threadEvent(void)
{
	_thdPool.addTask([this](void) -> void
		{
			DWORD bytes;
			NetWork::PCompletionKey pComKey;

			NetWork::PIOData tpIoData;
			DWORD flags;
			int ret;
			DWORD RecvBytes;
			while (true)
			{
				pComKey = nullptr;
				tpIoData = nullptr;
				bytes = -1;
				ret = ::GetQueuedCompletionStatus(_hIocp, &bytes, reinterpret_cast<PULONG_PTR>(&pComKey),
					reinterpret_cast<LPOVERLAPPED*>(&tpIoData), INFINITE);

				if (!ret)
				{ // 接收有问题
					DWORD dwError = ::GetLastError();
					if (WAIT_TIMEOUT == dwError)
						continue;
					if (tpIoData != nullptr)
					{
						// 取消等待执行的异步操作
						CancelIo(reinterpret_cast<HANDLE>(pComKey->sock));
						::closesocket(pComKey->sock);
						::GlobalFree(pComKey);
						continue;
					}
					break;
				}
				else
				{
					if (bytes == 0 && (tpIoData->operatorType == READ ||
						tpIoData->operatorType == WRITE))
					{
						std::cout << "Send disconnect." << std::endl;
						::closesocket(pComKey->sock);
						::GlobalFree(pComKey);
						::GlobalFree(tpIoData);
						continue;
					}
					else
					{// 正常接收消息，开始处理
						runEvent(tpIoData, pComKey);
					}
				}
			}
		});
}

void
NetWork::receivePoint::
runEvent(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey)
{
	if (tpIoData->operatorType == READ)
	{
		if (strcmp(tpIoData->dataBuffer, "Connect the Send!") == 0)
		{
			remove(TAR);
			std::cout << tpIoData->dataBuffer << std::endl;
			Send(tpIoData, pComKey);
		}
		else if (strcmp(tpIoData->dataBuffer, "Send Complete!") == 0)
		{
			transCode();
			Recv(tpIoData, pComKey);
		}
		else if (strcmp(tpIoData->dataBuffer, "Start Send Message!") == 0)
		{
			remove(TAR);
			Recv(tpIoData, pComKey);
		}
		else
		{
			std::fstream fs;
			// 写入文件，剪贴板
			fs.open(TAR, std::fstream::in | std::fstream::out | std::fstream::app
				| std::fstream::binary);
			if (!fs)
			{
				std::cout << "Open Error" << std::endl;
				return;
			}
			if (fs.is_open())
			{
				fs.write(tpIoData->dataBuffer, 1024);
				fs.close();
			}
			Recv(tpIoData, pComKey);
		}
	}
	else if (tpIoData->operatorType == WRITE)
	{
		// 接收操作
		Recv(tpIoData, pComKey);
	}
	else if (tpIoData->operatorType == ACCEPT)
	{
		
		std::cout << "accpet success." << std::endl;
		if (::setsockopt(tpIoData->csock, SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			reinterpret_cast<char*>(&pComKey->sock),
			sizeof(pComKey->sock)) == SOCKET_ERROR)
		{
			std::cout << "SetSockOpt Error." << std::endl;
		}

		NetWork::PCompletionKey tmpIOData = reinterpret_cast<NetWork::PCompletionKey>(::GlobalAlloc(GPTR, sizeof(NetWork::CompletionKey)));
		tmpIOData->sock = tpIoData->csock;

		sockaddr_in* addrClient = NULL, * addrLocal = NULL;
		int nClientLen = sizeof(SOCKADDR_IN), nLocalLen = sizeof(SOCKADDR_IN);

		lpfnGetAcceptExSockaddrs(tpIoData->dataBuffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(LPSOCKADDR*)&addrLocal, &nLocalLen, (LPSOCKADDR*)&addrClient, &nClientLen);

		printf(tmpIOData->IP, " %d\n", addrClient->sin_port);	//cliAdd.sin_port );

		::CreateIoCompletionPort(reinterpret_cast<HANDLE>(tmpIOData->sock), _hIocp, reinterpret_cast<ULONG_PTR>(tmpIOData), 0);
		Recv(tpIoData, tmpIOData);
		// 接收到一个连接，再发起一个异步操作
		startReceiver();
	}
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

BOOL
NetWork::receivePoint::
Recv(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	ZeroMemory(&tpIoData->overlapped, sizeof(OVERLAPPED));

	tpIoData->operatorType = READ;
	tpIoData->buf.buf = tpIoData->dataBuffer;
	tpIoData->buf.len = tpIoData->dataLength = _max_size;

	if (SOCKET_ERROR == ::WSARecv(pComKey->sock, &tpIoData->buf, 1, &recvBytes, &flags, &tpIoData->overlapped, nullptr))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("发起重叠接收失败!   %d\n", ::GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}

BOOL
NetWork::receivePoint::
Send(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	ZeroMemory(&tpIoData->overlapped, sizeof(OVERLAPPED));

	tpIoData->operatorType = WRITE;
	tpIoData->buf.len = _max_size;
	strncpy_s(tpIoData->buf.buf, 1024, "Connect the Receiver!", 1024);

	if (SOCKET_ERROR == WSASend(pComKey->sock, &tpIoData->buf, 1, &recvBytes, flags, &tpIoData->overlapped, NULL))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("发起发送重叠失败!\n");
			return FALSE;
		}
	}

	return TRUE;
}

NetWork::receivePoint::
receivePoint() : _thdPool(10)
{
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &_wsaData);
	if (wsaResult != 0)
	{
		printf("WSAStartup failed: %d\n", wsaResult);
		return;
	}

	_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	if (_hIocp == nullptr)
	{
		printf("Create CompletionPort Failed %d\n", ::WSAGetLastError());
		return;
	}

	threadEvent();

	_sock = ::WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	//_sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (_sock == INVALID_SOCKET)
		return;
	// 将监听socket与完成端口绑定
	NetWork::PCompletionKey scKey;
	scKey = reinterpret_cast<NetWork::PCompletionKey>(::GlobalAlloc(GPTR, sizeof(NetWork::CompletionKey)));
	scKey->sock = _sock;
	::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_sock), _hIocp, reinterpret_cast<ULONG_PTR>(scKey), 0);

	int net_buf;

	std::wstring IPv6;
	NetWork::getIP(IPv4, IPv6);

	if (::InetPtonA(AF_INET, IPv4.c_str(), &net_buf) != 1)
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

	lpfnAcceptEx = nullptr;
	guidAcceptEx = WSAID_ACCEPTEX;
	dwBytes = 0;
	if (::WSAIoctl(_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx,
		sizeof(lpfnAcceptEx), &dwBytes, nullptr, nullptr) == 0)
	{
		std::cout << "AcceptEx Success." << std::endl;
	}
	else
	{
		printf("AcceptEx Failed. %d\n", ::WSAGetLastError());
		return;
	}

	lpfnGetAcceptExSockaddrs = nullptr;
	guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

	if (::WSAIoctl(_sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs,
		sizeof(guidGetAcceptExSockaddrs), &lpfnGetAcceptExSockaddrs,
		sizeof(lpfnGetAcceptExSockaddrs), &dwBytes1, nullptr, nullptr) == 0)
	{
		std::cout << "GetAcceptExSockAddrs Success." << std::endl;
	}
	else
	{
		printf("GetAcceptExSockAddrs Failed. %d\n", ::WSAGetLastError());
		return;
	}
}

NetWork::receivePoint::
~receivePoint()
{
	::closesocket(_sock);
	::WSACleanup();
}